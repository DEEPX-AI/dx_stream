"""E2E pipeline benchmark runners (single-stream and multi-stream).

Uses gst-launch-1.0 to run full inference pipelines with:
  - Explicit use-ort setting (always specified, never rely on default)
  - Decoder detection from pipeline logs
  - FPS from "Execution ended after" + frame count
  - CPU% and RSS from GNU time
  - NPU stats from dxtop
"""

import logging
import os
import re
import shutil
import signal
import statistics
import subprocess
import time
from dataclasses import dataclass, field
from datetime import datetime
from pathlib import Path
from typing import Callable, Optional

from .config import (
    BenchmarkConfig, ROOT_DIR,
    get_task_preprocess, get_task_inference, get_postprocess_config_path,
)
from .model_catalog import ModelEntry
from .npu_monitor import NpuMonitor, NpuStats, read_npu_temp_c, read_npu_clock_mhz
from .npu_stats_util import merge_npu_stats as _merge_npu_stats


@dataclass
class PipelineResult:
    """Result of one E2E pipeline benchmark condition."""
    model: str
    task: str
    size: str
    use_ort: bool
    video: str
    stream_count: int = 1
    frame_count: int = 0
    runs: int = 1
    requested_runs: int = 1
    timeout_runs: int = 0
    decoder: str = "unknown"
    avg_time_sec: float = 0.0
    avg_e2e_fps: float = 0.0
    fps_std: Optional[float] = None
    avg_per_channel_fps: float = 0.0
    avg_cpu_pct: float = 0.0
    max_rss_mib: float = 0.0
    npu_stats: Optional[dict] = None
    pipeline_caps: Optional[dict] = None
    source: Optional[str] = None  # "single_stream" when injected from single-stream result
    status: str = "ok"
    reason: str = ""
    raw_logs: list = field(default_factory=list)

    def as_dict(self) -> dict:
        d = {
            "model": self.model,
            "task": self.task,
            "size": self.size,
            "use_ort": self.use_ort,
            "video": os.path.basename(self.video),
            "stream_count": self.stream_count,
            "frame_count": self.frame_count,
            "runs": self.runs,
            "requested_runs": self.requested_runs,
            "timeout_runs": self.timeout_runs,
            "decoder": self.decoder,
            "avg_time_sec": round(self.avg_time_sec, 3),
            "avg_e2e_fps": round(self.avg_e2e_fps, 2),
            "fps_std": round(self.fps_std, 2) if self.fps_std is not None else None,
            "avg_per_channel_fps": round(self.avg_per_channel_fps, 2),
            "avg_cpu_pct": round(self.avg_cpu_pct, 1),
            "max_rss_mib": round(self.max_rss_mib, 1),
            "status": self.status,
            "reason": self.reason,
        }
        if self.source:
            d["source"] = self.source
        if self.npu_stats:
            d.update(self.npu_stats)
        if self.pipeline_caps:
            d["pipeline_caps"] = self.pipeline_caps
        return d


# ── Helpers ────────────────────────────────────────────────────────────────

def _get_frame_count(video_path: str) -> int:
    """Count frames via ffprobe."""
    if not shutil.which("ffprobe"):
        return 0
    try:
        r = subprocess.run(
            ["ffprobe", "-v", "error", "-count_frames", "-select_streams", "v:0",
             "-show_entries", "stream=nb_read_frames",
             "-of", "default=nokey=1:noprint_wrappers=1", video_path],
            capture_output=True, text=True, timeout=60,
        )
        return int(r.stdout.strip().split("\n")[-1])
    except (ValueError, subprocess.TimeoutExpired, OSError):
        return 0


def _hms_to_sec(hms: str) -> float:
    """Convert H:MM:SS.nnn to seconds."""
    parts = hms.split(":")
    if len(parts) == 3:
        return float(parts[0]) * 3600 + float(parts[1]) * 60 + float(parts[2])
    return 0.0


def _parse_execution_time(log: str) -> Optional[float]:
    """Extract seconds from 'Execution ended after H:MM:SS.nnn'."""
    m = re.search(r"Execution ended after\s+(\d+:\d+:\d+[\d.]*)", log)
    if m:
        return _hms_to_sec(m.group(1))
    return None


def _parse_cpu_pct(log: str) -> Optional[float]:
    m = re.search(r"__CPU_PERCENT__=(\d+)%", log)
    return float(m.group(1)) if m else None


def _parse_max_rss_kb(log: str) -> Optional[int]:
    m = re.search(r"__MAX_RSS_KB__=(\d+)", log)
    return int(m.group(1)) if m else None


def _detect_decoder(log: str) -> str:
    """Extract video decoder from pipeline log."""
    # Try instance pattern first: GstTypeDec:name0
    instances = re.findall(r"Gst[A-Za-z0-9_]*Dec[A-Za-z0-9_]*:([A-Za-z0-9_]+)", log)
    for inst in instances:
        # Strip trailing digits to get element name
        elem = re.sub(r"\d+$", "", inst)
        if _is_video_decoder(elem):
            return elem

    # Fallback: look for *dec* patterns
    candidates = re.findall(r"[A-Za-z0-9_]+dec[A-Za-z0-9_]*", log, re.IGNORECASE)
    for c in candidates:
        if _is_video_decoder(c):
            return c

    return "unknown"


def _is_video_decoder(element: str) -> bool:
    """Check if a GStreamer element is a video decoder."""
    if not shutil.which("gst-inspect-1.0"):
        return False
    try:
        r = subprocess.run(
            ["gst-inspect-1.0", element],
            capture_output=True, text=True, timeout=5,
        )
        return "Decoder" in r.stdout and "Video" in r.stdout
    except (subprocess.TimeoutExpired, OSError):
        return False


