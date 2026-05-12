#!/usr/bin/env python3
"""
.deepx/ Framework Integrity Checker

Validates the structural integrity of the .deepx/ knowledge base directory.
Checks for required files, cross-references, YAML frontmatter, interaction
markers, and content quality.

Usage:
    python3 validate_framework.py [--deepx-root DIR] [--fix] [--json]

Exit codes:
    0  All checks passed
    1  Validation errors found
    2  .deepx/ directory not found
"""

import argparse
import json
import os
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Set


# ---------------------------------------------------------------------------
# Expected .deepx/ structure (33 files across 11 categories)
# ---------------------------------------------------------------------------

REQUIRED_FILES = {
    "agents": [
        "dx-stream-builder.md",
        "dx-pipeline-builder.md",
        "dx-model-manager.md",
    ],
    "instructions": [
        "architecture.md",
        "coding-standards.md",
        "gstreamer-pipeline.md",
        "testing-patterns.md",
        "agent-protocols.md",
        "orchestration.md",
    ],
    "skills": [
        "dx-agentic-stream-build-pipeline.md",
        "dx-agentic-stream-build-mqtt-kafka.md",
        "dx-agentic-stream-model-management.md",
        "dx-agentic-stream-validate.md",
    ],
    "toolsets": [
        "dx-stream-elements.md",
        "dx-stream-metadata.md",
        "dx-engine-api.md",
        "model-registry.md",
    ],
    "memory": [
        "MEMORY.md",
        "common_pitfalls.md",
        "pipeline_optimization.md",
        "platform_api.md",
    ],
    "knowledge": [
        "knowledge_base.yaml",
    ],
    "prompts": [
        "new-stream-pipeline.md",
        "new-mqtt-kafka-app.md",
        "orchestrated-build.md",
    ],
    "contextual-rules": [
        "stream-pipelines.md",
        "postprocess.md",
        "tests.md",
    ],
    "scripts": [
        "validate_app.py",
        "validate_framework.py",
    ],
    ".": [
        "README.md",
    ],
}


# ---------------------------------------------------------------------------
# Data types
# ---------------------------------------------------------------------------

@dataclass
class CheckResult:
    category: str
    check: str
    passed: bool
    message: str
    auto_fixable: bool = False


@dataclass
class FrameworkValidation:
    deepx_root: str
    results: List[CheckResult] = field(default_factory=list)

    @property
    def errors(self) -> List[CheckResult]:
        return [r for r in self.results if not r.passed]

    @property
    def passed(self) -> bool:
        return len(self.errors) == 0

    def add(self, category: str, check: str, passed: bool, message: str,
            auto_fixable: bool = False):
        self.results.append(CheckResult(
            category=category, check=check, passed=passed,
            message=message, auto_fixable=auto_fixable
        ))

    def summary(self) -> str:
        total = len(self.results)
        passed_count = sum(1 for r in self.results if r.passed)
        failed_count = total - passed_count
        fixable = sum(1 for r in self.errors if r.auto_fixable)
        status = "PASS" if self.passed else "FAIL"
        return (
            f"\n{'=' * 60}\n"
            f"  Framework Integrity: {status}\n"
            f"  {passed_count}/{total} checks passed, "
            f"{failed_count} failed"
            f"{f' ({fixable} auto-fixable)' if fixable else ''}\n"
            f"  Root: {self.deepx_root}\n"
            f"{'=' * 60}"
        )


# ---------------------------------------------------------------------------
# Category 1: File Existence
# ---------------------------------------------------------------------------

