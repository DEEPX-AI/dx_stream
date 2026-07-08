#!/usr/bin/env python3
"""
dx_stream Pipeline Application Validator

Validates a dx_stream pipeline application for correctness, completeness,
and runtime readiness. Runs static analysis, property validation,
file-existence checks, and optional smoke tests.

Usage:
    python3 validate_app.py <script_path> [--smoke-test] [--model-dir DIR]

Exit codes:
    0  All checks passed
    1  Validation errors found
    2  Script or file not found
"""

import argparse
import json
import os
import re
import subprocess
import sys
from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path
from typing import List, Optional, Tuple


# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

DX_ELEMENTS = [
    "dxpreprocess", "dxinfer", "dxpostprocess", "dxtracker",
    "dxosd", "dxgather", "dxinputselector", "dxoutputselector",
    "dxrate", "dxmsgconv", "dxmsgbroker", "dxscale", "dxconvert",
]

POSTPROCESS_LIB_DIR = "/usr/local/share/gstdxstream/lib"

# Known valid models from model_list.json v2.3.0
KNOWN_MODELS = [
    "EfficientNet_Lite0.dxnn", "SCRFD500M.dxnn", "YoloV5S_PPU.dxnn",
    "YOLOv5s_Face.dxnn", "yolo26n.dxnn", "yolo26n-pose.dxnn",
    "yolo26n-seg.dxnn", "YoloV5S.dxnn", "YoloV7.dxnn", "YoloV8N.dxnn",
    "YoloV9S.dxnn", "YoloXS.dxnn", "YOLOV11N.dxnn", "yolov8m_pose.dxnn",
]

# Model-to-postprocess mapping
POSTPROCESS_MAP = {
    "EfficientNet_Lite0": "libpostprocess_object_class.so",
    "SCRFD500M": "libpostprocess_scrfd500m.so",
    "YoloV5S_PPU": "libpostprocess_ppu.so",
    "YOLOv5s_Face": "libpostprocess_yolov5s_face.so",
    "yolo26n": "libpostprocess_yolo26od.so",
    "yolo26n-pose": "libpostprocess_yolo26pose.so",
    "yolo26n-seg": "libpostprocess_yolo26seg.so",
    "YoloV5S": "libpostprocess_yolov5s_6.so",
    "YoloV7": "libpostprocess_yolov7.so",
    "YoloV8N": "libpostprocess_yolov8n.so",
    "YoloV9S": "libpostprocess_yolov9s.so",
    "YoloXS": "libpostprocess_yoloxs.so",
    "YOLOV11N": "libpostprocess_yolov11.so",
    "yolov8m_pose": "libpostprocess_yolov8m_pose.so",
}


# ---------------------------------------------------------------------------
# Data types
# ---------------------------------------------------------------------------

class Severity(Enum):
    ERROR = "ERROR"
    WARNING = "WARNING"
    INFO = "INFO"


@dataclass
class Finding:
    severity: Severity
    check: str
    message: str
    line: Optional[int] = None

    def __str__(self):
        loc = f" (line {self.line})" if self.line else ""
        return f"[{self.severity.value}] {self.check}{loc}: {self.message}"


@dataclass
class ValidationResult:
    script_path: str
    findings: List[Finding] = field(default_factory=list)

    @property
    def errors(self) -> List[Finding]:
        return [f for f in self.findings if f.severity == Severity.ERROR]

    @property
    def warnings(self) -> List[Finding]:
        return [f for f in self.findings if f.severity == Severity.WARNING]

    @property
    def passed(self) -> bool:
        return len(self.errors) == 0

    def add(self, severity: Severity, check: str, message: str,
            line: Optional[int] = None):
        self.findings.append(Finding(severity, check, message, line))

    def summary(self) -> str:
        total = len(self.findings)
        errs = len(self.errors)
        warns = len(self.warnings)
        status = "PASS" if self.passed else "FAIL"
        return (
            f"\n{'=' * 60}\n"
            f"  Validation: {status}  |  "
            f"{errs} error(s), {warns} warning(s), {total} total finding(s)\n"
            f"  Script: {self.script_path}\n"
            f"{'=' * 60}"
        )