def _parse_raw_caps(caps_str: str) -> dict:
    """Parse a GStreamer raw-video caps string into {format, memory}.

    Examples:
      "video/x-raw, format=(string)NV12, ..."          → {"format": "NV12", "memory": None}
      "video/x-raw(memory:VASurface), format=..."       → {"format": "NV12", "memory": "VASurface"}
    """
    result: dict = {"format": None, "memory": None}
    mem = re.search(r"video/x-raw\(memory:(\w+)\)", caps_str)
    if mem:
        result["memory"] = mem.group(1)
    fmt = re.search(r"format=\(string\)(\w+)", caps_str)
    if fmt:
        result["format"] = fmt.group(1)
    return result


def _is_decoder_src_path(path_segment: str) -> bool:
    """Return True when a decodebin subpath likely represents a decoder src pad."""
    last_segment = path_segment.split("/")[-1]
    factory_or_class = last_segment.split(":", 1)[0].lower()
    instance = last_segment.split(":", 1)[1].lower() if ":" in last_segment else ""

    non_decoder_tokens = ("decodebin", "queue", "capsfilter", "parse", "typefind", "postproc")
    if any(token in factory_or_class for token in non_decoder_tokens):
        return False
    if any(token in instance for token in non_decoder_tokens):
        return False

    return "dec" in factory_or_class or "dec" in instance


def _extract_pipeline_caps(log: str) -> Optional[dict]:
    """Extract decoder output and dxpreprocess input caps from pipeline -v log.

    Returns dict with:
      video_codec, decoder_src_format, decoder_src_memory,
      dxpreprocess_sink_format, dxpreprocess_sink_memory
    or None if parsing fails.
    """
    caps: dict = {}

    # Video codec from compressed caps (e.g. video/x-h264)
    codec_m = re.search(r"caps = video/x-(h26[45]|vp[89]|av1)", log)
    if codec_m:
        caps["video_codec"] = codec_m.group(1)

    # Decoder src: find the first video/x-raw src pad under decodebin that belongs
    # to an actual decoder element. This covers both Gst*Dec* class names and
    # software decoder factories such as avdec_h264 on Raspberry Pi.
    dec_src_matches = re.findall(
        r"/GstDecodeBin:decodebin\d+/(.+?)\.GstPad:src: caps = (video/x-raw[^\n]+)",
        log,
    )
    decoder_caps = None
    for path_segment, caps_str in dec_src_matches:
        if _is_decoder_src_path(path_segment):
            decoder_caps = caps_str
            break
    if decoder_caps:
        dec_caps = _parse_raw_caps(decoder_caps)
        caps["decoder_src_format"] = dec_caps["format"]
        caps["decoder_src_memory"] = dec_caps["memory"]

    # dxpreprocess sink caps
    pp_sink_matches = re.findall(
        r"GstDxPreprocess:\w+\.GstPad:sink: caps = (video/x-raw[^\n]+)", log
    )
    if pp_sink_matches:
        pp_caps = _parse_raw_caps(pp_sink_matches[0])
        caps["dxpreprocess_sink_format"] = pp_caps["format"]
        caps["dxpreprocess_sink_memory"] = pp_caps["memory"]

    # dxpreprocess backend from application message (gst-launch -m output)
    backend_m = re.search(
        r'application/x-dx-preprocess-backend.*backend=\(string\)(\w+)', log
    )
    if backend_m:
        caps["dxpreprocess_backend"] = backend_m.group(1)

    return caps if caps else None


# ── Pipeline builder ──────────────────────────────────────────────────────

def _build_single_pipeline(model_path: str, use_ort: bool, video_path: str,
                           postprocess_cfg: str,
                           preprocess: dict | None = None,
                           inference: dict | None = None) -> list[str]:
    """Build gst-launch-1.0 command for single-stream pipeline."""
    pp = preprocess or get_task_preprocess()
    inf = inference or get_task_inference()
    ort_str = "true" if use_ort else "false"
    keep_ratio_str = "true" if pp.get("keep_ratio", True) else "false"
    video_uri = Path(video_path).resolve().as_uri()

    pipeline = [
        "gst-launch-1.0", "-e", "-v", "-m",
        "urisourcebin", f"uri={video_uri}", "!", "decodebin", "!",
        "dxpreprocess",
        f'preprocess-id={pp["preprocess_id"]}',
        f'resize-width={pp["resize_width"]}',
        f'resize-height={pp["resize_height"]}',
        f'pad-value={pp["pad_value"]}',
        f'keep-ratio={keep_ratio_str}',
        "!",
        "queue", "leaky=no", "!",
        "dxinfer",
        f'preprocess-id={inf["preprocess_id"]}',
        f'inference-id={inf["inference_id"]}',
        f'model-path={model_path}',
        f'use-ort={ort_str}',
        "!",
        "queue", "leaky=no", "!",
        "dxpostprocess", f"config-file-path={postprocess_cfg}", "!",
        "queue", "leaky=no", "!",
        "fakesink", "sync=false", "async=false", "qos=false", "enable-last-sample=false",
    ]
    return pipeline


