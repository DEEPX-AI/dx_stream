"""Model-level benchmark runners.

Two benchmark families:
  - throughput: async (multi-core), high loop count → measures FPS
  - latency:   sync  (single-core) with profiler → measures ms per frame
"""

import json
import os
import re
import shutil
import statistics
import subprocess
import tempfile
import time
from contextlib import suppress
from dataclasses import dataclass
from pathlib import Path
from typing import Optional

from .config import BenchmarkConfig, ROOT_DIR
from .model_catalog import ModelEntry
from .npu_monitor import NpuMonitor, NpuStats
from .npu_stats_util import merge_npu_stats as _merge_npu_stats
from .runner_pipeline import cleanup_after_timeout as _cleanup_after_timeout
from .runner_pipeline import collect_timeout_incident as _collect_timeout_incident


def _stdev(values: list[float]) -> Optional[float]:
    """Return sample stdev if ≥2 values, else None."""
    return statistics.stdev(values) if len(values) >= 2 else None


def _cleanup_run_model(incident_context: str = "") -> None:
    """Kill any lingering run_model processes and recover NPU after timeout."""
    try:
        subprocess.run(
            ["pkill", "-9", "-f", "run_model"],
            timeout=5, capture_output=True,
        )
    except (subprocess.TimeoutExpired, OSError):
        pass
    time.sleep(0.5)
    _collect_timeout_incident(incident_context or "run_model")
    _cleanup_after_timeout()


@dataclass
class ModelResult:
    """Result of a single model-level benchmark run."""
    model: str
    task: str
    size: str
    use_ort: bool
    family: str          # "throughput" or "latency"
    fps: Optional[float] = None
    total_ms: Optional[float] = None
    npu_task_ms: Optional[float] = None
    cpu_0_ms: Optional[float] = None
    cpu_pct: Optional[float] = None
    fps_std: Optional[float] = None
    npu_stats: Optional[dict] = None
    input_tensor: Optional[dict] = None
    status: str = "ok"
    reason: str = ""
    raw_log: str = ""

    def as_dict(self) -> dict:
        d = {
            "model": self.model,
            "task": self.task,
            "size": self.size,
            "use_ort": self.use_ort,
            "family": self.family,
            "fps": self.fps,
            "fps_std": self.fps_std,
            "total_ms": self.total_ms,
            "npu_task_ms": self.npu_task_ms,
            "cpu_0_ms": self.cpu_0_ms,
            "cpu_pct": self.cpu_pct,
            "status": self.status,
            "reason": self.reason,
        }
        if self.npu_stats:
            d.update(self.npu_stats)
        if self.input_tensor:
            d["input_tensor"] = self.input_tensor
        return d


def _parse_input_tensor_shape(log: str) -> Optional[dict]:
    """Extract the first input tensor info from run_model output.

    Matches lines like:  ``  -  images, UINT8, [1, 640, 640, 3 ]``
    Returns e.g. ``{"name": "images", "dtype": "UINT8", "shape": [1, 640, 640, 3]}``.
    """
    m = re.search(
        r"-\s+(\w+),\s+(\w+),\s*\[([\d,\s]+)\]",
        log,
    )
    if not m:
        return None
    name = m.group(1)
    dtype = m.group(2)
    shape = [int(x.strip()) for x in m.group(3).split(",") if x.strip()]
    return {"name": name, "dtype": dtype, "shape": shape}


def _parse_fps_from_log(log: str) -> Optional[float]:
    """Extract average FPS from run_model output."""
    fps_values = []
    for m in re.finditer(r"FPS\s*:\s*([\d.]+)", log):
        fps_values.append(float(m.group(1)))
    return sum(fps_values) / len(fps_values) if fps_values else None


def _parse_npu_memory_bytes(log: str) -> Optional[int]:
    """Extract NPU memory usage in bytes from run_model output.

    Matches lines like: 'NPU memory usage 124,731,520 bytes'
    """
    m = re.search(r"NPU memory usage\s+([\d,]+)\s+bytes", log)
    if m:
        return int(m.group(1).replace(",", ""))
    return None


def _parse_cpu_pct(stderr: str) -> Optional[float]:
    """Extract CPU% from GNU time output."""
    m = re.search(r"__CPU_PERCENT__=([\d]+)%", stderr)
    if m:
        return float(m.group(1))
    return None