# ---------------------------------------------------------------------------
# Utility: extract pipeline string from script content
# ---------------------------------------------------------------------------

def extract_pipeline_strings(content: str) -> List[str]:
    """Extract gst-launch-1.0 pipeline strings from shell script content."""
    pipelines = []
    # Match multiline gst-launch-1.0 commands (backslash-continued lines)
    pattern = re.compile(
        r'gst-launch-1\.0\s+(.*?)(?:\n[^\\\n]|\Z)',
        re.DOTALL
    )
    for match in pattern.finditer(content):
        raw = match.group(1)
        # Join backslash-continued lines
        joined = re.sub(r'\\\s*\n\s*', ' ', raw)
        # Remove shell comments
        joined = re.sub(r'#[^\n]*', '', joined)
        pipelines.append(joined.strip())

    # Also extract pipeline_str = "..." assignments in Python
    py_pattern = re.compile(
        r'(?:pipeline_str|pipeline|PIPELINE)\s*=\s*["\'](.+?)["\']',
        re.DOTALL
    )
    for match in py_pattern.finditer(content):
        pipelines.append(match.group(1).strip())

    # Also extract f-string or multi-line string pipeline definitions
    fstr_pattern = re.compile(
        r'(?:pipeline_str|pipeline|PIPELINE)\s*=\s*f?"""(.+?)"""',
        re.DOTALL
    )
    for match in fstr_pattern.finditer(content):
        pipelines.append(match.group(1).strip())

    return pipelines


def extract_element_props(pipeline_str: str, element_name: str) -> List[dict]:
    """Extract properties for a named element from a pipeline string."""
    # Match element name followed by key=value pairs
    pattern = rf'{element_name}\s+((?:\S+=\S+\s*)*)'
    matches = re.findall(pattern, pipeline_str, re.IGNORECASE)
    results = []
    for match in matches:
        props = dict(re.findall(r'(\S+)=(\S+)', match))
        results.append(props)
    return results


# ---------------------------------------------------------------------------
# Check 1: Script Syntax
# ---------------------------------------------------------------------------

def check_syntax(script_path: str, result: ValidationResult):
    """Check script syntax (bash -n or python -m py_compile)."""
    ext = os.path.splitext(script_path)[1]

    if ext == ".sh" or script_path.endswith(".sh"):
        proc = subprocess.run(
            ["bash", "-n", script_path],
            capture_output=True, text=True
        )
        if proc.returncode != 0:
            result.add(Severity.ERROR, "syntax",
                       f"Bash syntax error: {proc.stderr.strip()}")
        else:
            result.add(Severity.INFO, "syntax", "Shell syntax OK")

    elif ext == ".py":
        proc = subprocess.run(
            [sys.executable, "-m", "py_compile", script_path],
            capture_output=True, text=True
        )
        if proc.returncode != 0:
            result.add(Severity.ERROR, "syntax",
                       f"Python syntax error: {proc.stderr.strip()}")
        else:
            result.add(Severity.INFO, "syntax", "Python syntax OK")

    else:
        result.add(Severity.WARNING, "syntax",
                   f"Unknown script type: {ext}")


# ---------------------------------------------------------------------------
# Check 2: Preprocess-ID / Inference-ID Consistency
# ---------------------------------------------------------------------------

