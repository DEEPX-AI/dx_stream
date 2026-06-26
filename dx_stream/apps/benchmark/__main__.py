#!/usr/bin/env python3
"""YOLO26 Benchmark Tool – unified benchmark orchestrator.

Usage:
    python -m benchmark preflight          # check environment
    python -m benchmark dry-run            # show what would run
    python -m benchmark run                # run all benchmarks
    python -m benchmark run --family model # model-level only
    python -m benchmark run --family e2e   # E2E pipeline only
    python -m benchmark run --family multi # multi-stream only
    python -m benchmark report             # regenerate report from results
"""

import argparse
import json
import re
import statistics
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

from .aggregator import aggregate_result_directories, save_dataset_json
from .result_layout import make_hw_id
from .config import BenchmarkConfig, ROOT_DIR, SIZES, TASK_GROUP_MAP, TASK_GROUP_VIDEOS, E2E_SUPPORTED_TASKS, MULTI_STREAM_SUPPORTED_TASKS, TASK_MODEL_META, get_protocol_metadata
from .dashboard_builder import build_static_dashboard
from .env_fingerprint import collect_fingerprint, check_preflight, save_fingerprint, get_video_info
from .model_catalog import discover_models, filter_models
from .npu_monitor import parse_npu_log_temp_clock
from .reporter import (
    generate_markdown_report,
    save_results_csv,
    save_results_json,
)
from .runner_model import run_throughput, run_latency
from .runner_pipeline import run_single_stream, run_multi_stream_sweep, get_boundary_search_start, wait_until_cool, set_incident_dir


def _make_run_id() -> str:
    return datetime.now().strftime("%Y%m%d_%H%M%S")


def _resolve_resume_dir(resume_arg: str | None) -> Path | None:
    if not resume_arg:
        return None

    resume_path = Path(resume_arg)
    if resume_path.is_absolute():
        return resume_path

    # Keep documented `results/...` usage stable regardless of the caller's cwd.
    if resume_path.parts and resume_path.parts[0] == "results":
        return (Path(__file__).resolve().parent / resume_path).resolve()

    return resume_path.resolve()


def _resolve_output_dir(
    cfg: BenchmarkConfig,
    resume_dir: Path | None,
    fingerprint: dict,
    run_id: str,
) -> Path:
    if resume_dir:
        return resume_dir
    return cfg.get_output_dir() / make_hw_id(fingerprint) / run_id


def cmd_preflight(args: argparse.Namespace) -> int:
    """Check environment readiness."""
    fp = collect_fingerprint()
    ok, errors = check_preflight(fp)

    print("=== Environment Fingerprint ===")
    print(json.dumps(fp, indent=2, default=str))
    print()

    if ok:
        print("[OK] All required tools are available.")
    else:
        print("[FAIL] Missing required tools:")
        for e in errors:
            print(f"  - {e}")
    return 0 if ok else 1


def cmd_dry_run(args: argparse.Namespace) -> int:
    """Show the benchmark matrix without executing."""
    cfg = _build_config(args)
    models = _get_models(cfg)

    families = _get_families(args)

    print("=== Benchmark Dry-Run ===")
    print(f"Task:      {cfg.task}")
    print(f"Sizes:     {cfg.sizes}")
    print(f"ORT modes: {['ON' if o else 'OFF' for o in cfg.ort_modes]}")
    print(f"Models:    {len(models)}")
    print(f"Families:  {families}")
    print(f"Output:    {cfg.get_output_dir()}")
    print()

    if "model" in families or "all" in families:
        print("--- Model-Level Benchmarks ---")
        print(f"  Throughput: -t {cfg.model_time_sec}s, Warmup: {cfg.model_warmup}, Runs: {cfg.model_throughput_runs}")
        print(f"  Latency:    -l {cfg.model_latency_loops} loops, Warmup: {cfg.model_warmup}, Runs: {cfg.model_latency_runs}")
        for m in models:
            for ort in cfg.ort_modes:
                ort_s = "ON" if ort else "OFF"
                print(f"  [throughput] {m.name}  ORT={ort_s}")
                print(f"  [latency]    {m.name}  ORT={ort_s}")
        print()

    if "e2e" in families or "all" in families:
        print("--- E2E Pipeline (Single-Stream) ---")
        print(f"  Runs: {cfg.e2e_runs}")
        for m in models:
            video = cfg.get_video(m.task)
            for ort in cfg.ort_modes:
                ort_s = "ON" if ort else "OFF"
                print(f"  [e2e] {m.name}  ORT={ort_s}  video={video.name}")
        print()

    if "multi" in families or "all" in families:
        print("--- E2E Pipeline (Multi-Stream Sweep) ---")
        print(f"  Runs: {cfg.e2e_runs}, FPS threshold: {cfg.fps_threshold} (no upper stream limit)")
        for m in models:
            if m.task not in MULTI_STREAM_SUPPORTED_TASKS:
                continue
            for ort in cfg.ort_modes:
                ort_s = "ON" if ort else "OFF"
                print(f"  [multi] {m.name}  ORT={ort_s}  streams=1..∞")
        print()

    total = _count_runs(cfg, models, families)
    print(f"Total benchmark invocations: {total}")
    return 0