def _build_multi_pipeline(model_path: str, use_ort: bool, video_path: str,
                          postprocess_cfg: str, stream_count: int,
                          preprocess: dict | None = None,
                          inference: dict | None = None) -> list[str]:
    """Build gst-launch-1.0 command for multi-stream pipeline."""
    pp = preprocess or get_task_preprocess()
    inf = inference or get_task_inference()
    ort_str = "true" if use_ort else "false"
    keep_ratio_str = "true" if pp.get("keep_ratio", True) else "false"
    video_uri = Path(video_path).resolve().as_uri()

    src_pipes: list[str] = []
    sink_pipes: list[str] = []
    for i in range(stream_count):
        src_pipes.extend([
            "urisourcebin", f"uri={video_uri}", "!", "decodebin", "!",
            "queue", "max-size-buffers=10", "leaky=no", "!", f"in.sink_{i}",
        ])
        sink_pipes.extend([
            f"out.src_{i}", "!", "queue", "max-size-buffers=10", "leaky=no", "!",
            "fakesink", "sync=false", "async=false", "qos=false",
        ])

    pipeline = ["gst-launch-1.0", "-e", "-v", "-m"]
    pipeline.extend(src_pipes)
    pipeline.extend([
        "dxinputselector", "name=in", "!",
        "dxpreprocess",
        f'preprocess-id={pp["preprocess_id"]}',
        f'resize-width={pp["resize_width"]}',
        f'resize-height={pp["resize_height"]}',
        f'pad-value={pp["pad_value"]}',
        f'keep-ratio={keep_ratio_str}',
        "!",
        "queue", "max-size-buffers=10", "leaky=no", "!",
        "dxinfer",
        f'preprocess-id={inf["preprocess_id"]}',
        f'inference-id={inf["inference_id"]}',
        f'model-path={model_path}',
        f'use-ort={ort_str}',
        "!",
        "queue", "max-size-buffers=10", "leaky=no", "!",
        "dxpostprocess", f"config-file-path={postprocess_cfg}", "!",
        "queue", "max-size-buffers=10", "leaky=no", "!",
        "dxoutputselector", "name=out",
    ])
    pipeline.extend(sink_pipes)

    return pipeline


def _run_gst_pipeline(pipeline_parts: list[str], env_extra: dict | None = None, incident_context: str = "") -> str:
    """Execute a gst-launch-1.0 pipeline via GNU time wrapper."""
    env = os.environ.copy()
    env["GST_DEBUG_NO_COLOR"] = "1"
    env["GST_DEBUG"] = "0"  # minimal debug for clean benchmarks
    if env_extra:
        env.update(env_extra)

    full_cmd = [
        "/usr/bin/time",
        "-f",
        "__CPU_PERCENT__=%P\n__MAX_RSS_KB__=%M",
        *pipeline_parts,
    ]

    try:
        # Use start_new_session=True so the child and all its descendants form their
        # own process group (session leader).  On timeout we can then os.killpg() the
        # entire group, ensuring gst-launch grandchildren (which hold NPU device FDs)
        # are also killed – not just the direct /usr/bin/time child.
        proc = subprocess.Popen(
            full_cmd,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            text=True, env=env,
            start_new_session=True,
        )
        try:
            pgid = os.getpgid(proc.pid)
            _active_pipeline_pgids.add(pgid)
        except (ProcessLookupError, OSError):
            pgid = None
        try:
            stdout, stderr = proc.communicate(timeout=600)
            rc = proc.returncode
            sig = -rc if rc < 0 else None
            trailer = f"\n__EXIT_CODE__={rc}"
            if sig is not None:
                try:
                    sig_name = signal.Signals(-rc).name
                except ValueError:
                    sig_name = str(sig)
                trailer += f"\n__KILLED_BY_SIGNAL__={sig_name}({-rc})"
            if pgid is not None:
                _active_pipeline_pgids.discard(pgid)
            return stdout + "\n" + stderr + trailer
        except subprocess.TimeoutExpired:
            # Graceful shutdown: SIGTERM gives gst-launch a chance to release
            # NPU inference handles cleanly.  Jumping straight to SIGKILL
            # destroys dxrtd's IPC queue and bricks the NPU until service restart.
            pgid = None
            try:
                pgid = os.getpgid(proc.pid)
            except (ProcessLookupError, OSError):
                pass

            killed_hard = False
            if pgid is not None:
                # Phase 1: SIGTERM → wait up to 10s for graceful exit
                try:
                    os.killpg(pgid, signal.SIGTERM)
                except (ProcessLookupError, PermissionError):
                    pass
                try:
                    proc.communicate(timeout=10)
                except subprocess.TimeoutExpired:
                    # Phase 2: SIGKILL as last resort
                    try:
                        os.killpg(pgid, signal.SIGKILL)
                    except (ProcessLookupError, PermissionError):
                        proc.kill()
                    try:
                        proc.communicate(timeout=5)
                    except subprocess.TimeoutExpired:
                        pass
                    killed_hard = True
            else:
                proc.kill()
                try:
                    proc.communicate(timeout=5)
                except subprocess.TimeoutExpired:
                    pass
                killed_hard = True

            # SIGKILL was needed → NPU likely corrupted, must recover
            if killed_hard:
                collect_timeout_incident(incident_context or "gst_pipeline")
                cleanup_after_timeout()
            else:
                collect_timeout_incident(incident_context or "gst_pipeline.soft_timeout")

            if pgid is not None:
                _active_pipeline_pgids.discard(pgid)
            return "__TIMEOUT__"
    except OSError:
        collect_timeout_incident(incident_context or "gst_pipeline.oserror")
        return "__TIMEOUT__"


# Module-level set tracking PGIDs of pipelines started by this process.
# Used by _cleanup_orphaned_pipelines() to kill only our own children.
_active_pipeline_pgids: set[int] = set()


def _cleanup_orphaned_pipelines() -> None:
    """Kill any lingering gst-launch-1.0 processes started by this benchmark."""
    cleaned = False
    for pgid in list(_active_pipeline_pgids):
        try:
            os.killpg(pgid, signal.SIGKILL)
            cleaned = True
        except (ProcessLookupError, PermissionError, OSError):
            pass
        _active_pipeline_pgids.discard(pgid)
    if cleaned:
        time.sleep(1)  # give OS time to reclaim NPU device FDs


