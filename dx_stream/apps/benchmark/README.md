# YOLO26 Benchmark Tool

Unified benchmarking tool for YOLO26 models on DEEPX NPU.
Produces reproducible performance measurements across any Host PC + NPU combination using a standardized procedure.

## Key Features

- **Model-Level Benchmarks**: NPU inference engine throughput/latency measurement (direct `run_model` execution)
- **E2E Pipeline Benchmarks**: Full GStreamer pipeline FPS measurement via DX-STREAM (Single-Stream)
- **Multi-Stream Benchmarks**: Boundary channel count search based on single-stream FPS (1ch reuses single-stream results)
- **NPU Temperature/Clock Monitoring**: Automatic logging of NPU temperature, utilization, and clock (MHz) via dxtop + dxrt-cli pre/post snapshots for throttle detection
- **CPU/NPU Clock Tracking**: Records NPU MHz and CPU MHz to track changes (min/max)
- **Thermal Steady-State Normalization**: Rejects start above 60°C, per-model cooldown targeting `min(idle + Δ10°C, 55°C)`, benchmark failure on cooldown timeout
- **Automatic ORT ON/OFF Comparison**: Both modes measured automatically in every benchmark
- **Environment Fingerprinting**: Automatic capture of measurement context for reproducibility
- **Markdown Report Generation**: Result tables + ORT comparison + channel capacity summary
- **Static Dashboard**: HTML dashboard for comparing results across multiple environments
- **Version Trend Tracking**: Line charts comparing performance changes across SDK versions for the same HW_ID
- **Resume / Retry-Failed**: Continue from interrupted runs or rerun only failed conditions

## Supported Tasks

| Task | Model-Level | E2E Pipeline | Multi-Stream |
|------|:-----------:|:------------:|:------------:|
| Object Detection | O | O | O |
| Pose Estimation | O | O | O |
| Segmentation | O | O | O |
| Oriented BBox (OBB) | O | O | O |
| Classification | O | O | — |

> OBB models use 1024x1024 input, Classification uses 224x224 (keep-ratio=false), all others use 640x640.
> Classification Multi-Stream is excluded: Classification is less representative of common E2E multi-stream usage scenarios.

## Directory Structure

```
benchmark/
├── __init__.py                # Package init
├── __main__.py                # CLI entry point
├── config.py                  # Configuration, paths, task definitions
├── env_fingerprint.py         # Environment info collection (model metadata included)
├── model_catalog.py           # .dxnn model discovery and classification
├── runner_model.py            # Model-level benchmarks (throughput/latency)
├── runner_pipeline.py         # E2E Pipeline / Multi-Stream benchmarks
├── npu_monitor.py             # NPU utilization/temperature/clock monitoring (dxtop/dxrt-cli)
├── npu_stats_util.py          # NPU stats merge utilities
├── reporter.py                # CSV/JSON/Markdown report generation
├── aggregator.py              # Multi-result aggregation → dataset.json
├── result_layout.py           # HW_ID and nested result layout helpers
├── dashboard_builder.py       # Static HTML dashboard generation
├── model_list.json            # Benchmark target model list (auto-download reference)
├── setup_benchmark_env.sh     # One-time environment setup (sudoers, etc.)
├── setup_benchmark_models.sh  # Benchmark model download
├── setup_benchmark_videos.sh  # Benchmark video download
├── ANALYSIS_KOR.md            # Benchmark analysis report (Korean)
├── ANALYSIS_EN.md             # Benchmark analysis report (English)
├── README.md                  # README

├── assets/
│   ├── models/                # .dxnn model files
│   └── videos/                # Benchmark input videos
└── tests/
    ├── test_aggregator.py
    ├── test_asset_setup.py
    ├── test_result_layout.py
    └── test_thermal_cooldown.py
```

## Usage

### 1. Environment Check

```bash
cd /path/to/dx_stream
python3 -m dx_stream.apps.benchmark preflight
```

### 2. Dry-Run (Preview Matrix)

```bash
python3 -m dx_stream.apps.benchmark dry-run
python3 -m dx_stream.apps.benchmark dry-run --sizes n,s --task object_detection
```

### 3. Run Benchmarks