def cmd_run(args: argparse.Namespace) -> int:
    """Execute benchmarks."""
    cfg = _build_config(args)
    families = _get_families(args)
    run_id = _make_run_id()
    resume_dir = _resolve_resume_dir(getattr(args, "resume", None))
    session_start_iso = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    session_start_time = time.time()

    if getattr(args, "retry_failed", False) and not resume_dir:
        print("[ERROR] --retry-failed requires --resume <result_dir>")
        return 2

    if resume_dir and not resume_dir.exists():
        print(f"[ERROR] Resume directory not found: {resume_dir}")
        return 2

    # Preflight
    fp = collect_fingerprint()
    ok, errors = check_preflight(fp)
    if not ok:
        print("[FAIL] Preflight check failed:")
        for e in errors:
            print(f"  - {e}")
        return 1

    if cfg.product_name:
        fp["product_name"] = cfg.product_name

    out_dir = _resolve_output_dir(cfg, resume_dir, fp, run_id)
    existing_fp = _load_json_object(out_dir / "environment.json") if resume_dir else {}
    prior_timing = existing_fp.get("timing", {}) if existing_fp else {}
    overall_start_iso = prior_timing.get("start") or session_start_iso
    overall_start_time = _parse_local_timestamp(overall_start_iso) or session_start_time
    timing_history = _load_timing_history(existing_fp)

    # Auto-detect NPU idle temperature for thermal steady-state
    if cfg.thermal_mode == "steady" and cfg.thermal_idle_temp_c is None:
        from .npu_monitor import read_npu_temp_c
        idle_temp = read_npu_temp_c()
        if idle_temp is not None:
            if idle_temp > cfg.thermal_hot_start_block_c:
                print(
                    f"[ERROR] NPU start temperature {idle_temp:.1f}°C exceeds hot-start limit "
                    f"{cfg.thermal_hot_start_block_c:.1f}°C. Cool the device before running the benchmark."
                )
                return 1
            cfg.thermal_idle_temp_c = idle_temp
            print(f"[INFO] NPU idle temperature: {idle_temp:.1f}°C")

    save_fingerprint(fp, out_dir)

    if resume_dir:
        print(f"[INFO] Resuming: {out_dir}")
    else:
        print(f"[INFO] Run ID: {run_id}")
        print(f"[INFO] Output: {out_dir}")

    models = _get_models(cfg)
    if not models:
        print("[WARN] No models found.")
        return 1

    # Collect video info for each task group used by the models
    video_infos: dict[str, dict] = {}
    for task_name in set(m.task for m in models):
        group = TASK_GROUP_MAP.get(task_name, "od_pose_seg")
        if group not in video_infos:
            video_path = cfg.get_video(task_name)
            video_infos[group] = get_video_info(video_path)

    # Save video_infos alongside fingerprint for report regeneration
    fp["video_infos"] = video_infos

    # Record benchmark parameters for reproducibility
    fp["benchmark_params"] = {
        "model_time_sec": cfg.model_time_sec,
        "model_latency_loops": cfg.model_latency_loops,
        "model_warmup": cfg.model_warmup,
        "model_latency_runs": cfg.model_latency_runs,
        "model_throughput_runs": cfg.model_throughput_runs,
        "e2e_runs": cfg.e2e_runs,
        "fps_threshold": cfg.fps_threshold,
        "families": families,
        "ort_modes": ["ON" if o else "OFF" for o in cfg.ort_modes],
        "retry_failed": bool(getattr(args, "retry_failed", False)),
    }
    fp["protocol"] = get_protocol_metadata(cfg)

    # Collect model metadata via parse_model
    from .env_fingerprint import collect_model_metadata
    benchmarked_models = []
    for m in models:
        entry = {
            "name": m.name,
            "task": m.task,
            "size": m.size,
            "input_size": TASK_MODEL_META.get(m.task, {}).get("input_size", "N/A"),
        }
        meta = collect_model_metadata(m.path)
        entry.update(meta)
        benchmarked_models.append(entry)
    fp["benchmarked_models"] = benchmarked_models

    env_path = out_dir / "environment.json"
    with open(env_path, "w") as _f:
        json.dump(fp, _f, indent=2)

    model_results: list[dict] = _load_json(out_dir / "model_results.json") if resume_dir else []
    pipeline_results: list[dict] = _load_json(out_dir / "pipeline_results.json") if resume_dir else []
    multi_results: list[dict] = _load_json(out_dir / "multi_stream_results.json") if resume_dir else []

    model_index = {
        (r.get("model"), bool(r.get("use_ort")), r.get("family")): r
        for r in model_results
    }
    pipeline_index = {
        (r.get("model"), bool(r.get("use_ort"))): r
        for r in pipeline_results
    }

    raw_dir = out_dir / "raw"
    raw_dir.mkdir(parents=True, exist_ok=True)

    # Set up incident directory for timeout diagnostics
    incident_dir = out_dir / "incidents"
    incident_dir.mkdir(parents=True, exist_ok=True)
    set_incident_dir(incident_dir)

    total = _count_runs(cfg, models, families)
    done = 0
    failure_context: dict | None = None

    # ── Per-model sequential execution ───────────────────────────
    # Order per model (steady mode):
    #   ① cooldown → ② latency → ③ throughput → ④ E2E → ⑤ multi-stream
    run_model_level = "model" in families or "all" in families
    run_e2e = "e2e" in families or "all" in families
    run_multi = "multi" in families or "all" in families

    print("\n=== Benchmarks ===")
    total_models = len(models)

    for m_idx, m in enumerate(models, 1):
        for use_ort in cfg.ort_modes:
            ort_s = "ON" if use_ort else "OFF"
            model_timeout_count = 0  # track consecutive timeouts for early skip

            print(f"\n── [{m_idx}/{total_models}] {m.name}  ORT={ort_s}  ({m.task}) ──", flush=True)

            # ① Cooldown — steady mode, model-level runs present
            if run_model_level and cfg.thermal_mode == "steady":
                try:
                    temp = wait_until_cool(cfg)
                except RuntimeError as error:
                    print(f"  [cooldown] FAILED: {error}", flush=True)
                    failure_context = {
                        "failure_stage": "cooldown",
                        "failure_model": m.name,
                        "failure_ort": ort_s,
                        "failure_reason": str(error),
                    }
                    break
                if temp > 0:
                    print(f"  [cooldown] ready: {temp:.1f}°C", flush=True)

            if failure_context:
                break

            # ② Latency (single-core, sync) — run first from cool state
            if run_model_level:
                latency_key = (m.name, use_ort, "latency")
                done += 1
                print(f"  [{done}/{total}] latency   {m.name} ORT={ort_s}", flush=True)
                existing = model_index.get(latency_key)
                if existing and not _should_retry_failed(existing, args.retry_failed):
                    print("    skip [resume]")
                else:
                    if existing:
                        print(f"    retry [{existing.get('status', 'unknown')}]", flush=True)
                    t0 = time.monotonic()
                    r = run_latency(m, use_ort, cfg, raw_dir)
                    elapsed = time.monotonic() - t0
                    r_dict = r.as_dict()
                    _upsert_result(
                        model_results, r_dict,
                        lambda item: (item.get("model"), bool(item.get("use_ort")), item.get("family")),
                    )
                    model_index[latency_key] = r_dict
                    _save_result_set(model_results, out_dir / "model_results.csv", out_dir / "model_results.json")
                    fps_s = f"{r.fps:.1f}" if r.fps else "N/A"
                    print(f"  ← {fps_s} fps ({elapsed:.1f}s) [{r.status}]")
                    if r.status == "timeout":
                        model_timeout_count += 1

            # ③ Throughput (multi-core, async) — records T_start/T_end
            if run_model_level:
                throughput_key = (m.name, use_ort, "throughput")
                done += 1
                print(f"  [{done}/{total}] throughput {m.name} ORT={ort_s}", flush=True)
                existing = model_index.get(throughput_key)
                if existing and not _should_retry_failed(existing, args.retry_failed):
                    print("    skip [resume]")
                else:
                    if existing:
                        print(f"    retry [{existing.get('status', 'unknown')}]", flush=True)
                    t0 = time.monotonic()
                    r = run_throughput(m, use_ort, cfg, raw_dir)
                    elapsed = time.monotonic() - t0
                    r_dict = r.as_dict()
                    _upsert_result(
                        model_results, r_dict,
                        lambda item: (item.get("model"), bool(item.get("use_ort")), item.get("family")),
                    )
                    model_index[throughput_key] = r_dict
                    _save_result_set(model_results, out_dir / "model_results.csv", out_dir / "model_results.json")
                    # Capture input tensor metadata
                    if r.input_tensor is not None:
                        for bm_entry in fp.get("benchmarked_models", []):
                            if bm_entry["name"] == m.name:
                                bm_entry["input_tensor_shape"] = r.input_tensor["shape"]
                                bm_entry["input_tensor_dtype"] = r.input_tensor["dtype"]
                                break
                        with open(env_path, "w") as _f:
                            json.dump(fp, _f, indent=2)
                    fps_s = f"{r.fps:.1f}" if r.fps else "N/A"
                    print(f"  ← {fps_s} fps ({elapsed:.1f}s) [{r.status}]")
                    if r.status == "timeout":
                        model_timeout_count += 1

            # Skip E2E + multi if model-level benchmarks both timed out
            if model_timeout_count >= 2:
                remaining = 0
                if run_e2e and m.task in E2E_SUPPORTED_TASKS:
                    done += 1
                    remaining += 1
                if run_multi and m.task in MULTI_STREAM_SUPPORTED_TASKS:
                    done += 1
                    remaining += 1
                if remaining:
                    print(f"  [SKIP] model-level all timed out → skipping e2e/multi ({remaining} steps)")
                continue

            # ④ E2E Single-Stream
            if run_e2e and m.task in E2E_SUPPORTED_TASKS:
                key = (m.name, use_ort)
                done += 1
                print(f"  [{done}/{total}] e2e       {m.name} ORT={ort_s}", flush=True)
                existing = pipeline_index.get(key)
                if existing and not _should_retry_failed(existing, args.retry_failed):
                    print("    skip [resume]")
                else:
                    if existing:
                        print(f"    retry [{existing.get('status', 'unknown')}]", flush=True)
                    t0 = time.monotonic()
                    r = run_single_stream(m, use_ort, cfg, raw_dir)
                    elapsed = time.monotonic() - t0
                    r_dict = r.as_dict()
                    _upsert_result(
                        pipeline_results, r_dict,
                        lambda item: (item.get("model"), bool(item.get("use_ort"))),
                    )
                    pipeline_index[key] = r_dict
                    _save_result_set(pipeline_results, out_dir / "pipeline_results.csv", out_dir / "pipeline_results.json")
                    print(f"  ← {r.avg_e2e_fps:.1f} fps ({elapsed:.1f}s) [{r.status}]")

            # ⑤ Multi-Stream Sweep — directly follows E2E (already at thermal equilibrium)
            if run_multi and m.task in MULTI_STREAM_SUPPORTED_TASKS:
                done += 1
                print(f"  [{done}/{total}] multi     {m.name} ORT={ort_s}", flush=True)
                existing_multi = [
                    r for r in multi_results
                    if r.get("model") == m.name and bool(r.get("use_ort")) == use_ort
                ]
                single_stream_result = pipeline_index.get((m.name, use_ort))
                single_stream_fps = float(single_stream_result.get("avg_e2e_fps", 0.0) or 0.0) if single_stream_result else 0.0

                start_stream = _get_resume_stream_start(
                    existing_multi, cfg.fps_threshold, args.retry_failed, single_stream_fps,
                )
                retry_stream_counts = {
                    int(r.get("stream_count", 0) or 0)
                    for r in existing_multi
                    if _is_failed_result(r)
                } if args.retry_failed else set()

                if start_stream is None:
                    print("    skip [resume complete]")
                    continue

                print(f"    start_stream={start_stream} (single_fps={single_stream_fps:.1f}, threshold={cfg.fps_threshold})", flush=True)
                t0 = time.monotonic()

                def _multi_progress(stream_count: int) -> None:
                    print(f"    testing streams={stream_count}", flush=True)

                def _multi_checkpoint(result) -> None:
                    _upsert_result(
                        multi_results, result.as_dict(),
                        lambda item: (item.get("model"), bool(item.get("use_ort")), int(item.get("stream_count", 0) or 0)),
                    )
                    _save_result_set(multi_results, out_dir / "multi_stream_results.csv", out_dir / "multi_stream_results.json")

                results = run_multi_stream_sweep(
                    m, use_ort, cfg, raw_dir,
                    start_stream=start_stream,
                    existing_results=existing_multi,
                    retry_stream_counts=retry_stream_counts,
                    progress_callback=_multi_progress,
                    result_callback=_multi_checkpoint,
                    single_stream_result=single_stream_result,
                )
                elapsed = time.monotonic() - t0

                if results:
                    max_sc = max((r.stream_count for r in results), default=start_stream - 1)
                    print(f"  ← max_streams={max_sc} ({elapsed:.1f}s)")
                else:
                    print("    skip [resume complete]")

        if failure_context:
            break

    # ── Timing ────────────────────────────────────────────────────
    bench_end_time = time.time()
    bench_end_iso = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    session_elapsed_sec = bench_end_time - session_start_time
    elapsed_sec = bench_end_time - overall_start_time
    fp["timing"] = {
        "start": overall_start_iso,
        "end": bench_end_iso,
        "duration_sec": round(elapsed_sec, 1),
    }
    timing_history.append(_make_timing_history_entry(
        cfg=cfg,
        families=families,
        resume_dir=resume_dir,
        retry_failed=bool(getattr(args, "retry_failed", False)),
        start_iso=session_start_iso,
        end_iso=bench_end_iso,
        duration_sec=session_elapsed_sec,
        outcome="failed" if failure_context else "completed",
        failure_stage=failure_context.get("failure_stage") if failure_context else None,
        failure_model=failure_context.get("failure_model") if failure_context else None,
        failure_ort=failure_context.get("failure_ort") if failure_context else None,
        failure_reason=failure_context.get("failure_reason") if failure_context else None,
    ))
    fp["timing_history"] = timing_history

    # ── Save updated fingerprint with timing ──────────────────────
    env_path = out_dir / "environment.json"
    with open(env_path, "w") as _f:
        json.dump(fp, _f, indent=2)

    # ── Generate report ───────────────────────────────────────────
    report_path = out_dir / "REPORT.md"
    generate_markdown_report(fp, model_results, pipeline_results, multi_results, report_path,
                             video_infos=video_infos)
    if failure_context:
        print(f"\n[FAIL] Partial report: {report_path}")
        return 1

    print(f"\n[DONE] Report: {report_path}")

    return 0


