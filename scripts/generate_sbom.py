#!/usr/bin/env python3
"""Generate CycloneDX SBOM from the full dx_stream project.

Scans ALL meson.build files across the project tree (main plugin, custom
libraries, postprocess models, apps) and the Python bindings (pydxs) to
produce a comprehensive CycloneDX 1.5 JSON SBOM.

Usage:
    python3 scripts/generate_sbom.py [--output PATH]
"""
import argparse
import glob
import json
import os
import re
import sys
import uuid
from datetime import datetime, timezone


def find_project_root():
    return os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))


def read_json(path):
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def read_release_version(project_root):
    ver_file = os.path.join(project_root, "release.ver")
    if os.path.isfile(ver_file):
        with open(ver_file, encoding="utf-8") as f:
            ver = f.read().strip()
        return ver.lstrip("v")
    return "unknown"


# ── Meson introspection (main plugin) ──────────────────────────────────────

def collect_introspection_deps(build_dir):
    """Collect dependencies from Meson introspection JSON (already-built)."""
    deps_file = os.path.join(build_dir, "meson-info", "intro-dependencies.json")
    if not os.path.isfile(deps_file):
        return {}
    deps = read_json(deps_file)
    result = {}
    for dep in deps:
        name = dep.get("name", "unknown")
        version = dep.get("version", "unknown")
        dep_type = dep.get("type", "unknown")
        if name not in result:
            result[name] = {"version": version, "type": dep_type}
    return result


# ── Meson.build parser (for sub-projects without introspection data) ───────

def parse_meson_build(meson_path):
    """Parse a meson.build file and extract project info + dependencies."""
    with open(meson_path, encoding="utf-8") as f:
        content = f.read()

    # Extract project name & version
    proj_match = re.search(
        r"project\(\s*'([^']+)'.*?version\s*:\s*'([^']+)'", content, re.DOTALL
    )
    proj_name = proj_match.group(1) if proj_match else os.path.basename(os.path.dirname(meson_path))
    proj_version = proj_match.group(2) if proj_match else "1.0.0"

    # Extract output type: shared_library or executable
    if re.search(r"\bshared_library\b", content):
        output_type = "library"
    elif re.search(r"\bexecutable\b", content):
        output_type = "application"
    else:
        output_type = "library"

    # Extract dependency('name') calls
    deps = {}
    for m in re.finditer(r"dependency\(\s*'([^']+)'", content):
        dep_name = m.group(1)
        ver_match = re.search(
            rf"dependency\(\s*'{re.escape(dep_name)}'.*?version\s*:\s*'([^']+)'",
            content, re.DOTALL
        )
        deps[dep_name] = ver_match.group(1) if ver_match else None

    # Extract find_library('name') / declare_dependency with -l
    for m in re.finditer(r"find_library\(\s*'([^']+)'", content):
        deps[m.group(1)] = None
    for m in re.finditer(r"-l(\w+)", content):
        lib = m.group(1)
        if lib not in deps:
            deps[lib] = None

    return {
        "project_name": proj_name,
        "project_version": proj_version,
        "output_type": output_type,
        "dependencies": deps,
        "path": meson_path,
    }


def classify_subproject(rel_path):
    """Classify a meson.build by its relative path to assign a group."""
    if "postprocess_library" in rel_path:
        return "postprocess"
    if "preprocess_library" in rel_path:
        return "preprocess"
    if "message_convert_library" in rel_path:
        return "custom-library"
    if "/apps/" in rel_path:
        return "application"
    if "test_plugin" in rel_path or "test_usermeta" in rel_path:
        return "test"
    if "gst-dxstream-plugin" in rel_path:
        return "core-plugin"
    return "other"


# ── Python bindings ────────────────────────────────────────────────────────

def collect_python_components(project_root):
    """Collect pydxs and its build-time dependencies."""
    components = []
    toml_path = os.path.join(
        project_root, "bindings", "python", "pydxs", "pyproject.toml"
    )
    name = "pydxs"
    version = "0.1.0"
    build_requires = []
    if os.path.isfile(toml_path):
        section = None
        with open(toml_path, encoding="utf-8") as f:
            for line in f:
                stripped = line.strip()
                if stripped.startswith("["):
                    section = stripped
                elif section == "[build-system]" and stripped.startswith("requires"):
                    # Parse list: requires = ["setuptools>=40.8.0", ...]
                    for m in re.finditer(r'"([^"]+)"', stripped):
                        build_requires.append(m.group(1))
                elif section == "[project]":
                    if stripped.startswith("version"):
                        version = stripped.split("=", 1)[1].strip().strip('"')
                    elif stripped.startswith("name"):
                        name = stripped.split("=", 1)[1].strip().strip('"')

    # pydxs itself
    components.append({
        "type": "library",
        "name": name,
        "version": version,
        "purl": f"pkg:pypi/{name}@{version}",
        "bom-ref": f"python:{name}@{version}",
        "group": "python-bindings",
        "scope": "optional",
        "description": "Python bindings for DX Stream",
    })

    # Build dependencies (setuptools, pybind11, wheel)
    for req in build_requires:
        m = re.match(r"([a-zA-Z0-9_-]+)([><=!]+)?(.*)?", req)
        if m:
            dep_name = m.group(1)
            dep_ver = (m.group(2) or "") + (m.group(3) or "")
            components.append({
                "type": "library",
                "name": dep_name,
                "version": dep_ver if dep_ver else "latest",
                "purl": f"pkg:pypi/{dep_name}",
                "bom-ref": f"python:{dep_name}@{dep_ver or 'latest'}",
                "group": "python-build-deps",
                "scope": "optional",
            })
    return components