def check_file_existence(deepx_root: str, result: FrameworkValidation):
    """Check all 33 required files exist."""
    for category, files in REQUIRED_FILES.items():
        if category == ".":
            cat_dir = deepx_root
        else:
            cat_dir = os.path.join(deepx_root, category)

        if category != "." and not os.path.isdir(cat_dir):
            result.add("existence", f"directory:{category}",
                       False, f"Missing directory: {category}/")
            continue

        for filename in files:
            filepath = os.path.join(cat_dir, filename)
            exists = os.path.isfile(filepath)
            if exists:
                result.add("existence", f"file:{category}/{filename}",
                           True, f"Found: {category}/{filename}")
            else:
                result.add("existence", f"file:{category}/{filename}",
                           False, f"Missing: {category}/{filename}",
                           auto_fixable=False)


# ---------------------------------------------------------------------------
# Category 2: Agent YAML Frontmatter
# ---------------------------------------------------------------------------

def check_agent_frontmatter(deepx_root: str, result: FrameworkValidation):
    """Check agent files have valid YAML frontmatter."""
    agents_dir = os.path.join(deepx_root, "agents")
    if not os.path.isdir(agents_dir):
        return

    required_fields = {"name", "description", "version"}

    for filename in os.listdir(agents_dir):
        if not filename.endswith(".md"):
            continue

        filepath = os.path.join(agents_dir, filename)
        content = Path(filepath).read_text(encoding="utf-8", errors="replace")

        # Check for YAML frontmatter
        fm_match = re.match(r'^---\s*\n(.*?)\n---', content, re.DOTALL)
        if not fm_match:
            result.add("frontmatter", f"agent:{filename}",
                       False, f"Missing YAML frontmatter in {filename}")
            continue

        frontmatter = fm_match.group(1)
        found_fields = set(re.findall(r'^(\w+):', frontmatter, re.MULTILINE))

        missing = required_fields - found_fields
        if missing:
            result.add("frontmatter", f"agent:{filename}",
                       False, f"Missing frontmatter fields in {filename}: "
                              f"{', '.join(sorted(missing))}")
        else:
            result.add("frontmatter", f"agent:{filename}",
                       True, f"Frontmatter OK: {filename}")


# ---------------------------------------------------------------------------
# Category 3: Interaction Markers (agents)
# ---------------------------------------------------------------------------

def check_interaction_markers(deepx_root: str, result: FrameworkValidation):
    """Check agent files have <!-- INTERACTION: ... --> markers."""
    agents_dir = os.path.join(deepx_root, "agents")
    if not os.path.isdir(agents_dir):
        return

    for filename in os.listdir(agents_dir):
        if not filename.endswith(".md"):
            continue

        filepath = os.path.join(agents_dir, filename)
        content = Path(filepath).read_text(encoding="utf-8", errors="replace")

        markers = re.findall(r'<!--\s*INTERACTION:\s*(.*?)\s*-->', content)
        if not markers:
            result.add("interaction", f"agent:{filename}",
                       False, f"No INTERACTION markers in {filename}")
        else:
            result.add("interaction", f"agent:{filename}",
                       True, f"Found {len(markers)} INTERACTION marker(s) "
                             f"in {filename}")


# ---------------------------------------------------------------------------
# Category 4: Cross-References
# ---------------------------------------------------------------------------

def check_cross_references(deepx_root: str, result: FrameworkValidation):
    """Check that internal file references point to existing files."""
    all_files: Dict[str, str] = {}
    for root, dirs, files in os.walk(deepx_root):
        for f in files:
            full = os.path.join(root, f)
            rel = os.path.relpath(full, deepx_root)
            all_files[rel] = full

    ref_pattern = re.compile(
        r'(?:\.deepx/|deepx/)([a-zA-Z0-9_\-./]+\.(?:md|yaml|py))'
    )

    for rel_path, full_path in all_files.items():
        if not full_path.endswith((".md", ".yaml")):
            continue

        content = Path(full_path).read_text(encoding="utf-8", errors="replace")
        refs = ref_pattern.findall(content)

        for ref in refs:
            ref_normalized = ref.lstrip("/")
            if ref_normalized not in all_files:
                result.add("cross-ref", f"{rel_path} → {ref_normalized}",
                           False,
                           f"Broken reference in {rel_path}: "
                           f".deepx/{ref_normalized}")

    # Report if no broken refs found
    broken = [r for r in result.results
              if r.category == "cross-ref" and not r.passed]
    if not broken:
        result.add("cross-ref", "all",
                   True, "No broken cross-references found")