# ── Backfill NPU temp/clock min/max from raw dxtop logs ─────────────────

def _parse_npu_log_filename(name: str) -> tuple[str, str, str] | None:
    """Extract (model, category, ort_tag) from an .npu.log filename.

    Supported patterns:
      model results:   {model}.throughput.run{N}.ort_{on|off}.npu.log
                       {model}.latency.run{N}.ort_{on|off}.npu.log
      pipeline:        {model}.e2e.single.ort_{on|off}.run{N}.npu.log
      multi-stream:    {model}.e2e.multi.ort_{on|off}.sc{S}.run{N}.npu.log

    Returns (model, category, ort_tag) where category is one of:
      'throughput', 'latency', 'e2e.single', 'e2e.multi'
    """
    stem = name.removesuffix(".npu.log")
    tokens = stem.split(".")

    # Model results: ...throughput.runN.ort_on  or  ...latency.runN.ort_on
    if len(tokens) >= 4:
        ort_tag = tokens[-1]
        if ort_tag in ("ort_on", "ort_off"):
            family = tokens[-3]
            if family in ("throughput", "latency"):
                model = ".".join(tokens[:-3])
                return (model, family, ort_tag)

    # Pipeline: ...e2e.single.ort_on.runN  or  ...e2e.multi.ort_on.scS.runN
    for i, t in enumerate(tokens):
        if t == "e2e" and i + 2 < len(tokens):
            mode = tokens[i + 1]  # single or multi
            ort_tag = tokens[i + 2]
            if ort_tag in ("ort_on", "ort_off") and mode in ("single", "multi"):
                model = ".".join(tokens[:i])
                return (model, f"e2e.{mode}", ort_tag)

    return None


