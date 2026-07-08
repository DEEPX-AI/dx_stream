"""Environment fingerprinting – capture host, NPU, and tool versions.

Produces a dict that uniquely identifies the measurement environment
so results from different machines are traceable and comparable.
"""

import json
import platform
import re
import shutil
import subprocess
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


def _run(cmd: list[str], default: str = "unknown") -> str:
    """Run a command and return stripped stdout, or *default* on failure."""
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
        return r.stdout.strip() or default
    except (FileNotFoundError, subprocess.TimeoutExpired, OSError):
        return default


def _tool_version(name: str) -> dict[str, Any]:
    """Return {'path': ..., 'version': ...} for a CLI tool."""
    path = shutil.which(name)
    if not path:
        return {"path": None, "version": None, "available": False}
    ver = "unknown"
    if name == "ffprobe":
        ver = _run([name, "-version"]).split("\n")[0]
    elif name in ("gst-launch-1.0", "gst-inspect-1.0"):
        ver = _run([name, "--version"]).split("\n")[0]
    elif name == "dxtop":
        # dxtop has no --version; extract from runtime header instead
        ver = _run([name, "--help"]).split("\n")[0]
    elif name == "run_model":
        # Extract only the first line (version string)
        ver = _run([name, "--help"]).split("\n")[0]
    return {"path": path, "version": ver, "available": True}


REQUIRED_TOOLS = ["run_model", "gst-launch-1.0", "gst-inspect-1.0"]
OPTIONAL_TOOLS = ["dxtop", "ffprobe"]


def _get_dx_stream_version() -> str:
    """Get installed dxstream plugin version via gst-inspect-1.0."""
    if not shutil.which("gst-inspect-1.0"):
        return "unknown"
    raw = _run(["gst-inspect-1.0", "dxstream"])
    m = re.search(r"Version\s+(\S+)", raw)
    return m.group(1) if m else "unknown"


def _summarize_npu_topology(device_boards: list[str | None], device_count: int) -> dict[str, Any]:
    """Build a compact hardware-topology summary from per-device board types."""
    normalized = [(board or "").upper() for board in device_boards]
    from .result_layout import _format_hw_config
    h1_chips = sum(1 for board in normalized if "H1" in board)
    h1_cards = (h1_chips + 3) // 4 if h1_chips > 0 else 0
    m1_modules = max(0, device_count - h1_chips)
    hw_config = _format_hw_config(h1_cards, m1_modules)
    return {
        "h1_cards": h1_cards,
        "m1_modules": m1_modules,
        "hw_config": hw_config,
        "sku": hw_config,
    }


def collect_fingerprint() -> dict[str, Any]:
    """Collect full environment fingerprint."""
    uname = platform.uname()
    fp: dict[str, Any] = {
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "host": {
            "hostname": uname.node,
            "os": _get_os_pretty_name(),
            "kernel": uname.release,
            "arch": uname.machine,
            "cpu": _get_cpu_model(),
            "cpu_count": platform.os.cpu_count(),  # type: ignore[attr-defined]
            "ram_gb": _get_ram_gb(),
        },
        "npu": _get_npu_info(),
        "software": {
            "dx_stream": _get_dx_stream_version(),
        },
        "tools": {},
    }

    # Required tools
    missing = []
    for tool in REQUIRED_TOOLS:
        info = _tool_version(tool)
        fp["tools"][tool] = info
        if not info["available"]:
            missing.append(tool)

    # Optional tools
    for tool in OPTIONAL_TOOLS:
        fp["tools"][tool] = _tool_version(tool)

    fp["missing_required"] = missing
    return fp


def _get_os_pretty_name() -> str:
    """Get distro name from /etc/os-release (e.g. 'Ubuntu 24.04.2 LTS')."""
    try:
        with open("/etc/os-release") as f:
            for line in f:
                if line.startswith("PRETTY_NAME="):
                    return line.split("=", 1)[1].strip().strip('"')
    except OSError:
        pass
    return f"{platform.system()} {platform.release()}"