# ── SBOM Assembly ──────────────────────────────────────────────────────────

def generate_sbom(project_root, output_path):
    build_dir = os.path.join(project_root, "gst-dxstream-plugin", "builddir")

    # 1. Gather version-resolved deps from main plugin introspection
    introspection_deps = collect_introspection_deps(build_dir)

    # 2. Find ALL meson.build files across the project
    meson_files = glob.glob(
        os.path.join(project_root, "**", "meson.build"), recursive=True
    )
    # Exclude build directories
    meson_files = [
        f for f in meson_files
        if "/builddir" not in f and "/builddir_test" not in f
    ]

    # 3. Parse each sub-project
    subprojects = []
    all_deps = {}  # name -> {version, type}
    for mf in sorted(meson_files):
        info = parse_meson_build(mf)
        rel = os.path.relpath(mf, project_root)
        info["group"] = classify_subproject(rel)
        info["rel_path"] = rel
        subprojects.append(info)

        # Merge dependencies
        for dep_name, dep_ver_constraint in info["dependencies"].items():
            if dep_name in introspection_deps:
                all_deps[dep_name] = introspection_deps[dep_name]
            elif dep_name not in all_deps:
                all_deps[dep_name] = {
                    "version": dep_ver_constraint or "unknown",
                    "type": "pkgconfig",
                }

    # 4. Build components list
    components = []

    # 4a. Sub-project components (our own code)
    for sp in subprojects:
        components.append({
            "type": sp["output_type"],
            "name": sp["project_name"],
            "version": sp["project_version"],
            "purl": f"pkg:generic/deepx/{sp['project_name']}@{sp['project_version']}",
            "bom-ref": f"subproject:{sp['project_name']}@{sp['project_version']}",
            "group": sp["group"],
            "description": f"Source: {sp['rel_path']}",
        })

    # 4b. External dependency components
    # Skip internal deps that are our own sub-projects
    internal_names = {sp["project_name"] for sp in subprojects}
    internal_names.add("gstdxstream")  # built by main plugin
    for dep_name, dep_info in sorted(all_deps.items()):
        if dep_name in internal_names:
            continue
        version = dep_info["version"]
        dep_type = dep_info.get("type", "unknown")
        if dep_type == "pkgconfig":
            purl = f"pkg:pkg-config/{dep_name}@{version}"
        else:
            purl = f"pkg:generic/{dep_name}@{version}"
        components.append({
            "type": "library",
            "name": dep_name,
            "version": version,
            "purl": purl,
            "bom-ref": f"dep:{dep_name}@{version}",
            "group": "external-dependency",
            "scope": "required",
        })

    # 4c. Python components
    components.extend(collect_python_components(project_root))

    # 5. Assemble BOM
    release_version = read_release_version(project_root)
    project_name = "dx-stream"
    project_file = os.path.join(build_dir, "meson-info", "intro-projectinfo.json")
    if os.path.isfile(project_file):
        proj = read_json(project_file)
        project_name = proj.get("descriptive_name", project_name)

    bom = {
        "bomFormat": "CycloneDX",
        "specVersion": "1.5",
        "serialNumber": f"urn:uuid:{uuid.uuid4()}",
        "version": 1,
        "metadata": {
            "timestamp": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
            "component": {
                "type": "application",
                "name": project_name,
                "version": release_version,
                "licenses": [{"license": {"id": "LicenseRef-DEEPX-Proprietary"}}],
            },
            "tools": [{"name": "generate_sbom.py", "version": "2.0.0"}],
        },
        "components": components,
    }

    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(bom, f, indent=2, ensure_ascii=False)

    # Summary
    groups = {}
    for c in components:
        g = c.get("group", "other")
        groups[g] = groups.get(g, 0) + 1

    print(f"SBOM generated: {output_path}")
    print(f"  Format:     CycloneDX 1.5 (JSON)")
    print(f"  Project:    {project_name} v{release_version}")
    print(f"  Components: {len(components)} total")
    for g, count in sorted(groups.items()):
        print(f"    - {g}: {count}")


def main():
    project_root = find_project_root()
    default_output = os.path.join(project_root, "bom.cdx.json")

    parser = argparse.ArgumentParser(
        description="Generate CycloneDX SBOM for the full dx_stream project"
    )
    parser.add_argument(
        "--output", "-o",
        default=default_output,
        help=f"Output SBOM file path (default: {default_output})",
    )
    args = parser.parse_args()

    generate_sbom(project_root, args.output)


if __name__ == "__main__":
    main()