def recover_npu_device() -> bool:
    """Restart dxrt.service to recover NPU after a timeout-induced error.

    When a process holding NPU inference requests is SIGKILL'd, dxrtd enters
    a broken state (IPC queue destroyed, Error 43).  The only reliable recovery
    is restarting the service.

    Returns True if recovery succeeded, False otherwise.
    """
    _thermal_logger.warning("Attempting NPU device recovery (restart dxrt.service)")
    print("    [recovery] restarting dxrt.service ...", end=" ", flush=True)
    try:
        r = subprocess.run(
            ["sudo", "-n", "systemctl", "restart", "dxrt.service"],
            capture_output=True, text=True, timeout=30,
        )
        if r.returncode == 0:
            # Wait for the service to fully initialize
            time.sleep(3)
            print("ok", flush=True)
            return True
        else:
            print(f"FAILED (rc={r.returncode})", flush=True)
            _thermal_logger.error("dxrt.service restart failed: %s", r.stderr.strip())
            return False
    except subprocess.TimeoutExpired:
        print("TIMEOUT", flush=True)
        return False
    except OSError as e:
        print(f"ERROR ({e})", flush=True)
        return False


def cleanup_after_timeout() -> bool:
    """Full cleanup after a pipeline/model timeout: kill orphans + recover NPU.

    Returns True if NPU recovery succeeded, False otherwise.
    """
    _cleanup_orphaned_pipelines()
    recovered = recover_npu_device()
    if not recovered:
        _thermal_logger.warning("NPU device recovery failed — subsequent benchmarks may fail")
        print("    [recovery] WARNING: NPU recovery failed", flush=True)
    return recovered


# ── Timeout incident data collection ─────────────────────────────────────

# Module-level incident directory — set by callers (e.g. __main__.py)
_incident_dir: Optional[Path] = None
_incident_seq: int = 0


def set_incident_dir(path: Path) -> None:
    """Set the directory where timeout incident snapshots are saved."""
    global _incident_dir
    _incident_dir = path


def _run_diagnostic_cmd(cmd: list[str], timeout: int = 10) -> str:
    """Run a diagnostic command and return its output, or an error string."""
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
        return r.stdout + r.stderr
    except subprocess.TimeoutExpired:
        return f"<command timed out after {timeout}s>"
    except OSError as e:
        return f"<command failed: {e}>"


def _run_diagnostic_cmd_elevated(cmd: list[str], timeout: int = 10) -> str:
    """Run a diagnostic command with sudo -n, falling back to bare command.

    Some commands (dmesg, journalctl) require elevated privileges on most
    systems.  Try ``sudo -n`` (non-interactive) first; if that fails with
    a password prompt or permission error, retry without sudo.
    """
    result = _run_diagnostic_cmd(["sudo", "-n"] + cmd, timeout=timeout)
    # sudo prints "sudo: a password is required" to stderr when NOPASSWD
    # is not configured.  Fall back to a bare invocation in that case.
    if "password is required" in result or "sudo:" in result.split("\n", 1)[0]:
        result = _run_diagnostic_cmd(cmd, timeout=timeout)
    return result


def collect_timeout_incident(context: str) -> Optional[Path]:
    """Capture a diagnostic snapshot when a timeout occurs.

    Collects dxrt-cli status, systemctl status, journalctl logs, dmesg tail,
    and process tree to help determine whether the hang is in dxrt or dx-stream.

    Args:
        context: A short label describing when the timeout happened
                 (e.g. "throughput.run3", "e2e.warmup").

    Returns:
        Path to the incident directory, or None if incident_dir is not set.
    """
    global _incident_seq

    if _incident_dir is None:
        return None

    _incident_seq += 1
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    incident_name = f"{_incident_seq:03d}_{ts}_{context}"
    inc_dir = _incident_dir / incident_name
    inc_dir.mkdir(parents=True, exist_ok=True)

    _thermal_logger.info("Collecting timeout incident: %s", incident_name)

    # 1. dxrt-cli -s (NPU device status)
    dxrt_status = _run_diagnostic_cmd(["dxrt-cli", "-s"])
    (inc_dir / "dxrt_cli_status.txt").write_text(dxrt_status)

    # 2. systemctl status dxrt.service
    svc_status = _run_diagnostic_cmd(["systemctl", "status", "dxrt.service", "--no-pager", "-l"])
    (inc_dir / "dxrt_service_status.txt").write_text(svc_status)

    # 3. journalctl for dxrt (last 100 lines)
    journal = _run_diagnostic_cmd_elevated(
        ["journalctl", "-u", "dxrt.service", "--no-pager", "-n", "100", "--output=short-precise"],
    )
    (inc_dir / "dxrt_journal.txt").write_text(journal)

    # 4. dmesg tail (kernel-level NPU/driver errors)
    dmesg = _run_diagnostic_cmd_elevated(["dmesg", "--time-format=iso", "-T"], timeout=5)
    # Keep only last 200 lines to avoid huge files
    dmesg_lines = dmesg.splitlines()
    if len(dmesg_lines) > 200:
        dmesg_lines = dmesg_lines[-200:]
    (inc_dir / "dmesg_tail.txt").write_text("\n".join(dmesg_lines) + "\n")

    # 5. Process tree snapshot
    ps_out = _run_diagnostic_cmd(["ps", "auxf"], timeout=5)
    (inc_dir / "ps_tree.txt").write_text(ps_out)

    # 6. NPU temperature + clock at the moment of timeout
    npu_temp = read_npu_temp_c()
    npu_clock = read_npu_clock_mhz()
    summary_lines = [
        f"incident: {incident_name}",
        f"context: {context}",
        f"timestamp: {ts}",
        f"npu_temp_c: {npu_temp}",
        f"npu_clock_mhz: {npu_clock}",
    ]
    (inc_dir / "summary.txt").write_text("\n".join(summary_lines) + "\n")

    print(f"    [incident] saved: {incident_name}", flush=True)
    return inc_dir


def _build_pipeline_reason(
    requested_runs: int,
    timeout_runs: int,
    parse_fail_runs: int,
    warmup_timed_out: bool,
) -> str:
    parts: list[str] = []
    if warmup_timed_out:
        parts.append("warmup timed out")
    if timeout_runs:
        parts.append(f"{timeout_runs}/{requested_runs} measured runs timed out")
    if parse_fail_runs:
        parts.append(f"{parse_fail_runs}/{requested_runs} measured runs were unparsable")
    return "; ".join(parts)