def _get_dxrt_version() -> str:
    """Get DXRT runtime version from dxrt-cli --version."""
    if not shutil.which("dxrt-cli"):
        return "unknown"
    ver = _run(["dxrt-cli", "--version"])
    if ver and ver != "unknown":
        first_line = ver.split("\n")[0].strip()
        # Strip "DXRT " prefix: "DXRT v3.2.0" -> "v3.2.0"
        if first_line.upper().startswith("DXRT "):
            first_line = first_line[5:].strip()
        return first_line
    return "unknown"


def _get_ram_gb() -> float:
    """Get total RAM in GB from /proc/meminfo."""
    try:
        with open("/proc/meminfo") as f:
            for line in f:
                if line.startswith("MemTotal:"):
                    kb = int(line.split()[1])
                    return round(kb / 1024 / 1024, 1)
    except OSError:
        pass
    return 0.0


def _get_cpu_model() -> str:
    """Get CPU model name from lscpu."""
    if not shutil.which("lscpu"):
        return platform.processor() or "unknown"
    raw = _run(["lscpu"])
    for line in raw.split("\n"):
        if "Model name" in line:
            m = re.search(r":\s*(.+)", line)
            if m:
                return m.group(1).strip()
    return raw.split("\n")[0] if raw else "unknown"


def _get_npu_info() -> dict[str, Any]:
    """Parse NPU information from dxrt-cli -s output."""
    info: dict[str, Any] = {
        "sku": "unknown",
        "rt_version": _get_dxrt_version(),
        "driver": "unknown",
        "pcie_driver": "unknown",
        "firmware": "unknown",
        "memory": "unknown",
        "board": "unknown",
        "pcie": "unknown",
        "cores": [],
    }
    if not shutil.which("dxrt-cli"):
        return info

    raw = _run(["dxrt-cli", "-s"])
    if raw == "unknown":
        return info
    info["raw"] = raw

    # Parse structured fields from dxrt-cli -s output
    device_count = 0
    device_boards: list[str | None] = []   # board type per device
    current_board: str | None = None
    for line in raw.split("\n"):
        line = line.strip()
        if line.startswith("* Device"):
            # " * Device 0: M1, Accelerator type"
            if device_count > 0:
                device_boards.append(current_board)
            current_board = None
            device_count += 1
        elif "Board" in line and ":" in line and "Chip" not in line:
            m = re.search(r":\s*(.+)", line)
            if m:
                val = m.group(1).strip()
                current_board = val.split(",")[0].strip()
                info["board"] = val
        elif "RT Driver version" in line:
            m = re.search(r":\s*(.+)", line)
            if m:
                info["driver"] = m.group(1).strip()
        elif "PCIe Driver version" in line:
            m = re.search(r":\s*(.+)", line)
            if m:
                info["pcie_driver"] = m.group(1).strip()
        elif "FW version" in line:
            m = re.search(r":\s*(.+)", line)
            if m:
                info["firmware"] = m.group(1).strip()
        elif "Memory" in line and ":" in line:
            m = re.search(r":\s*(.+)", line)
            if m:
                info["memory"] = m.group(1).strip()
        elif "PCIe" in line and "Gen" in line:
            m = re.search(r":\s*(.+)", line)
            if m:
                info["pcie"] = m.group(1).strip()
        elif line.startswith("NPU"):
            # "NPU 0: voltage 750 mV, clock 1000 MHz, temperature 38'C"
            info["cores"].append(line)
    # Append last device's board
    if device_count > 0:
        device_boards.append(current_board)

    info["device_count"] = device_count
    info.update(_summarize_npu_topology(device_boards, device_count))

    # Extract max NPU clock from core info
    clock_mhz: int | None = None
    for core_line in info["cores"]:
        cm = re.search(r"clock\s+(\d+)\s*MHz", core_line)
        if cm:
            val = int(cm.group(1))
            if clock_mhz is None or val > clock_mhz:
                clock_mhz = val
    info["clock_mhz"] = clock_mhz

    return info