def _backfill_from_raw_logs(results: list[dict], raw_dir: Path, category: str) -> int:
    """Re-parse raw .npu.log files to fill missing npu_temp/clock min/max.

    *category* selects which log files to use:
      'throughput' | 'latency' | 'e2e.single' | 'e2e.multi'

    Returns the number of records patched.
    """
    # Build lookup: (model, ort_tag) → list of .npu.log paths
    npu_logs: dict[tuple[str, str], list[Path]] = {}
    for p in sorted(raw_dir.glob("*.npu.log")):
        parsed = _parse_npu_log_filename(p.name)
        if parsed is None or parsed[1] != category:
            continue
        model, _cat, ort_tag = parsed
        npu_logs.setdefault((model, ort_tag), []).append(p)

    patched = 0
    for rec in results:
        has_temp = rec.get("npu_temp_min_c") is not None
        has_clock_max = rec.get("npu_clock_mhz_max") is not None
        if has_temp and has_clock_max:
            continue

        model = rec.get("model", "")
        ort_tag = "ort_on" if rec.get("use_ort") else "ort_off"
        logs = npu_logs.get((model, ort_tag))
        if not logs:
            continue

        all_temp_min, all_temp_max = [], []
        all_clock_min, all_clock_max = [], []
        for log_path in logs:
            try:
                raw_text = log_path.read_text(errors="replace")
            except OSError:
                continue
            parsed_vals = parse_npu_log_temp_clock(raw_text)
            if parsed_vals["npu_temp_min_c"] is not None:
                all_temp_min.append(parsed_vals["npu_temp_min_c"])
            if parsed_vals["npu_temp_max_c"] is not None:
                all_temp_max.append(parsed_vals["npu_temp_max_c"])
            if parsed_vals["npu_clock_mhz_min"] is not None:
                all_clock_min.append(parsed_vals["npu_clock_mhz_min"])
            if parsed_vals["npu_clock_mhz_max"] is not None:
                all_clock_max.append(parsed_vals["npu_clock_mhz_max"])

        changed = False
        if not has_temp and all_temp_min:
            rec["npu_temp_min_c"] = round(min(all_temp_min), 1)
            rec["npu_temp_max_c"] = round(max(all_temp_max), 1)
            changed = True
        if not has_clock_max and all_clock_max:
            rec["npu_clock_mhz_max"] = round(max(all_clock_max), 0)
            if all_clock_min:
                rec["npu_clock_mhz_min"] = round(min(all_clock_min), 0)
            changed = True
        if changed:
            patched += 1

    return patched