def _parse_profiler_metric(profiler_path: Path, metric_name: str) -> Optional[float]:
    """Extract a metric's average duration (ms) from profiler.json.

    Searches JSON keys case-insensitively for *metric_name* and averages
    durations across ALL matching keys (one event per job/key).
    """
    if not profiler_path.exists():
        return None
    try:
        with open(profiler_path) as f:
            data = json.load(f)
    except (json.JSONDecodeError, OSError):
        return None

    target_key = metric_name.lower()
    all_durations: list[float] = []
    for key, events in data.items():
        if target_key not in key.lower():
            continue
        if not isinstance(events, list) or not events:
            continue
        for ev in events:
            if "start" in ev and "end" in ev:
                all_durations.append(ev["end"] - ev["start"])
    if all_durations:
        avg_ns = sum(all_durations) / len(all_durations)
        return avg_ns / 1_000_000  # ns → ms
    return None


def run_throughput(
    model: ModelEntry,
    use_ort: bool,
    cfg: BenchmarkConfig,
    save_dir: Optional[Path] = None,
) -> ModelResult:
    """Run async (multi-core) throughput benchmark for one model.

    Performs one warmup run followed by *cfg.model_runs* measured runs and
    reports the average FPS, matching the E2E pipeline measurement approach.
    """
    cmd = [
        "/usr/bin/time", "-f", "__CPU_PERCENT__=%P",
        "run_model", "-m", str(model.path),
        "-t", str(cfg.model_time_sec),
        "--warmup-runs", str(cfg.model_warmup),
    ]
    if use_ort:
        cmd.append("--use-ort")

    num_runs = max(1, cfg.model_throughput_runs)
    ort_s = "ON" if use_ort else "OFF"
    ort_tag = "ort_on" if use_ort else "ort_off"

    # Warmup run (discard result)
    print(f"    [throughput warmup] (-t {cfg.model_time_sec}s)", flush=True)
    try:
        subprocess.run(cmd, capture_output=True, text=True, timeout=600)
    except subprocess.TimeoutExpired:
        _cleanup_run_model(f"{model.name}.{ort_tag}.throughput.warmup")
        return ModelResult(
            model=model.name, task=model.task, size=model.size,
            use_ort=use_ort, family="throughput",
            status="timeout", reason="warmup run exceeded 600s",
        )

    # Measured runs
    fps_values = []
    cpu_pcts = []
    last_combined = ""
    npu_stats_accum: list[NpuStats] = []
    last_npu_mem = None

    for run_idx in range(num_runs):
        print(f"    [throughput run {run_idx + 1}/{num_runs}]", end=" ", flush=True)
        t0_run = time.monotonic()
        npu = NpuMonitor(cfg.npu_core_ids, cfg.npu_warmup_sec, cfg.npu_drain_sec)
        npu.start()

        try:
            proc = subprocess.run(
                cmd, capture_output=True, text=True, timeout=600,
            )
            combined = proc.stdout + "\n" + proc.stderr
        except subprocess.TimeoutExpired:
            npu.stop()
            _cleanup_run_model(f"{model.name}.{ort_tag}.throughput.run{run_idx+1}")
            print("TIMEOUT", flush=True)
            continue

        npu_stats = npu.stop()
        last_combined = combined
        npu_stats_accum.append(npu_stats)

        fps = _parse_fps_from_log(combined)
        if fps:
            fps_values.append(fps)
            print(f"{fps:.1f} fps ({time.monotonic() - t0_run:.1f}s)", flush=True)
        else:
            print("no fps parsed", flush=True)

        cpu_pct = _parse_cpu_pct(proc.stderr)
        if cpu_pct is not None:
            cpu_pcts.append(cpu_pct)

        mem = _parse_npu_memory_bytes(combined)
        if mem is not None:
            last_npu_mem = mem

        if save_dir:
            _save_raw(save_dir, model.name, f"throughput.run{run_idx + 1}", use_ort, combined, npu_stats.raw_log)

    if not fps_values:
        return ModelResult(
            model=model.name, task=model.task, size=model.size,
            use_ort=use_ort, family="throughput",
            status="no_fps", reason="Could not parse FPS from any run",
        )

    avg_fps = sum(fps_values) / len(fps_values)
    fps_std = _stdev(fps_values)
    avg_cpu = sum(cpu_pcts) / len(cpu_pcts) if cpu_pcts else None

    # Merge NpuStats across all measured runs
    merged_npu = _merge_npu_stats(npu_stats_accum, cfg.npu_core_ids)
    npu_dict = merged_npu.as_dict(cfg.npu_core_ids)
    if last_npu_mem is not None:
        npu_dict["npu_model_mem_bytes"] = last_npu_mem

    input_tensor = _parse_input_tensor_shape(last_combined)

    result = ModelResult(
        model=model.name, task=model.task, size=model.size,
        use_ort=use_ort, family="throughput",
        fps=avg_fps,
        fps_std=fps_std,
        cpu_pct=avg_cpu,
        npu_stats=npu_dict,
        input_tensor=input_tensor,
        status="ok",
        reason=f"avg of {len(fps_values)}/{num_runs} runs",
        raw_log=last_combined,
    )

    return result