def check_preflight(fingerprint: dict) -> tuple[bool, list[str]]:
    """Validate that all required tools are present.

    Returns (ok, list_of_error_messages).
    """
    errors = []
    for tool in fingerprint.get("missing_required", []):
        errors.append(f"Required tool not found: {tool}")
    return len(errors) == 0, errors


def get_video_info(video_path: str | Path) -> dict[str, Any]:
    """Get video metadata using ffprobe."""
    info: dict[str, Any] = {
        "path": str(video_path),
        "filename": Path(video_path).name,
    }
    if not shutil.which("ffprobe") or not Path(video_path).exists():
        return info
    try:
        r = subprocess.run(
            ["ffprobe", "-v", "quiet", "-print_format", "json",
             "-show_streams", "-show_format", str(video_path)],
            capture_output=True, text=True, timeout=10,
        )
        import json as _json
        data = _json.loads(r.stdout)
        for s in data.get("streams", []):
            if s.get("codec_type") == "video":
                info["codec"] = s.get("codec_name", "unknown")
                info["width"] = s.get("width", 0)
                info["height"] = s.get("height", 0)
                info["pix_fmt"] = s.get("pix_fmt", "unknown")
                fps_str = s.get("r_frame_rate", "0/1")
                try:
                    num, den = fps_str.split("/")
                    info["fps"] = round(int(num) / int(den), 1)
                except (ValueError, ZeroDivisionError):
                    info["fps"] = fps_str
                info["nb_frames"] = int(s.get("nb_frames", 0))
                info["duration_sec"] = float(s.get("duration", 0))
                break
        fmt = data.get("format", {})
        info["format"] = fmt.get("format_long_name", "unknown")
        bitrate = fmt.get("bit_rate")
        if bitrate:
            info["bitrate_mbps"] = round(int(bitrate) / 1_000_000, 2)
    except (subprocess.TimeoutExpired, OSError, ValueError):
        pass
    return info


def save_fingerprint(fingerprint: dict, output_dir: Path) -> Path:
    """Save fingerprint as JSON and return the path."""
    output_dir.mkdir(parents=True, exist_ok=True)
    path = output_dir / "environment.json"
    with open(path, "w") as f:
        json.dump(fingerprint, f, indent=2)
    return path


def _strip_ansi(text: str) -> str:
    """Remove ANSI/VT100 escape sequences from *text*."""
    return re.sub(r"\x1b\[[0-9;]*[A-Za-z]", "", text)


def collect_model_metadata(model_path: str | Path) -> dict[str, Any]:
    """Run ``parse_model -m <model_path>`` and extract key metadata.

    Returns a dict with ``format_version``, ``dxcom_version``, and
    ``total_memory_bytes`` (or empty dict on failure).
    """
    raw = _run(["parse_model", "-m", str(model_path)], default="")
    if not raw:
        return {}

    # parse_model always emits ANSI colour codes regardless of TTY; strip them
    # before applying regex patterns so captured values are clean strings.
    raw = _strip_ansi(raw)

    result: dict[str, Any] = {}

    # .dxnn Format Version
    m = re.search(r"\.dxnn Format Version\s*:\s*(\S+)", raw)
    if m:
        result["format_version"] = m.group(1)

    # DX-COM Version
    m = re.search(r"DX-COM Version\s*:\s*(\S+)", raw)
    if m:
        result["dxcom_version"] = m.group(1)

    # Total memory MB from "102.49 MB (107,468,288 bytes)"
    m = re.search(r"Total\s*:\s*[\d.]+ \S+\s*\(([\d,]+)\s*bytes\)", raw)
    if m:
        result["total_memory_mb"] = round(int(m.group(1).replace(",", "")) / (1024 * 1024), 2)

    return result