def _save_pipeline_log(
    save_dir: Path,
    mode: str,
    model_name: str,
    use_ort: bool,
    stream_count: int,
    log: str,
    run_index: int | None = None,
    is_warmup: bool = False,
    npu_log: str = "",
) -> None:
    """Save a single raw pipeline log with unique names per benchmark mode."""
    save_dir.mkdir(parents=True, exist_ok=True)
    ort_tag = "ort_on" if use_ort else "ort_off"
    if mode == "single":
        prefix = f"{model_name}.e2e.single.{ort_tag}"
    else:
        prefix = f"{model_name}.e2e.multi.{ort_tag}.sc{stream_count}"

    suffix = "warmup" if is_warmup else f"run{run_index}"
    with open(save_dir / f"{prefix}.{suffix}.log", "w") as f:
        f.write(log)
    if npu_log:
        with open(save_dir / f"{prefix}.{suffix}.npu.log", "w") as f:
            f.write(npu_log)


# ── Thermal steady-state helpers ──────────────────────────────────────────

_thermal_logger = logging.getLogger(__name__)


def wait_until_cool(cfg: BenchmarkConfig) -> float:
    """Wait for NPU temperature to drop below min(T_idle + delta, abs_cap).

    Returns the final temperature (°C), or -1 if temp reading is unavailable.
    **Does nothing when thermal_mode != 'steady'.**
    """
    if cfg.thermal_mode != "steady":
        temp = read_npu_temp_c()
        return temp if temp is not None else -1.0

    idle_temp = cfg.thermal_idle_temp_c
    if idle_temp is None:
        # Use a conservative fixed target when idle temp is unknown
        idle_temp = 45.0

    target_temp = min(
        idle_temp + cfg.thermal_cooldown_target_delta_c,
        cfg.thermal_cooldown_abs_cap_c,
    )
    deadline = time.monotonic() + cfg.thermal_cooldown_max_sec

    _first_poll = True
    while time.monotonic() < deadline:
        temp = read_npu_temp_c()
        if temp is None:
            return -1.0
        if temp <= target_temp:
            _thermal_logger.debug("Cooldown complete: %.1f°C <= %.1f°C target", temp, target_temp)
            return temp
        remaining = deadline - time.monotonic()
        if _first_poll:
            print(f"    [cooldown] {temp:.1f}°C → target ≤{target_temp:.1f}°C "
                  f"(max {cfg.thermal_cooldown_max_sec:.0f}s)", flush=True)
            _first_poll = False
        else:
            print(f"    [cooldown] still {temp:.1f}°C > {target_temp:.1f}°C "
                  f"({remaining:.0f}s remaining)", flush=True)
        time.sleep(10)

    # Final check — temperature may have reached target during the last sleep
    temp = read_npu_temp_c()
    if temp is not None and temp <= target_temp:
        _thermal_logger.debug("Cooldown complete (at deadline): %.1f°C <= %.1f°C target", temp, target_temp)
        return temp

    print(f"    [cooldown] TIMEOUT — {(temp or -1):.1f}°C still above {target_temp:.1f}°C "
          f"after {cfg.thermal_cooldown_max_sec:.0f}s", flush=True)
    _thermal_logger.warning("Cooldown timeout (%.0fs). Current: %.1f°C, target: %.1f°C",
                            cfg.thermal_cooldown_max_sec, temp or -1, target_temp)
    raise RuntimeError(
        f"Cooldown timed out at {(temp or -1):.1f}°C; target was {target_temp:.1f}°C"
    )


# ── Public runners ────────────────────────────────────────────────────────