def run_latency(
    model: ModelEntry,
    use_ort: bool,
    cfg: BenchmarkConfig,
    save_dir: Optional[Path] = None,
) -> ModelResult:
    """Run sync (single-core) latency benchmark with profiler for one model.

    Performs one warmup run followed by *cfg.model_runs* measured runs and
    reports the average latency, matching the E2E pipeline measurement approach.
    """
    work_dir_root = None
    if save_dir is not None:
        work_dir_root = save_dir / ".tmp"
        work_dir_root.mkdir(parents=True, exist_ok=True)

    cmd = [
        "/usr/bin/time", "-f", "__CPU_PERCENT__=%P",
        "run_model", "-m", str(model.path),
        "-s", "--profiler",
        "-l", str(cfg.model_latency_loops),
        "--warmup-runs", str(cfg.model_warmup),
    ]
    if use_ort:
        cmd.append("--use-ort")

    num_runs = max(1, cfg.model_latency_runs)
    ort_s = "ON" if use_ort else "OFF"
    ort_tag = "ort_on" if use_ort else "ort_off"

    # Warmup run (discard result)
    print(f"    [latency warmup] (-l {cfg.model_latency_loops}, profiler)", flush=True)
    try:
        with tempfile.TemporaryDirectory(prefix="bench_warmup_", dir=work_dir_root) as wd:
            subprocess.run(cmd, capture_output=True, text=True, timeout=600, cwd=wd)
    except subprocess.TimeoutExpired:
        _cleanup_run_model(f"{model.name}.{ort_tag}.latency.warmup")
        return ModelResult(
            model=model.name, task=model.task, size=model.size,
            use_ort=use_ort, family="latency",
            status="timeout", reason="warmup run exceeded 600s",
        )

    # Measured runs
    total_ms_values = []
    npu_task_ms_values = []
    cpu_0_ms_values = []
    fps_values = []
    cpu_pcts = []
    last_combined = ""
    npu_stats_accum: list[NpuStats] = []
    last_npu_mem = None
    last_profiler_path = None

    try:
        for run_idx in range(num_runs):
            print(f"    [latency run {run_idx + 1}/{num_runs}]", end=" ", flush=True)
            t0_run = time.monotonic()
            with tempfile.TemporaryDirectory(prefix="bench_latency_", dir=work_dir_root) as work_dir:
                profiler_path = Path(work_dir) / "profiler.json"

                npu = NpuMonitor(cfg.npu_core_ids, cfg.npu_warmup_sec, cfg.npu_drain_sec)
                npu.start()

                try:
                    proc = subprocess.run(
                        cmd, capture_output=True, text=True, timeout=600,
                        cwd=work_dir,
                    )
                    combined = proc.stdout + "\n" + proc.stderr
                except subprocess.TimeoutExpired:
                    npu.stop()
                    _cleanup_run_model(f"{model.name}.{ort_tag}.latency.run{run_idx+1}")
                    print("TIMEOUT", flush=True)
                    continue

                npu_stats = npu.stop()
                last_combined = combined
                npu_stats_accum.append(npu_stats)

                cpu_pct = _parse_cpu_pct(proc.stderr)
                if cpu_pct is not None:
                    cpu_pcts.append(cpu_pct)

                mem = _parse_npu_memory_bytes(combined)
                if mem is not None:
                    last_npu_mem = mem

                npu_task_ms = _parse_profiler_metric(profiler_path, "npu task")
                cpu_0_ms = _parse_profiler_metric(profiler_path, "cpu_0")

                if npu_task_ms is not None and cpu_0_ms is not None:
                    elapsed_run = time.monotonic() - t0_run
                    print(f"{npu_task_ms + cpu_0_ms:.1f}ms  (npu={npu_task_ms:.1f} cpu0={cpu_0_ms:.1f}, {elapsed_run:.1f}s)", flush=True)
                    total_ms_values.append(npu_task_ms + cpu_0_ms)
                    npu_task_ms_values.append(npu_task_ms)
                    cpu_0_ms_values.append(cpu_0_ms)
                elif npu_task_ms is not None:
                    elapsed_run = time.monotonic() - t0_run
                    print(f"{npu_task_ms:.1f}ms  (npu only, {elapsed_run:.1f}s)", flush=True)
                    total_ms_values.append(npu_task_ms)
                    npu_task_ms_values.append(npu_task_ms)
                else:
                    fps_fallback = _parse_fps_from_log(combined)
                    if fps_fallback:
                        elapsed_run = time.monotonic() - t0_run
                        print(f"{fps_fallback:.1f} fps (profiler fallback, {elapsed_run:.1f}s)", flush=True)
                        fps_values.append(fps_fallback)
                    else:
                        print("parse failed", flush=True)

                # Save last profiler for archival
                if profiler_path.exists() and save_dir:
                    dest = save_dir / f"{model.name}.ort_{'on' if use_ort else 'off'}.profiler.json"
                    shutil.copy2(profiler_path, dest)
                    last_profiler_path = dest

                if save_dir:
                    _save_raw(save_dir, model.name, f"latency.run{run_idx + 1}", use_ort, combined, npu_stats.raw_log)

        # Compute averages
        if total_ms_values:
            total_ms = sum(total_ms_values) / len(total_ms_values)
            npu_task_ms = sum(npu_task_ms_values) / len(npu_task_ms_values) if npu_task_ms_values else None
            cpu_0_ms = sum(cpu_0_ms_values) / len(cpu_0_ms_values) if cpu_0_ms_values else None
            fps = 1000.0 / total_ms if total_ms > 0 else None
            status = "ok"
            reason = f"avg of {len(total_ms_values)}/{num_runs} runs"
        elif fps_values:
            fps = sum(fps_values) / len(fps_values)
            total_ms = 1000.0 / fps if fps > 0 else None
            npu_task_ms = None
            cpu_0_ms = None
            status = "partial"
            reason = f"Profiler keys not found; FPS from stdout ({len(fps_values)}/{num_runs} runs)"
        else:
            return ModelResult(
                model=model.name, task=model.task, size=model.size,
                use_ort=use_ort, family="latency",
                status="error", reason="Could not parse metrics from any run",
            )

        npu_dict = _merge_npu_stats(npu_stats_accum, cfg.npu_core_ids).as_dict(cfg.npu_core_ids)
        if last_npu_mem is not None:
            npu_dict["npu_model_mem_bytes"] = last_npu_mem

        return ModelResult(
            model=model.name, task=model.task, size=model.size,
            use_ort=use_ort, family="latency",
            fps=fps, total_ms=total_ms,
            npu_task_ms=npu_task_ms, cpu_0_ms=cpu_0_ms,
            cpu_pct=sum(cpu_pcts) / len(cpu_pcts) if cpu_pcts else None,
            npu_stats=npu_dict,
            status=status, reason=reason,
            raw_log=last_combined,
        )
    finally:
        if work_dir_root is not None and work_dir_root.exists():
            with suppress(OSError):
                work_dir_root.rmdir()


def _save_raw(save_dir: Path, model_name: str, family: str, use_ort: bool, log: str, npu_log: str) -> None:
    """Save raw logs to disk."""
    save_dir.mkdir(parents=True, exist_ok=True)
    ort_tag = "ort_on" if use_ort else "ort_off"
    prefix = f"{model_name}.{family}.{ort_tag}"
    with open(save_dir / f"{prefix}.log", "w") as f:
        f.write(log)
    if npu_log:
        with open(save_dir / f"{prefix}.npu.log", "w") as f:
            f.write(npu_log)