# ── Backfill fps_std from raw run logs ───────────────────────────────────

def _parse_run_log_filename(name: str) -> tuple[str, str, str, int | None] | None:
    """Extract (model, category, ort_tag, stream_count) from a .log filename.

    Supported patterns (excluding .npu.log and warmup):
      throughput:   {model}.throughput.run{N}.ort_{on|off}.log
      e2e.single:   {model}.e2e.single.ort_{on|off}.run{N}.log
      e2e.multi:    {model}.e2e.multi.ort_{on|off}.sc{S}.run{N}.log

    Returns (model, category, ort_tag, stream_count) or None.
    stream_count is None for throughput and e2e.single.
    """
    if name.endswith(".npu.log") or "warmup" in name:
        return None
    if not name.endswith(".log"):
        return None

    stem = name.removesuffix(".log")
    tokens = stem.split(".")

    # Throughput: {model}.throughput.run{N}.ort_{on|off}
    if len(tokens) >= 4:
        ort_tag = tokens[-1]
        if ort_tag in ("ort_on", "ort_off"):
            family = tokens[-3]
            if family == "throughput" and tokens[-2].startswith("run"):
                model = ".".join(tokens[:-3])
                return (model, "throughput", ort_tag, None)

    # Pipeline: {model}.e2e.single.ort_{on|off}.run{N}
    # Multi:    {model}.e2e.multi.ort_{on|off}.sc{S}.run{N}
    for i, t in enumerate(tokens):
        if t == "e2e" and i + 2 < len(tokens):
            mode = tokens[i + 1]
            ort_tag = tokens[i + 2]
            if ort_tag not in ("ort_on", "ort_off") or mode not in ("single", "multi"):
                continue
            model = ".".join(tokens[:i])
            sc = None
            if mode == "multi":
                for tok in tokens[i + 3:]:
                    m = re.match(r"sc(\d+)", tok)
                    if m:
                        sc = int(m.group(1))
                        break
            return (model, f"e2e.{mode}", ort_tag, sc)

    return None


def _backfill_fps_std(results: list[dict], raw_dir: Path, category: str) -> int:
    """Compute fps_std from raw run logs for results that lack it.

    *category*: 'throughput' | 'e2e.single' | 'e2e.multi'

    For throughput: parses FPS from 'FPS : xx.xx' in each run log.
    For e2e: parses 'Execution ended after H:MM:SS.nnn' and uses frame_count.

    Returns the number of records patched.
    """
    # Build lookup: key → list of (log_path, stream_count)
    # key = (model, ort_tag) for throughput/single, (model, ort_tag, sc) for multi
    run_logs: dict[tuple, list[Path]] = {}
    for p in sorted(raw_dir.glob("*.log")):
        parsed = _parse_run_log_filename(p.name)
        if parsed is None or parsed[1] != category:
            continue
        model, _cat, ort_tag, sc = parsed
        if category == "e2e.multi":
            key = (model, ort_tag, sc)
        else:
            key = (model, ort_tag)
        run_logs.setdefault(key, []).append(p)

    patched = 0
    for rec in results:
        if rec.get("fps_std") is not None:
            continue

        model = rec.get("model", "")
        ort_tag = "ort_on" if rec.get("use_ort") else "ort_off"
        if category == "e2e.multi":
            sc = rec.get("stream_count")
            key = (model, ort_tag, sc)
        else:
            key = (model, ort_tag)

        logs = run_logs.get(key)
        if not logs:
            continue

        fps_values: list[float] = []
        for log_path in logs:
            try:
                text = log_path.read_text(errors="replace")
            except OSError:
                continue

            if category == "throughput":
                # Parse FPS : xx.xx
                m = re.search(r"FPS\s*:\s*([\d.]+)", text)
                if m:
                    fps_values.append(float(m.group(1)))
            else:
                # Parse Execution ended after H:MM:SS.nnn
                m = re.search(r"Execution ended after\s+(\d+):(\d+):([\d.]+)", text)
                if m:
                    secs = int(m.group(1)) * 3600 + int(m.group(2)) * 60 + float(m.group(3))
                    frame_count = rec.get("frame_count", 0)
                    if secs > 0 and frame_count > 0:
                        fps_values.append(frame_count / secs)

        if len(fps_values) >= 2:
            rec["fps_std"] = round(statistics.stdev(fps_values), 2)
            patched += 1

    return patched