def run_single_stream(
    model: ModelEntry,
    use_ort: bool,
    cfg: BenchmarkConfig,
    save_dir: Optional[Path] = None,
) -> PipelineResult:
    """Run single-stream E2E pipeline benchmark for one model."""
    video = str(cfg.get_video(model.task))
    frame_count = _get_frame_count(video)
    postprocess_cfg = str(get_postprocess_config_path(model.task_suffix))
    preprocess = get_task_preprocess(model.task_suffix)
    inference = get_task_inference()

    pipeline = _build_single_pipeline(str(model.path), use_ort, video, postprocess_cfg,
                                      preprocess, inference)

    # Warmup run (with retry on timeout)
    ort_tag = "ort_on" if use_ort else "ort_off"
    warmup_timed_out = False
    for warmup_attempt in range(2):
        print(f"    [e2e warmup] {model.name}", flush=True)
        warmup_log = _run_gst_pipeline(pipeline, incident_context=f"{model.name}.{ort_tag}.e2e.warmup")
        if "__TIMEOUT__" not in warmup_log:
            break
        print(f"    [e2e warmup] TIMEOUT", flush=True)
        if warmup_attempt == 0:
            print(f"    [e2e warmup] retrying ...", flush=True)
        else:
            warmup_timed_out = True
    if save_dir:
        _save_pipeline_log(save_dir, "single", model.name, use_ort, 1, warmup_log, is_warmup=True)

    # Measured runs
    times = []
    cpu_pcts = []
    rss_values = []
    decoder = "unknown"
    pipeline_caps = None
    npu_stats_accum: list[NpuStats] = []
    raw_logs = []
    timeout_runs = 0
    parse_fail_runs = 0
    retried_runs: set[int] = set()  # max 1 retry per run index

    for i in range(cfg.e2e_runs):
        print(f"    [e2e run {i + 1}/{cfg.e2e_runs}]", end=" ", flush=True)
        npu = NpuMonitor(cfg.npu_core_ids, cfg.npu_warmup_sec, cfg.npu_drain_sec)
        npu.start()
        log = _run_gst_pipeline(pipeline, incident_context=f"{model.name}.{ort_tag}.e2e.run{i+1}")
        stats = npu.stop()
        raw_logs.append(log)
        if save_dir:
            _save_pipeline_log(save_dir, "single", model.name, use_ort, 1, log, run_index=i + 1, npu_log=stats.raw_log)

        if "__TIMEOUT__" in log:
            # Retry once (deadlock can happen on any run, not just warmup)
            if i not in retried_runs:
                retried_runs.add(i)
                print("TIMEOUT → retrying ...", flush=True)
                npu = NpuMonitor(cfg.npu_core_ids, cfg.npu_warmup_sec, cfg.npu_drain_sec)
                npu.start()
                log = _run_gst_pipeline(pipeline, incident_context=f"{model.name}.{ort_tag}.e2e.run{i+1}.retry")
                stats = npu.stop()
                raw_logs.append(log)
                if save_dir:
                    _save_pipeline_log(save_dir, "single", model.name, use_ort, 1, log, run_index=i + 1, npu_log=stats.raw_log)
                if "__TIMEOUT__" not in log:
                    print(f"    [e2e run {i + 1}/{cfg.e2e_runs}] (retry)", end=" ", flush=True)
                else:
                    timeout_runs += 1
                    print(f"    [e2e run {i + 1}/{cfg.e2e_runs}] retry TIMEOUT", flush=True)
                    continue
            else:
                timeout_runs += 1
                print("TIMEOUT", flush=True)
                continue

        t = _parse_execution_time(log)
        if t is None:
            parse_fail_runs += 1
            print("parse failed", flush=True)
            continue

        fps_run = frame_count / t if t > 0 and frame_count > 0 else 0.0
        print(f"{fps_run:.1f} fps ({t:.1f}s)", flush=True)
        times.append(t)
        npu_stats_accum.append(stats)

        cpu = _parse_cpu_pct(log)
        if cpu is not None:
            cpu_pcts.append(cpu)

        rss = _parse_max_rss_kb(log)
        if rss:
            rss_values.append(rss)

        if decoder == "unknown":
            decoder = _detect_decoder(log)

        if pipeline_caps is None:
            pipeline_caps = _extract_pipeline_caps(log)

    completed_runs = len(times)
    reason = _build_pipeline_reason(cfg.e2e_runs, timeout_runs, parse_fail_runs, warmup_timed_out)

    if not times:
        return PipelineResult(
            model=model.name, task=model.task, size=model.size,
            use_ort=use_ort, video=video, frame_count=frame_count,
            runs=0,
            requested_runs=cfg.e2e_runs,
            timeout_runs=timeout_runs,
            status="timeout" if timeout_runs else "error",
            reason=reason or "Could not parse execution time",
        )

    avg_time = sum(times) / len(times)
    avg_fps = frame_count / avg_time if avg_time > 0 and frame_count > 0 else 0.0
    fps_list = [frame_count / t for t in times if t > 0] if frame_count > 0 else []
    fps_std = statistics.stdev(fps_list) if len(fps_list) >= 2 else None
    avg_cpu = sum(cpu_pcts) / len(cpu_pcts) if cpu_pcts else 0.0
    max_rss = max(rss_values) / 1024 if rss_values else 0.0  # KB → MiB

    # Average NPU stats
    merged_npu = _merge_npu_stats(npu_stats_accum, cfg.npu_core_ids)

    result = PipelineResult(
        model=model.name, task=model.task, size=model.size,
        use_ort=use_ort, video=video,
        stream_count=1,
        frame_count=frame_count,
        runs=completed_runs,
        requested_runs=cfg.e2e_runs,
        timeout_runs=timeout_runs,
        decoder=decoder,
        avg_time_sec=avg_time,
        avg_e2e_fps=avg_fps,
        fps_std=fps_std,
        avg_per_channel_fps=avg_fps,
        avg_cpu_pct=avg_cpu,
        max_rss_mib=max_rss,
        npu_stats=merged_npu.as_dict(cfg.npu_core_ids),
        pipeline_caps=pipeline_caps,
        status="partial" if timeout_runs or parse_fail_runs else "ok",
        reason=reason,
        raw_logs=raw_logs,
    )

    return result