def check_id_matching(pipeline_str: str, result: ValidationResult):
    """Verify preprocess-id and inference-id consistency across elements."""
    preprocess_elems = extract_element_props(pipeline_str, "dxpreprocess")
    infer_elems = extract_element_props(pipeline_str, "dxinfer")
    postprocess_elems = extract_element_props(pipeline_str, "dxpostprocess")

    if not infer_elems:
        # May use config-file-path mode — warn but don't error
        if "dxinfer" in pipeline_str.lower():
            result.add(Severity.WARNING, "id-match",
                       "DxInfer found but no inline properties — "
                       "may be using config-file-path mode")
        return

    # Check preprocess-id matching: DxPreprocess → DxInfer
    for pp in preprocess_elems:
        pp_id = pp.get("preprocess-id", "0")
        matching = [i for i in infer_elems
                    if i.get("preprocess-id", "0") == pp_id]
        if not matching:
            result.add(Severity.ERROR, "id-match",
                       f"DxPreprocess preprocess-id={pp_id} has no "
                       f"matching DxInfer")

    # Check inference-id matching: DxInfer → DxPostprocess
    for inf in infer_elems:
        inf_id = inf.get("inference-id", "0")
        matching = [p for p in postprocess_elems
                    if p.get("inference-id", "0") == inf_id]
        if not matching:
            result.add(Severity.ERROR, "id-match",
                       f"DxInfer inference-id={inf_id} has no "
                       f"matching DxPostprocess")

    # Reverse check: DxInfer preprocess-id should have a DxPreprocess
    for inf in infer_elems:
        pp_id = inf.get("preprocess-id", "0")
        matching = [p for p in preprocess_elems
                    if p.get("preprocess-id", "0") == pp_id]
        if not matching:
            result.add(Severity.WARNING, "id-match",
                       f"DxInfer preprocess-id={pp_id} has no "
                       f"matching DxPreprocess (may use config-file-path)")

    if not result.errors:
        result.add(Severity.INFO, "id-match",
                   "Preprocess-ID / Inference-ID consistency OK")


# ---------------------------------------------------------------------------
# Check 3: Queue Placement
# ---------------------------------------------------------------------------

def check_queue_placement(pipeline_str: str, result: ValidationResult):
    """Verify queue elements between consecutive dx_stream elements."""
    # Split by '!' to get element chain
    parts = [p.strip() for p in pipeline_str.split("!")]
    dx_lower = [e.lower() for e in DX_ELEMENTS]

    violations = []
    for i in range(len(parts) - 1):
        current = parts[i].split()[0].lower() if parts[i].split() else ""
        next_elem = parts[i + 1].split()[0].lower() if parts[i + 1].split() else ""

        if current in dx_lower and next_elem in dx_lower:
            violations.append((current, next_elem))

    for cur, nxt in violations:
        result.add(Severity.ERROR, "queue-placement",
                   f"Missing queue between {cur} and {nxt}")

    if not violations:
        result.add(Severity.INFO, "queue-placement",
                   "Queue placement OK")


# ---------------------------------------------------------------------------
# Check 4: Absolute Path Validation
# ---------------------------------------------------------------------------

def check_absolute_paths(pipeline_str: str, result: ValidationResult):
    """Check that model-path and library-file-path are absolute or variables."""
    infer_elems = extract_element_props(pipeline_str, "dxinfer")
    for inf in infer_elems:
        model_path = inf.get("model-path", "")
        if model_path:
            # Skip variable references ($VAR, ${VAR})
            if model_path.startswith("$"):
                result.add(Severity.INFO, "abs-path",
                           f"model-path uses variable: {model_path}")
            elif not os.path.isabs(model_path):
                result.add(Severity.ERROR, "abs-path",
                           f"DxInfer model-path is not absolute: {model_path}")

    postprocess_elems = extract_element_props(pipeline_str, "dxpostprocess")
    for pp in postprocess_elems:
        lib_path = pp.get("library-file-path", "")
        if lib_path:
            if lib_path.startswith("$"):
                result.add(Severity.INFO, "abs-path",
                           f"library-file-path uses variable: {lib_path}")
            elif not os.path.isabs(lib_path):
                result.add(Severity.ERROR, "abs-path",
                           f"DxPostprocess library-file-path is not absolute: "
                           f"{lib_path}")


# ---------------------------------------------------------------------------
# Check 5: Model Path Validation
# ---------------------------------------------------------------------------