# ── Backfill multi-stream sc=1 from single-stream results ───────────────

# Fields to copy from single-stream results into sc=1 multi-stream entries
_SC1_BACKFILL_FIELDS = [
    "npu_core0_avg_pct", "npu_core0_max_pct",
    "npu_core1_avg_pct", "npu_core1_max_pct",
    "npu_core2_avg_pct", "npu_core2_max_pct",
    "npu_total_avg_pct", "npu_total_max_pct",
    "npu_mem_max_mib", "npu_samples",
    "npu_temp_min_c", "npu_temp_max_c",
    "npu_clock_mhz_min", "npu_clock_mhz_max",
    "npu_throttled",
    "decoder", "pipeline_caps", "fps_std",
]


def _backfill_sc1_from_single_stream(
    multi_results: list[dict],
    pipeline_results: list[dict],
) -> int:
    """Patch multi-stream sc=1 entries with data from single-stream results.

    When multi-stream sc=1 entries were created by an older version of
    ``_make_sc1_from_single_stream`` that didn't copy NPU stats, decoder,
    etc., those fields are None/unknown.  This function fills them in from
    the matching single-stream pipeline result so that the report shows
    correct values without re-running the benchmark.

    Returns the number of records patched.
    """
    # Build lookup: (model, use_ort) → single-stream result dict
    single_lookup: dict[tuple[str, bool], dict] = {}
    for r in pipeline_results:
        if r.get("stream_count", 1) == 1:
            key = (r["model"], bool(r.get("use_ort")))
            single_lookup[key] = r

    patched = 0
    for rec in multi_results:
        if rec.get("stream_count") != 1:
            continue

        key = (rec["model"], bool(rec.get("use_ort")))
        single = single_lookup.get(key)
        if single is None:
            continue

        changed = False
        for field in _SC1_BACKFILL_FIELDS:
            src_val = single.get(field)
            dst_val = rec.get(field)
            # Backfill if destination is missing/None or is a placeholder
            if src_val is not None and (
                dst_val is None
                or (field == "decoder" and dst_val == "unknown")
            ):
                rec[field] = src_val
                changed = True
        if changed:
            patched += 1

    return patched


def cmd_report(args: argparse.Namespace) -> int:
    """Regenerate report from existing result files."""
    result_dir = Path(args.result_dir)
    if not result_dir.exists():
        print(f"[ERROR] Result directory not found: {result_dir}")
        return 1

    fp_path = result_dir / "environment.json"
    fp = json.loads(fp_path.read_text()) if fp_path.exists() else {}

    model_results = _load_json(result_dir / "model_results.json")
    pipeline_results = _load_json(result_dir / "pipeline_results.json")
    multi_results = _load_json(result_dir / "multi_stream_results.json")

    # Backfill npu_temp/clock min/max from raw .npu.log when fields are missing
    raw_dir = result_dir / "raw"
    total_patched = 0
    fps_patched = 0
    if raw_dir.is_dir():
        for results, categories in [
            (model_results, ["throughput", "latency"]),
            (pipeline_results, ["e2e.single"]),
            (multi_results, ["e2e.multi"]),
        ]:
            for cat in categories:
                n = _backfill_from_raw_logs(results, raw_dir, cat)
                total_patched += n
        if total_patched:
            print(f"[INFO] Backfilled NPU temp/clock min/max for {total_patched} result(s) from raw logs")

        # Backfill fps_std from raw run logs when missing
        for results, categories in [
            (model_results, ["throughput"]),
            (pipeline_results, ["e2e.single"]),
            (multi_results, ["e2e.multi"]),
        ]:
            for cat in categories:
                n = _backfill_fps_std(results, raw_dir, cat)
                fps_patched += n
        if fps_patched:
            print(f"[INFO] Backfilled fps_std (σ) for {fps_patched} result(s) from raw logs")

    # Backfill multi-stream sc=1 entries from single-stream results.
    # Must run AFTER raw-log backfills so pipeline_results already has
    # npu_temp/clock and fps_std populated.
    sc1_patched = _backfill_sc1_from_single_stream(multi_results, pipeline_results)
    if sc1_patched:
        print(f"[INFO] Backfilled {sc1_patched} multi-stream sc=1 entry(ies) from single-stream results")

    # Persist backfilled data to CSV/JSON so they stay in sync with the report
    if total_patched or fps_patched or sc1_patched:
        _save_result_set(model_results,
                         result_dir / "model_results.csv",
                         result_dir / "model_results.json")
        _save_result_set(pipeline_results,
                         result_dir / "pipeline_results.csv",
                         result_dir / "pipeline_results.json")
        _save_result_set(multi_results,
                         result_dir / "multi_stream_results.csv",
                         result_dir / "multi_stream_results.json")
        print("[INFO] Updated CSV/JSON with backfilled data")

    report_path = result_dir / "REPORT.md"
    generate_markdown_report(fp, model_results, pipeline_results, multi_results, report_path,
                             video_infos=fp.get("video_infos"))
    print(f"[DONE] Report: {report_path}")
    return 0