def run_multi_stream(
    model: ModelEntry,
    use_ort: bool,
    stream_count: int,
    cfg: BenchmarkConfig,
    save_dir: Optional[Path] = None,
) -> PipelineResult:
    """Run multi-stream E2E pipeline benchmark for one model at a given stream count."""
    video = str(cfg.get_video(model.task))
    frame_count_per_stream = _get_frame_count(video)
    total_frames = frame_count_per_stream * stream_count
    postprocess_cfg = str(get_postprocess_config_path(model.task_suffix))
    preprocess = get_task_preprocess(model.task_suffix)
    inference = get_task_inference()

    pipeline = _build_multi_pipeline(
        str(model.path), use_ort, video, postprocess_cfg, stream_count,
        preprocess, inference,
    )

    # Warmup run (with retry — multi-stream init can deadlock intermittently)
    ort_tag = "ort_on" if use_ort else "ort_off"
    warmup_timed_out = False
    for warmup_attempt in range(2):
        print(f"    [multi warmup] {stream_count}ch", flush=True)
        warmup_log = _run_gst_pipeline(pipeline, incident_context=f"{model.name}.{ort_tag}.multi.sc{stream_count}.warmup")
        if "__TIMEOUT__" not in warmup_log:
            break
        print(f"    [multi warmup] TIMEOUT", flush=True)
        if warmup_attempt == 0:
            print(f"    [multi warmup] retrying ({stream_count}ch) ...", flush=True)
        else:
            warmup_timed_out = True
    if save_dir:
        _save_pipeline_log(save_dir, "multi", model.name, use_ort, stream_count, warmup_log, is_warmup=True)

    # Measured runs
    times = []
    cpu_pcts = []
    rss_values = []
    decoder = "unknown"
    pipeline_caps = None
    npu_stats_accum: list[NpuStats] = []
    raw_logs = []
    timeout_runs = 0
    parse_fail_runs = 0
    retried_runs: set[int] = set()  # max 1 retry per run index

    for i in range(cfg.e2e_runs):
        print(f"    [multi run {i + 1}/{cfg.e2e_runs}] {stream_count}ch", end=" ", flush=True)
        npu = NpuMonitor(cfg.npu_core_ids, cfg.npu_warmup_sec, cfg.npu_drain_sec)
        npu.start()
        log = _run_gst_pipeline(pipeline, incident_context=f"{model.name}.{ort_tag}.multi.sc{stream_count}.run{i+1}")
        stats = npu.stop()
        raw_logs.append(log)
        if save_dir:
            _save_pipeline_log(save_dir, "multi", model.name, use_ort, stream_count, log, run_index=i + 1, npu_log=stats.raw_log)

        if "__TIMEOUT__" in log:
            # Retry once (multi-stream deadlock can happen on any run)
            if i not in retried_runs:
                retried_runs.add(i)
                print("TIMEOUT → retrying ...", flush=True)
                npu = NpuMonitor(cfg.npu_core_ids, cfg.npu_warmup_sec, cfg.npu_drain_sec)
                npu.start()
                log = _run_gst_pipeline(pipeline, incident_context=f"{model.name}.{ort_tag}.multi.sc{stream_count}.run{i+1}.retry")
                stats = npu.stop()
                raw_logs.append(log)
                if save_dir:
                    _save_pipeline_log(save_dir, "multi", model.name, use_ort, stream_count, log, run_index=i + 1, npu_log=stats.raw_log)
                if "__TIMEOUT__" not in log:
                    print(f"    [multi run {i + 1}/{cfg.e2e_runs}] {stream_count}ch (retry)", end=" ", flush=True)
                else:
                    timeout_runs += 1
                    print(f"    [multi run {i + 1}/{cfg.e2e_runs}] {stream_count}ch retry TIMEOUT", flush=True)
                    continue
            else:
                timeout_runs += 1
                print("TIMEOUT", flush=True)
                continue

        t = _parse_execution_time(log)
        if t is None:
            parse_fail_runs += 1
            print("parse failed", flush=True)
            continue

        fps_run = total_frames / t if t > 0 and total_frames > 0 else 0.0
        fps_per_ch = fps_run / stream_count if stream_count > 0 else fps_run
        print(f"{fps_per_ch:.1f} fps/ch  total={fps_run:.1f} ({t:.1f}s)", flush=True)
        times.append(t)
        npu_stats_accum.append(stats)

        cpu = _parse_cpu_pct(log)
        if cpu is not None:
            cpu_pcts.append(cpu)

        rss = _parse_max_rss_kb(log)
        if rss:
            rss_values.append(rss)

        if decoder == "unknown":
            decoder = _detect_decoder(log)

        if pipeline_caps is None:
            pipeline_caps = _extract_pipeline_caps(log)

    completed_runs = len(times)
    reason = _build_pipeline_reason(cfg.e2e_runs, timeout_runs, parse_fail_runs, warmup_timed_out)

    if not times:
        return PipelineResult(
            model=model.name, task=model.task, size=model.size,
            use_ort=use_ort, video=video, frame_count=total_frames,
            stream_count=stream_count,
            runs=0,
            requested_runs=cfg.e2e_runs,
            timeout_runs=timeout_runs,
            status="timeout" if timeout_runs else "error",
            reason=reason or "Could not parse execution time",
        )

    avg_time = sum(times) / len(times)
    avg_fps = total_frames / avg_time if avg_time > 0 and total_frames > 0 else 0.0
    fps_list = [total_frames / t for t in times if t > 0] if total_frames > 0 else []
    fps_std = statistics.stdev(fps_list) if len(fps_list) >= 2 else None
    avg_per_ch = avg_fps / stream_count if stream_count > 0 else 0.0
    avg_cpu = sum(cpu_pcts) / len(cpu_pcts) if cpu_pcts else 0.0
    max_rss = max(rss_values) / 1024 if rss_values else 0.0

    merged_npu = _merge_npu_stats(npu_stats_accum, cfg.npu_core_ids)

    result = PipelineResult(
        model=model.name, task=model.task, size=model.size,
        use_ort=use_ort, video=video,
        stream_count=stream_count,
        frame_count=total_frames,
        runs=completed_runs,
        requested_runs=cfg.e2e_runs,
        timeout_runs=timeout_runs,
        decoder=decoder,
        avg_time_sec=avg_time,
        avg_e2e_fps=avg_fps,
        fps_std=fps_std,
        avg_per_channel_fps=avg_per_ch,
        avg_cpu_pct=avg_cpu,
        max_rss_mib=max_rss,
        npu_stats=merged_npu.as_dict(cfg.npu_core_ids),
        pipeline_caps=pipeline_caps,
        status="partial" if timeout_runs or parse_fail_runs else "ok",
        reason=reason,
        raw_logs=raw_logs,
    )

    return result


# Safety cap to prevent infinite loops (in practice FPS always declines first)
_MAX_SWEEP_STREAMS = 128


def is_capacity_pass(result: PipelineResult | dict, fps_threshold: float) -> bool:
    """Return True when a result satisfies the fixed stable-capacity rule."""
    status = result["status"] if isinstance(result, dict) else result.status
    runs = int(result.get("runs", 0) if isinstance(result, dict) else result.runs)
    requested_runs = int(
        result.get("requested_runs", runs) if isinstance(result, dict) else result.requested_runs
    )
    avg_per_channel_fps = float(
        result.get("avg_per_channel_fps", 0.0) if isinstance(result, dict) else result.avg_per_channel_fps
    )
    return status == "ok" and runs == requested_runs and avg_per_channel_fps >= fps_threshold