# ---------------------------------------------------------------------------
# Category 5: Content Quality
# ---------------------------------------------------------------------------

def check_content_quality(deepx_root: str, result: FrameworkValidation):
    """Check for common content quality issues."""
    for root, dirs, files in os.walk(deepx_root):
        for f in files:
            if not f.endswith((".md", ".yaml")):
                continue

            full_path = os.path.join(root, f)
            rel_path = os.path.relpath(full_path, deepx_root)
            content = Path(full_path).read_text(
                encoding="utf-8", errors="replace"
            )

            # Check for Korean text (exclude intentional Response Language
            # instructions that contain Korean examples like "한글 음차 표기 금지"
            # and established loanword examples like "모델, 서버, 파일, 데이터")
            allowed_korean_patterns = [
                r'한글\s*음차\s*표기\s*금지',
                r'모델,?\s*서버,?\s*파일,?\s*데이터',
            ]
            filtered_content = content
            for pat in allowed_korean_patterns:
                filtered_content = re.sub(pat, '', filtered_content)
            korean_chars = re.findall(r'[\uac00-\ud7af]', filtered_content)
            if korean_chars:
                result.add("content", f"korean:{rel_path}",
                           False,
                           f"Korean text found in {rel_path} "
                           f"({len(korean_chars)} char(s))")

            # Check minimum file size (non-trivial content)
            if len(content.strip()) < 50:
                result.add("content", f"size:{rel_path}",
                           False,
                           f"File {rel_path} appears nearly empty "
                           f"({len(content.strip())} chars)")

    # Report if no quality issues
    quality_errors = [r for r in result.results
                      if r.category == "content" and not r.passed]
    if not quality_errors:
        result.add("content", "all",
                   True, "No content quality issues found")


# ---------------------------------------------------------------------------
# Category 6: Skill Structure
# ---------------------------------------------------------------------------

def check_skill_structure(deepx_root: str, result: FrameworkValidation):
    """Check skill files have required sections."""
    skills_dir = os.path.join(deepx_root, "skills")
    if not os.path.isdir(skills_dir):
        return

    required_headings = ["overview", "usage"]

    # Process skills follow their own structure (Gate Function, Iron Law, etc.)
    process_skills = {
        "dx-verify-completion.md",
        "dx-brainstorm-and-plan.md",
        "dx-tdd.md",
    }

    for filename in os.listdir(skills_dir):
        if not filename.endswith(".md"):
            continue
        if filename in process_skills:
            result.add("skill-structure", f"skill:{filename}",
                       True, f"Skill structure OK (process skill): {filename}")
            continue

        filepath = os.path.join(skills_dir, filename)
        content = Path(filepath).read_text(encoding="utf-8", errors="replace")
        headings = [h.lower().strip()
                    for h in re.findall(r'^##\s+(.+)$', content, re.MULTILINE)]

        missing = [h for h in required_headings
                   if not any(h in heading for heading in headings)]
        if missing:
            result.add("skill-structure", f"skill:{filename}",
                       False,
                       f"Missing section(s) in {filename}: "
                       f"{', '.join(missing)}")
        else:
            result.add("skill-structure", f"skill:{filename}",
                       True, f"Skill structure OK: {filename}")


# ---------------------------------------------------------------------------
# Category 7: Knowledge Base YAML
# ---------------------------------------------------------------------------