def cmd_aggregate(args: argparse.Namespace) -> int:
    """Aggregate multiple benchmark result directories into a single dataset.json."""
    results_root = Path(args.results_root).resolve()
    output_path = Path(args.output).resolve() if args.output else results_root / "dataset.json"
    dataset = aggregate_result_directories(results_root)
    save_dataset_json(dataset, output_path)
    print(f"[DONE] Dataset: {output_path}")
    return 0


def cmd_dashboard(args: argparse.Namespace) -> int:
    """Build a static HTML dashboard from benchmark results."""
    results_root = Path(args.results_root).resolve()
    output_dir = Path(args.output).resolve() if args.output else results_root / "dashboard"
    dataset = aggregate_result_directories(results_root)
    build_static_dashboard(dataset, output_dir)
    print(f"[DONE] Dashboard: {output_dir / 'index.html'}")
    return 0


# ── Helpers ───────────────────────────────────────────────────────────────

def _build_config(args: argparse.Namespace) -> BenchmarkConfig:
    base_cfg = BenchmarkConfig()
    sizes = args.sizes.split(",") if hasattr(args, "sizes") and args.sizes else list(SIZES)
    ort_modes = [True, False]  # always both
    runs_override = getattr(args, "runs", None)
    _model_time = getattr(args, "model_time", None)
    _warmup = getattr(args, "warmup", None)
    _fps_thr = getattr(args, "fps_threshold", None)
    return BenchmarkConfig(
        task=getattr(args, "task", base_cfg.task),
        sizes=sizes,
        ort_modes=ort_modes,
        model_time_sec=_model_time if _model_time is not None else base_cfg.model_time_sec,
        model_warmup=_warmup if _warmup is not None else base_cfg.model_warmup,
        e2e_runs=runs_override if runs_override is not None else base_cfg.e2e_runs,
        model_latency_runs=runs_override if runs_override is not None else base_cfg.model_latency_runs,
        model_throughput_runs=runs_override if runs_override is not None else base_cfg.model_throughput_runs,
        video=getattr(args, "video", base_cfg.video),
        fps_threshold=_fps_thr if _fps_thr is not None else base_cfg.fps_threshold,
        output_dir=getattr(args, "output", base_cfg.output_dir),
        product_name=getattr(args, "product_name", base_cfg.product_name),
    )


def _get_models(cfg: BenchmarkConfig) -> list:
    models = discover_models()
    task_filter = None if cfg.task == "all" else cfg.task
    return filter_models(models, task=task_filter, sizes=cfg.sizes)


def _get_families(args: argparse.Namespace) -> list[str]:
    family = getattr(args, "family", "all")
    if family == "all":
        return ["all"]
    return [family]


def _count_runs(cfg: BenchmarkConfig, models: list, families: list[str]) -> int:
    n_model = len(models) * len(cfg.ort_modes)
    e2e_models = [m for m in models if m.task in E2E_SUPPORTED_TASKS]
    multi_models = [m for m in models if m.task in MULTI_STREAM_SUPPORTED_TASKS]
    n_e2e = len(e2e_models) * len(cfg.ort_modes)
    n_multi = len(multi_models) * len(cfg.ort_modes)
    total = 0
    if "model" in families or "all" in families:
        total += n_model * 2  # throughput + latency
    if "e2e" in families or "all" in families:
        total += n_e2e
    if "multi" in families or "all" in families:
        total += n_multi  # each model counts as 1 (sweep is internal)
    return total


def _load_json(path: Path) -> list[dict]:
    if path.exists():
        return json.loads(path.read_text())
    return []


def _load_json_object(path: Path) -> dict:
    if path.exists():
        return json.loads(path.read_text())
    return {}


def _parse_local_timestamp(ts: str) -> float | None:
    try:
        return datetime.strptime(ts, "%Y-%m-%d %H:%M:%S").timestamp()
    except ValueError:
        return None


def _save_result_set(results: list[dict], csv_path: Path, json_path: Path) -> None:
    save_results_csv(results, csv_path)
    save_results_json(results, json_path)


def _make_timing_history_entry(
    cfg: BenchmarkConfig,
    families: list[str],
    resume_dir: Path | None,
    retry_failed: bool,
    start_iso: str,
    end_iso: str,
    duration_sec: float,
    outcome: str = "completed",
    failure_stage: str | None = None,
    failure_model: str | None = None,
    failure_ort: str | None = None,
    failure_reason: str | None = None,
) -> dict:
    if resume_dir and retry_failed:
        mode = "retry-failed"
    elif resume_dir:
        mode = "resume"
    else:
        mode = "run"

    entry = {
        "mode": mode,
        "start": start_iso,
        "end": end_iso,
        "duration_sec": round(duration_sec, 1),
        "families": families,
        "task": cfg.task,
        "sizes": list(cfg.sizes),
        "retry_failed": retry_failed,
        "outcome": outcome,
    }

    if failure_stage:
        entry["failure_stage"] = failure_stage
    if failure_model:
        entry["failure_model"] = failure_model
    if failure_ort:
        entry["failure_ort"] = failure_ort
    if failure_reason:
        entry["failure_reason"] = failure_reason

    return entry