def estimate_start_stream(single_stream_fps: float, fps_threshold: float) -> int:
    """Estimate the initial stream count from single-stream E2E FPS."""
    if fps_threshold <= 0:
        return 1
    return max(1, int(single_stream_fps // fps_threshold))


def get_existing_capacity(existing_results: list[dict], fps_threshold: float) -> int:
    """Return the best stable capacity from existing multi-stream results."""
    passing_streams = [
        int(result.get("stream_count", 0) or 0)
        for result in existing_results
        if is_capacity_pass(result, fps_threshold)
    ]
    return max(passing_streams, default=0)


def get_boundary_search_start(
    existing_results: list[dict],
    fps_threshold: float,
    single_stream_fps: float,
) -> int | None:
    """Choose the next stream count for boundary search from existing results."""
    if not existing_results:
        return estimate_start_stream(single_stream_fps, fps_threshold)

    latest_by_stream: dict[int, dict] = {}
    for result in existing_results:
        stream_count = int(result.get("stream_count", 0) or 0)
        latest_by_stream[stream_count] = result

    pass_streams = sorted(
        stream for stream, result in latest_by_stream.items() if is_capacity_pass(result, fps_threshold)
    )
    fail_streams = sorted(
        stream for stream, result in latest_by_stream.items() if not is_capacity_pass(result, fps_threshold)
    )

    max_pass = max(pass_streams, default=0)
    min_fail = min(fail_streams, default=_MAX_SWEEP_STREAMS + 1)

    if max_pass and min_fail == max_pass + 1:
        return None
    if fail_streams and not pass_streams:
        return max(1, min_fail - 1)
    if pass_streams and not fail_streams:
        return min(_MAX_SWEEP_STREAMS, max_pass + 1)
    if pass_streams and fail_streams:
        candidate = max_pass + 1
        if candidate < min_fail:
            return candidate
        return None
    return estimate_start_stream(single_stream_fps, fps_threshold)


def run_multi_stream_sweep(
    model: ModelEntry,
    use_ort: bool,
    cfg: BenchmarkConfig,
    save_dir: Optional[Path] = None,
    start_stream: int = 1,
    existing_results: Optional[list[dict]] = None,
    retry_stream_counts: Optional[set[int]] = None,
    progress_callback: Optional[Callable[[int], None]] = None,
    result_callback: Optional[Callable[["PipelineResult"], None]] = None,
    single_stream_result: Optional[dict] = None,
) -> list[PipelineResult]:
    """Find the multi-stream boundary starting from a single-stream FPS-based estimate."""
    known_results = {int(item.get("stream_count", 0) or 0): item for item in (existing_results or [])}
    results: list[PipelineResult] = []
    current = max(1, start_stream)
    direction: int | None = None
    retry_stream_counts = {
        stream_count for stream_count in (retry_stream_counts or set()) if stream_count > 0
    }
    visited: set[int] = set(known_results) - retry_stream_counts
    retried_streams: set[int] = set()  # tracks timeout retries (max 1 per stream count)

    while 1 <= current <= _MAX_SWEEP_STREAMS and current not in visited:
        if progress_callback is not None:
            progress_callback(current)

        # Reuse single-stream result for 1ch instead of running pipeline
        if current == 1 and single_stream_result and single_stream_result.get("status") in ("ok", "partial"):
            result = _make_sc1_from_single_stream(single_stream_result)
            print(f"    [multi 1ch] reusing single-stream: "
                  f"{result.avg_per_channel_fps:.1f} fps/ch", flush=True)
        else:
            result = run_multi_stream(model, use_ort, current, cfg, save_dir)

            # Retry once on timeout (multi-stream init can deadlock intermittently)
            if result.status == "timeout" and current not in retried_streams:
                retried_streams.add(current)
                print(f"    [multi {current}ch] retrying after timeout ...", flush=True)
                result = run_multi_stream(model, use_ort, current, cfg, save_dir)

        results.append(result)
        if result_callback is not None:
            result_callback(result)
        visited.add(current)

        passed = is_capacity_pass(result, cfg.fps_threshold)
        if direction is None:
            direction = 1 if passed else -1

        if direction == 1 and passed:
            current += 1
            continue
        if direction == -1 and not passed:
            current -= 1
            continue

        break

    return results


def _make_sc1_from_single_stream(single_result: dict) -> PipelineResult:
    """Convert a single-stream pipeline result dict to a multi-stream sc=1 PipelineResult."""
    # Extract npu_stats fields from the flat single_result dict
    npu_keys = [k for k in single_result if k.startswith("npu_")]
    npu_stats = {k: single_result[k] for k in npu_keys} if npu_keys else None

    return PipelineResult(
        model=single_result.get("model", ""),
        task=single_result.get("task", ""),
        size=single_result.get("size", ""),
        use_ort=bool(single_result.get("use_ort")),
        video=single_result.get("video", ""),
        stream_count=1,
        frame_count=int(single_result.get("frame_count", 0)),
        runs=int(single_result.get("runs", 0)),
        requested_runs=int(single_result.get("requested_runs", 0)),
        timeout_runs=int(single_result.get("timeout_runs", 0)),
        decoder=single_result.get("decoder", "unknown"),
        avg_time_sec=float(single_result.get("avg_time_sec", 0.0)),
        avg_e2e_fps=float(single_result.get("avg_e2e_fps", 0.0)),
        fps_std=float(single_result["fps_std"]) if single_result.get("fps_std") is not None else None,
        avg_per_channel_fps=float(single_result.get("avg_e2e_fps", 0.0)),
        avg_cpu_pct=float(single_result.get("avg_cpu_pct", 0.0)),
        max_rss_mib=float(single_result.get("max_rss_mib", 0.0)),
        npu_stats=npu_stats,
        pipeline_caps=single_result.get("pipeline_caps"),
        status=single_result.get("status", "ok"),
        source="single_stream",
    )