def check_model_paths(pipeline_str: str, content: str,
                      model_dir: Optional[str], result: ValidationResult):
    """Validate model paths against the known model list and filesystem."""
    # Extract model file references from content (both inline and variable)
    model_refs = re.findall(r'(\w+\.dxnn)', content)
    if not model_refs:
        result.add(Severity.INFO, "model-path", "No .dxnn references found")
        return

    for model_name in set(model_refs):
        # Check against known model list
        if model_name not in KNOWN_MODELS:
            result.add(Severity.WARNING, "model-path",
                       f"Model '{model_name}' not in model_list.json registry")

        # Check filesystem existence if model_dir given
        if model_dir:
            model_file = os.path.join(model_dir, model_name)
            if not os.path.isfile(model_file):
                result.add(Severity.WARNING, "model-path",
                           f"Model file not found: {model_file}")

    # Check model-to-postprocess consistency
    infer_elems = extract_element_props(pipeline_str, "dxinfer")
    postprocess_elems = extract_element_props(pipeline_str, "dxpostprocess")

    for inf in infer_elems:
        model_path = inf.get("model-path", "")
        if not model_path:
            continue

        # Extract model base name
        model_basename = os.path.basename(model_path).replace(".dxnn", "")
        # Handle variable substitution patterns
        model_basename = re.sub(r'\$\{?\w+\}?/?', '', model_basename)
        if not model_basename:
            continue

        expected_lib = POSTPROCESS_MAP.get(model_basename)
        if not expected_lib:
            continue

        # Try to find matching postprocess element
        inf_id = inf.get("inference-id", "0")
        matching_pp = [p for p in postprocess_elems
                       if p.get("inference-id", "0") == inf_id]
        for pp in matching_pp:
            lib_path = pp.get("library-file-path", "")
            if lib_path and expected_lib not in lib_path:
                result.add(Severity.WARNING, "model-postprocess",
                           f"Model '{model_basename}' expects "
                           f"'{expected_lib}' but postprocess uses "
                           f"'{os.path.basename(lib_path)}'")


# ---------------------------------------------------------------------------
# Check 6: Required Files
# ---------------------------------------------------------------------------

def check_required_files(script_path: str, content: str,
                         result: ValidationResult):
    """Check that referenced config files exist relative to the script."""
    script_dir = os.path.dirname(os.path.abspath(script_path))

    # Find config-file-path references
    config_refs = re.findall(r'config-file-path=(\S+)', content)
    for ref in config_refs:
        # Skip variable references
        if ref.startswith("$"):
            continue
        if not os.path.isabs(ref):
            ref = os.path.join(script_dir, ref)
        if not os.path.isfile(ref):
            result.add(Severity.WARNING, "required-files",
                       f"Config file not found: {ref}")

    # Find broker config references
    broker_configs = re.findall(r'config=(\S+)', content)
    for ref in broker_configs:
        if ref.startswith("$"):
            continue
        if not os.path.isabs(ref):
            ref = os.path.join(script_dir, ref)
        if not os.path.isfile(ref):
            result.add(Severity.WARNING, "required-files",
                       f"Broker config not found: {ref}")


# ---------------------------------------------------------------------------
# Check 7: RTSP-Specific Checks
# ---------------------------------------------------------------------------

def check_rtsp_patterns(pipeline_str: str, content: str,
                        result: ValidationResult):
    """Check RTSP-specific requirements (DxRate, DxInputSelector)."""
    has_rtsp = "rtsp://" in content.lower() or "rtspsrc" in content.lower()
    if not has_rtsp:
        return

    has_dxrate = "dxrate" in pipeline_str.lower()
    if not has_dxrate:
        result.add(Severity.WARNING, "rtsp",
                   "RTSP source detected but no DxRate element — "
                   "frames may accumulate and cause OOM")

    # Check for DxInputSelector/DxOutputSelector pairing
    has_input_sel = "dxinputselector" in pipeline_str.lower()
    has_output_sel = "dxoutputselector" in pipeline_str.lower()
    if has_input_sel and not has_output_sel:
        result.add(Severity.ERROR, "rtsp",
                   "DxInputSelector found without matching DxOutputSelector")
    if has_output_sel and not has_input_sel:
        result.add(Severity.ERROR, "rtsp",
                   "DxOutputSelector found without matching DxInputSelector")


# ---------------------------------------------------------------------------
# Check 8: Broker Pipeline Checks
# ---------------------------------------------------------------------------