def check_knowledge_yaml(deepx_root: str, result: FrameworkValidation):
    """Check knowledge_base.yaml is valid YAML with expected structure."""
    yaml_path = os.path.join(deepx_root, "knowledge", "knowledge_base.yaml")
    if not os.path.isfile(yaml_path):
        result.add("knowledge", "yaml-exists",
                   False, "knowledge_base.yaml not found")
        return

    content = Path(yaml_path).read_text(encoding="utf-8", errors="replace")

    # Basic YAML structure checks (without importing yaml)
    expected_keys = ["bottleneck_patterns", "insights", "recipes"]
    for key in expected_keys:
        if f"{key}:" not in content:
            result.add("knowledge", f"yaml-key:{key}",
                       False,
                       f"Missing top-level key '{key}' in knowledge_base.yaml")
        else:
            result.add("knowledge", f"yaml-key:{key}",
                       True, f"Found key: {key}")


# ---------------------------------------------------------------------------
# Category 8: README Completeness
# ---------------------------------------------------------------------------

def check_readme(deepx_root: str, result: FrameworkValidation):
    """Check README.md has required sections."""
    readme_path = os.path.join(deepx_root, "README.md")
    if not os.path.isfile(readme_path):
        result.add("readme", "exists", False, "README.md not found")
        return

    content = Path(readme_path).read_text(encoding="utf-8", errors="replace")
    required_sections = [
        "context routing",
        "skills",
        "directory",
    ]

    for section in required_sections:
        if section.lower() in content.lower():
            result.add("readme", f"section:{section}",
                       True, f"README section found: {section}")
        else:
            result.add("readme", f"section:{section}",
                       False,
                       f"README missing section: {section}")


# ---------------------------------------------------------------------------
# Main validation runner
# ---------------------------------------------------------------------------

def validate_framework(deepx_root: str) -> FrameworkValidation:
    """Run all framework integrity checks."""
    result = FrameworkValidation(deepx_root=deepx_root)

    if not os.path.isdir(deepx_root):
        result.add("existence", "root", False,
                   f".deepx/ directory not found: {deepx_root}")
        return result

    check_file_existence(deepx_root, result)
    check_agent_frontmatter(deepx_root, result)
    check_interaction_markers(deepx_root, result)
    check_cross_references(deepx_root, result)
    check_content_quality(deepx_root, result)
    check_skill_structure(deepx_root, result)
    check_knowledge_yaml(deepx_root, result)
    check_readme(deepx_root, result)

    return result


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def parse_args():
    parser = argparse.ArgumentParser(
        description="Validate .deepx/ framework integrity"
    )
    parser.add_argument(
        "--deepx-root",
        default=os.path.join(os.path.dirname(os.path.dirname(
            os.path.abspath(__file__)))),
        help="Path to .deepx/ directory (default: auto-detect from script)"
    )
    parser.add_argument(
        "--json", action="store_true",
        help="Output results as JSON"
    )
    return parser.parse_args()


def main():
    args = parse_args()

    # Auto-detect: if deepx-root doesn't end with .deepx, check
    deepx_root = args.deepx_root
    if not deepx_root.endswith(".deepx"):
        candidate = os.path.join(deepx_root, ".deepx")
        if os.path.isdir(candidate):
            deepx_root = candidate

    result = validate_framework(deepx_root)

    if args.json:
        output = {
            "deepx_root": result.deepx_root,
            "passed": result.passed,
            "total_checks": len(result.results),
            "errors": len(result.errors),
            "results": [
                {
                    "category": r.category,
                    "check": r.check,
                    "passed": r.passed,
                    "message": r.message,
                    "auto_fixable": r.auto_fixable,
                }
                for r in result.results
            ],
        }
        print(json.dumps(output, indent=2))
    else:
        # Group by category
        categories: Dict[str, List[CheckResult]] = {}
        for r in result.results:
            categories.setdefault(r.category, []).append(r)

        for cat, checks in categories.items():
            print(f"\n--- {cat.upper()} ---")
            for check in checks:
                status = "PASS" if check.passed else "FAIL"
                print(f"  [{status}] {check.message}")

        print(result.summary())

    sys.exit(0 if result.passed else 1)


if __name__ == "__main__":
    main()