```bash
# Full suite (model + e2e + multi-stream)
python3 -m dx_stream.apps.benchmark run

# Run by family
python3 -m dx_stream.apps.benchmark run --family model
python3 -m dx_stream.apps.benchmark run --family e2e
python3 -m dx_stream.apps.benchmark run --family multi

# Limit sizes / time
python3 -m dx_stream.apps.benchmark run --sizes n,s --family model --model-time 30

# Resume interrupted run
python3 -m dx_stream.apps.benchmark run --resume results/BIOSTAR_H1/20260421_201621

# Retry failed conditions only
python3 -m dx_stream.apps.benchmark run --resume results/BIOSTAR_H1/20260421_201621 --retry-failed
```

### 4. Regenerate Report

```bash
# Specify a {hw_id}/{run_id} result directory path
python3 -m dx_stream.apps.benchmark report dx_stream/apps/benchmark/results/BIOSTAR_H1/20260421_201621
```

### 5. Aggregate Results

```bash
# Aggregate multiple environment/run results into a single dataset.json
python3 -m dx_stream.apps.benchmark aggregate dx_stream/apps/benchmark/results
python3 -m dx_stream.apps.benchmark aggregate dx_stream/apps/benchmark/results --output /tmp/dataset.json
```

### 6. Build Dashboard

```bash
# Aggregate + generate dashboard
python3 -m dx_stream.apps.benchmark dashboard dx_stream/apps/benchmark/results
# → generates index.html, app.js, styles.css, dataset.json under results/dashboard/

# Custom output directory
python3 -m dx_stream.apps.benchmark dashboard dx_stream/apps/benchmark/results --output /tmp/dashboard

# Local preview
cd dx_stream/apps/benchmark/results/dashboard && python3 -m http.server 8899
```

Pure HTML/CSS/JS with no external CDN — works fully offline.

**Dashboard Tabs:**

| Tab | Description |
|-----|-------------|
| E2E FPS Overview | E2E FPS comparison chart by Task/ORT (grouped bars by model size). Max Ch badge displayed above E2E FPS. |
| Full Metrics | Cross-environment comparison of NPU Latency, Throughput, E2E FPS per Task/Size/ORT. Latency shown as dashed line on secondary Y-axis. |
| Detailed Data | Full numeric table with Environment/Task/ORT filters. Run ID dropdown for specific run selection. |
| Version Trend | SDK version performance trend line charts from nested result history. Metrics dropdown: Latency/Throughput/E2E FPS/Max Channel. |

### 7. Version Trend Tracking

Compare benchmark results before and after SDK updates using the same HW_ID.

**Workflow:**

```bash
# (1) Run benchmark on each environment
python3 -m dx_stream.apps.benchmark run

# (2) Update SDK, run again on the same HW
python3 -m dx_stream.apps.benchmark run

# (3) Generate dashboard from nested results root → check Version Trend tab
python3 -m dx_stream.apps.benchmark dashboard dx_stream/apps/benchmark/results
```

Results always follow the `results/{hw_id}/{run_id}/` structure. HW_ID is automatically computed from the `environment.json` fingerprint during `run`.

- With `--product-name`: `{product_name}_{hw_config}` (e.g., `DX-AIPlayer-N97_M1`)
- Without: `{hostname}_{hw_config}` (e.g., `RPi_M1`)

**Result Directory Structure:**

```
results/
├── DX-AIPlayer-N97_M1/          # When --product-name is used
│   ├── 20260403_174607/
│   │   └── (result files)
│   └── 20260409_165331/
│       └── (result files)
├── BIOSTAR_H1/                   # Hostname-based (default)
│   └── 20260409_170451/
│       └── (result files)
├── RPi_M1/
│   └── 20260410_120000/
│       └── (result files)
└── dashboard/                    # Static dashboard files
    ├── index.html
    ├── app.js
    ├── styles.css
    └── dataset.json
```

**Version Trend Tab:**

- Environment / Task / ORT / Metrics filters for condition selection
- Metrics dropdown: Latency, Throughput, E2E FPS, Max Channel
- X-axis: snapshot date, Y-axis: selected metric
- Per-size (N/S/M/L/X) line charts
- Automatic label de-overlap, selected column highlight (white halo + black text)
- Click a point to view the snapshot's environment details (Host PC / NPU / Tools)