def _load_timing_history(fingerprint: dict) -> list[dict]:
    history = fingerprint.get("timing_history")
    if isinstance(history, list):
        return list(history)

    legacy_timing = fingerprint.get("timing")
    if not legacy_timing:
        return []

    params = fingerprint.get("benchmark_params", {})
    benchmarked_models = fingerprint.get("benchmarked_models", [])
    sizes = sorted({m.get("size") for m in benchmarked_models if m.get("size")})
    return [{
        "mode": "run",
        "start": legacy_timing.get("start"),
        "end": legacy_timing.get("end"),
        "duration_sec": legacy_timing.get("duration_sec", 0),
        "families": params.get("families", []),
        "task": "mixed",
        "sizes": sizes,
        "retry_failed": bool(params.get("retry_failed", False)),
        "outcome": "completed",
    }]


def _is_failed_result(result: dict | None) -> bool:
    if not result:
        return False
    return result.get("status") not in {"ok", "partial"}


def _should_retry_failed(result: dict | None, retry_failed: bool) -> bool:
    return bool(retry_failed and _is_failed_result(result))


def _upsert_result(results: list[dict], new_result: dict, key_func) -> None:
    new_key = key_func(new_result)
    for idx in range(len(results) - 1, -1, -1):
        if key_func(results[idx]) == new_key:
            results[idx] = new_result
            return
    results.append(new_result)


def _get_resume_stream_start(
    existing_results: list[dict],
    fps_threshold: float,
    retry_failed: bool = False,
    single_stream_fps: float = 0.0,
) -> int | None:
    if not existing_results:
        return get_boundary_search_start([], fps_threshold, single_stream_fps)

    if retry_failed:
        failed_results = [result for result in existing_results if _is_failed_result(result)]
        if not failed_results:
            return None

        # Retry search should follow the original boundary-search strategy.
        # Failed rows are retried, but they do not determine the new start point.
        search_results = [result for result in existing_results if not _is_failed_result(result)]
        return get_boundary_search_start(search_results, fps_threshold, single_stream_fps)

    return get_boundary_search_start(existing_results, fps_threshold, single_stream_fps)


# ── CLI ───────────────────────────────────────────────────────────────────

def _build_parser() -> argparse.ArgumentParser:
    defaults = BenchmarkConfig()

    parser = argparse.ArgumentParser(
        description="YOLO26 Benchmark Tool",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    sub = parser.add_subparsers(dest="command", required=True)

    # preflight
    sub.add_parser("preflight", help="Check environment readiness")

    # dry-run
    p_dry = sub.add_parser("dry-run", help="Show benchmark matrix without running")
    _add_common_args(p_dry)
    _add_benchmark_args(p_dry, defaults)

    # run
    p_run = sub.add_parser("run", help="Execute benchmarks")
    _add_common_args(p_run)
    _add_benchmark_args(p_run, defaults)

    # report
    p_report = sub.add_parser("report", help="Regenerate report from results")
    p_report.add_argument("result_dir", help="Path to result directory")

    # aggregate
    p_aggregate = sub.add_parser("aggregate", help="Aggregate multiple result directories into dataset.json")
    p_aggregate.add_argument("results_root", help="Path containing benchmark result directories")
    p_aggregate.add_argument("--output", default=None, help="Output dataset.json path")

    # dashboard
    p_dashboard = sub.add_parser("dashboard", help="Build a static dashboard from result directories")
    p_dashboard.add_argument("results_root", help="Path containing benchmark result directories")
    p_dashboard.add_argument("--output", default=None, help="Output directory for the static dashboard")

    return parser


def main() -> int:
    parser = _build_parser()

    args = parser.parse_args()

    commands = {
        "preflight": cmd_preflight,
        "dry-run": cmd_dry_run,
        "run": cmd_run,
        "report": cmd_report,
        "aggregate": cmd_aggregate,
        "dashboard": cmd_dashboard,
    }
    return commands[args.command](args)


def _add_common_args(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--task", default="all",
                        help="Task name or 'all' (default: all)")
    parser.add_argument("--sizes", default=None,
                        help="Comma-separated sizes, e.g. n,s,m (default: all)")
    parser.add_argument("--video", default=None,
                        help="Override video path (applies to all tasks)")
    parser.add_argument("--output", default=None,
                        help="Override output root directory (runs are stored under <output>/<hw_id>/<run_id>)")
    parser.add_argument("--product-name", default=None,
                        help="Product name to include in report (e.g. DX_AIPlayer-N97)")


def _add_benchmark_args(parser: argparse.ArgumentParser, defaults: BenchmarkConfig) -> None:
    parser.add_argument("--family", choices=["all", "model", "e2e", "multi"],
                        default="all", help="Benchmark family (default: all)")
    parser.add_argument("--model-time", type=int, default=None,
                        help=f"Duration of model benchmark in seconds (default: {defaults.model_time_sec})")
    parser.add_argument("--warmup", type=int, default=None,
                        help=f"Warmup runs (default: {defaults.model_warmup})")
    parser.add_argument("--runs", type=int, default=None,
                        help=f"Measured repetitions for model and E2E benchmarks (default: {defaults.e2e_runs})")
    parser.add_argument("--fps-threshold", type=float, default=None,
                        help=f"Per-channel FPS threshold for multi-stream sweep (default: {defaults.fps_threshold:g})")
    parser.add_argument("--resume", default=None,
                        help="Resume and extend an existing result directory instead of starting a new run")
    parser.add_argument("--retry-failed", action="store_true",
                        help="With --resume, rerun only entries whose latest status is not ok/partial")


if __name__ == "__main__":
    sys.exit(main())