def check_broker_patterns(pipeline_str: str, result: ValidationResult):
    """Check broker pipeline requirements (DxMsgConv before DxMsgBroker)."""
    has_broker = "dxmsgbroker" in pipeline_str.lower()
    if not has_broker:
        return

    has_msgconv = "dxmsgconv" in pipeline_str.lower()
    if not has_msgconv:
        result.add(Severity.ERROR, "broker",
                   "DxMsgBroker found without DxMsgConv — "
                   "metadata must be serialized before publishing")

    # Check ordering: DxMsgConv should appear before DxMsgBroker in pipeline
    parts = [p.strip().split()[0].lower() for p in pipeline_str.split("!")
             if p.strip()]
    conv_idx = None
    broker_idx = None
    for i, part in enumerate(parts):
        if part == "dxmsgconv" and conv_idx is None:
            conv_idx = i
        if part == "dxmsgbroker" and broker_idx is None:
            broker_idx = i

    if conv_idx is not None and broker_idx is not None:
        if conv_idx > broker_idx:
            result.add(Severity.ERROR, "broker",
                       "DxMsgConv must appear BEFORE DxMsgBroker in pipeline")

    # Check broker-name property
    broker_elems = extract_element_props(pipeline_str, "dxmsgbroker")
    for br in broker_elems:
        broker_name = br.get("broker-name", "")
        if broker_name and broker_name not in ("kafka", "mqtt"):
            result.add(Severity.ERROR, "broker",
                       f"Invalid broker-name: '{broker_name}' "
                       f"(must be 'kafka' or 'mqtt')")


# ---------------------------------------------------------------------------
# Check 9: Element Order Validation
# ---------------------------------------------------------------------------

def check_element_ordering(pipeline_str: str, result: ValidationResult):
    """Check for known invalid element orderings."""
    parts = [p.strip().split()[0].lower() for p in pipeline_str.split("!")
             if p.strip()]

    invalid_sequences = [
        ("dxinfer", "dxtracker",
         "DxTracker must follow DxPostprocess, not DxInfer"),
        ("dxpostprocess", "dxmsgbroker",
         "DxMsgBroker must follow DxMsgConv, not DxPostprocess directly"),
        ("dxinfer", "dxosd",
         "DxOsd must follow DxPostprocess, not DxInfer"),
    ]

    for i in range(len(parts) - 1):
        for bad_first, bad_second, msg in invalid_sequences:
            # Allow queue between them
            if parts[i] == bad_first:
                # Look ahead past queues
                j = i + 1
                while j < len(parts) and parts[j] == "queue":
                    j += 1
                if j < len(parts) and parts[j] == bad_second:
                    result.add(Severity.ERROR, "element-order", msg)


# ---------------------------------------------------------------------------
# Check 10: Headless Mode Check
# ---------------------------------------------------------------------------

def check_headless_support(content: str, result: ValidationResult):
    """Check if script handles headless mode (no DISPLAY)."""
    has_display_sink = any(
        sink in content.lower()
        for sink in ["fpsdisplaysink", "ximagesink", "xvimagesink",
                     "autovideosink", "glimagesink"]
    )
    if not has_display_sink:
        return

    has_display_check = (
        "$DISPLAY" in content or
        "${DISPLAY" in content or
        "DISPLAY" in content and ("fakesink" in content or "headless" in content.lower())
    )
    if not has_display_check:
        result.add(Severity.WARNING, "headless",
                   "Script uses video sink but does not check DISPLAY — "
                   "will fail on headless systems")


# ---------------------------------------------------------------------------
# Check 11: Smoke Test (optional, requires NPU)
# ---------------------------------------------------------------------------