## CLI Options

### `run` / `dry-run` Common Options

| Option | Default | Description |
|--------|---------|-------------|
| `--task` | `all` | Task type (`all`, `object_detection`, `pose_estimation`, `segmentation`, `oriented_bbox`, `classification`) |
| `--sizes` | `n,s,m,l,x` | Model sizes to measure (comma-separated) |
| `--family` | `all` | Benchmark family: `model`, `e2e`, `multi`, `all` |
| `--model-time` | 30 | Model-level throughput measurement duration (seconds, `run_model -t`). Latency uses fixed 300 loops (`-l`) |
| `--warmup` | 1 | Warmup run count |
| `--runs` | — | Override all repetition counts to the same value. Defaults: latency=1, throughput=3, e2e=3 |
| `--fps-threshold` | 30 | Per-channel minimum FPS threshold for multi-stream |
| `--video` | Auto per task | Override input video path (applied to all tasks) |
| `--output` | `dx_stream/apps/benchmark/results/` | Output root directory. Actual output: `<output>/<hw_id>/<run_id>/` |
| `--resume` | — | Resume from an existing result directory |
| `--retry-failed` | — | With `--resume`, rerun only entries not in `ok`/`partial` status |
| `--product-name` | — | Product name. Used in HW_ID instead of hostname (e.g., `DX-AIPlayer-N97`) |

### Subcommands

| Command | Description |
|---------|-------------|
| `preflight` | Check tool availability + print environment fingerprint |
| `dry-run` | Preview benchmark matrix (no execution) |
| `run` | Execute benchmarks |
| `report <result_dir>` | Regenerate Markdown report from existing results |
| `aggregate <results_root> [--output PATH]` | Aggregate results into dataset.json |
| `dashboard <results_root> [--output DIR]` | Generate static HTML dashboard |

## Output Structure

```
results/{hw_id}/{run_id}/
├── environment.json              # Environment fingerprint + timing + timing_history
├── model_results.csv/json        # Model-level results (throughput + latency)
├── pipeline_results.csv/json     # E2E single-stream results
├── multi_stream_results.csv/json # Multi-stream boundary search results
├── REPORT.md                     # Comprehensive Markdown report
├── raw/                          # Raw logs (.log + .npu.log + profiler.json)
└── incidents/                    # Timeout diagnostic snapshots (when applicable)
```

## Resume vs Retry-Failed

| Scenario | Command |
|----------|---------|
| Interrupted → continue unfinished combinations | `--resume <dir>` |
| Rerun only failed conditions | `--resume <dir> --retry-failed` |
| Fresh measurement | New result directory via `run` |

## Measurement Protocol

| Parameter | Value |
|-----------|-------|
| Throughput duration (`-t`) | 30s |
| Latency loops (`-l`) | 300 loops (`run_model -s` mode ignores `-t`, uses `-l` only) |
| Warmup | 1 run |
| Latency runs | 1 |
| Throughput runs | 3 |
| E2E runs | 3 (uniform across all tasks) |
| ORT modes | ON + OFF |
| Thermal mode | steady |
| Hot-start block | 60°C (benchmark start rejected if exceeded) |
| Cooldown target | `min(idle + Δ10°C, 55°C)` |
| Cooldown timeout | 300s (RuntimeError on exceed) |
| NPU warmup | 1.0s |
| NPU drain | 0.5s |
| NPU clock monitoring | dxtop Core Clock MHz (during measurement) + dxrt-cli pre/post snapshots |
| CPU clock monitoring | sysfs scaling_cur_freq pre/post snapshots |
| Multi-stream 1ch | Reuses single-stream result |
| Multi-stream max streams | 128 (safety cap) |
| Process timeout | 600s/run |
| Graceful shutdown | SIGTERM → 10s wait → SIGKILL (2-phase) |
| Pipeline retry | 1 automatic retry each for warmup + measured run |
| NPU recovery | Automatic dxrt.service restart after SIGKILL |

## Per-Model Execution Order (Thermal Normalization)

Each model × ORT combination follows these steps sequentially.
This ordering ensures the NPU warms up naturally from cold state.