def run_smoke_test(pipeline_str: str, result: ValidationResult,
                   timeout_sec: int = 30):
    """Run pipeline with videotestsrc for a brief smoke test."""
    # Replace source with videotestsrc
    smoke_pipeline = re.sub(
        r'(urisourcebin|filesrc|v4l2src|rtspsrc)\s+\S+',
        'videotestsrc num-buffers=5 ! video/x-raw,width=640,height=640,format=RGB',
        pipeline_str,
        count=1
    )
    # Replace display sink with fakesink
    smoke_pipeline = re.sub(
        r'(fpsdisplaysink|ximagesink|xvimagesink|autovideosink|glimagesink)\s*\S*',
        'fakesink sync=false',
        smoke_pipeline
    )

    cmd = f"gst-launch-1.0 {smoke_pipeline}"
    try:
        proc = subprocess.run(
            cmd, shell=True, capture_output=True, text=True,
            timeout=timeout_sec
        )
        if proc.returncode == 0:
            result.add(Severity.INFO, "smoke-test",
                       "Smoke test passed (videotestsrc → fakesink)")
        else:
            stderr_short = proc.stderr.strip()[:200]
            result.add(Severity.ERROR, "smoke-test",
                       f"Smoke test failed (exit {proc.returncode}): "
                       f"{stderr_short}")
    except subprocess.TimeoutExpired:
        result.add(Severity.ERROR, "smoke-test",
                   f"Smoke test timed out after {timeout_sec}s")
    except FileNotFoundError:
        result.add(Severity.WARNING, "smoke-test",
                   "gst-launch-1.0 not found — smoke test skipped")


# ---------------------------------------------------------------------------
# Main validation runner
# ---------------------------------------------------------------------------

def validate(script_path: str, smoke_test: bool = False,
             model_dir: Optional[str] = None) -> ValidationResult:
    """Run all validation checks on a pipeline script."""
    result = ValidationResult(script_path=script_path)

    if not os.path.isfile(script_path):
        result.add(Severity.ERROR, "file", f"Script not found: {script_path}")
        return result

    content = Path(script_path).read_text(encoding="utf-8", errors="replace")

    # Check 1: Syntax
    check_syntax(script_path, result)

    # Extract pipeline strings for element-level checks
    pipelines = extract_pipeline_strings(content)
    if not pipelines:
        result.add(Severity.WARNING, "pipeline-extract",
                   "Could not extract pipeline string from script — "
                   "skipping element-level checks")
        return result

    for idx, pipeline_str in enumerate(pipelines):
        if len(pipelines) > 1:
            result.add(Severity.INFO, "pipeline-extract",
                       f"Checking pipeline {idx + 1}/{len(pipelines)}")

        # Check 2: ID consistency
        check_id_matching(pipeline_str, result)

        # Check 3: Queue placement
        check_queue_placement(pipeline_str, result)

        # Check 4: Absolute paths
        check_absolute_paths(pipeline_str, result)

        # Check 5: Model path validation
        check_model_paths(pipeline_str, content, model_dir, result)

        # Check 7: RTSP patterns
        check_rtsp_patterns(pipeline_str, content, result)

        # Check 8: Broker patterns
        check_broker_patterns(pipeline_str, result)

        # Check 9: Element ordering
        check_element_ordering(pipeline_str, result)

        # Check 11: Smoke test (first pipeline only)
        if smoke_test and idx == 0:
            run_smoke_test(pipeline_str, result)

    # Check 6: Required files (operates on full content)
    check_required_files(script_path, content, result)

    # Check 10: Headless support
    check_headless_support(content, result)

    return result


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def parse_args():
    parser = argparse.ArgumentParser(
        description="Validate a dx_stream pipeline application"
    )
    parser.add_argument(
        "script", help="Path to the pipeline script (.sh or .py)"
    )
    parser.add_argument(
        "--smoke-test", action="store_true",
        help="Run a brief smoke test with videotestsrc (requires NPU)"
    )
    parser.add_argument(
        "--model-dir",
        help="Directory containing .dxnn model files for existence checks"
    )
    parser.add_argument(
        "--json", action="store_true",
        help="Output results as JSON"
    )
    return parser.parse_args()


def main():
    args = parse_args()

    result = validate(
        script_path=args.script,
        smoke_test=args.smoke_test,
        model_dir=args.model_dir,
    )

    if args.json:
        output = {
            "script": result.script_path,
            "passed": result.passed,
            "errors": len(result.errors),
            "warnings": len(result.warnings),
            "findings": [
                {
                    "severity": f.severity.value,
                    "check": f.check,
                    "message": f.message,
                    "line": f.line,
                }
                for f in result.findings
            ],
        }
        print(json.dumps(output, indent=2))
    else:
        for finding in result.findings:
            print(finding)
        print(result.summary())

    sys.exit(0 if result.passed else 1)


if __name__ == "__main__":
    main()