```
── [1/N] yolo26-n_640x640.dxnn  ORT=ON  (object_detection) ──

  ① Cooldown → Rejects start above 60°C. Wait until ≤ min(idle + Δ10°C, 55°C) (when family=model is included)
  ② Latency  → Single-core sync mode (-l 300 loops), profiler-based NPU/CPU ms
                (cold state → NPU DVFS stabilization begins)
  ③ Throughput → Multi-core async mode, FPS measurement (3 runs)
                 (sustained NPU load → natural temperature convergence)
  ④ E2E Single-Stream → Full GStreamer pipeline FPS measurement (3 runs)
  ⑤ Multi-Stream Sweep → Start point estimation from single-stream FPS, then boundary search

→ Proceed to next model × ORT combination (repeat from ①)
```

**Design Rationale:**

- ② Latency runs from cold state. The profiler accurately measures NPU/CPU time separation with minimal temperature impact.
- ③ Throughput runs 30s × 3 consecutive runs to sufficiently heat the NPU.
- ④ By E2E measurement time, NPU temperature has nearly converged (steady state) after ②+③.
- ⑤ Multi-stream runs immediately after E2E, maintaining thermal equilibrium without additional cooldown.
- ① Cooldown is only performed when `thermal_mode=steady` and model family is included.
- If cooldown times out (300s), the run fails with RuntimeError.
- If both latency and throughput time out, E2E/Multi-Stream phases are automatically skipped for that model.

## Timeout Recovery and Retry Strategy

During benchmark execution, GStreamer pipelines or run_model processes may become
unresponsive (deadlock, NPU hang). A 3-layer recovery structure handles these cases.

### Layer 1: Graceful Shutdown (SIGTERM → SIGKILL)

When a process exceeds the 600s timeout:

1. **SIGTERM** sent to the entire process group → up to 10s wait for graceful exit
2. If not terminated by SIGTERM → **SIGKILL** forced termination
3. If SIGKILL was needed → NPU device recovery (see below)

> Force-killing gst-launch-1.0 via SIGKILL destroys dxrtd's IPC message queue
> (Error 43: Identifier removed). All subsequent NPU inference fails in this state,
> making device recovery mandatory.

### Layer 2: Python-Level Retry (Per Pipeline)

Handles transient deadlocks in individual pipelines/models:

| Phase | Retries | Notes |
|-------|:-------:|-------|
| Model warmup (latency/throughput) | 0 | Immediate failure return on timeout |
| Model measured run | 0 | Skips timed-out run, averages remaining runs |
| E2E warmup | 1 | 1 retry on TIMEOUT |
| E2E measured run | 1/run | Max 1 retry per run index |
| Multi-stream warmup | 1 | GStreamer dxinputselector init deadlock mitigation |
| Multi-stream measured run | 1/run | Max 1 retry per run index |
| Multi-stream sweep | 1/channel | Max 1 retry per stream count |

When both model-level (latency + throughput) time out consecutively,
E2E and multi-stream phases are automatically skipped for that model × ORT combination.

### Incident Diagnostic Collection

On timeout, diagnostic snapshots are saved to the `incidents/` directory:

- `dxrt-cli -s` NPU status
- `systemctl status dxrt.service` service status
- `journalctl` recent 100 lines / `dmesg` recent 200 lines
- Process tree dump (`ps`)
- NPU temperature/clock snapshot

### NPU Device Recovery

Automatically triggered when SIGKILL was required:

1. `pkill -9 gst-launch-1.0` — clean up orphaned pipeline processes
2. `sudo -n systemctl restart dxrt.service` — restart NPU runtime daemon (3s settle)
3. Same procedure for run_model timeout (`pkill -9 run_model` + service restart)

> Passwordless sudo required: run `setup_benchmark_env.sh` or manually add the
> following rules to `/etc/sudoers.d/benchmark-dxrt`:
> ```
> user ALL=(ALL) NOPASSWD: /usr/bin/systemctl restart dxrt.service
> user ALL=(ALL) NOPASSWD: /usr/bin/dmesg *
> user ALL=(ALL) NOPASSWD: /usr/bin/journalctl *
> ```
