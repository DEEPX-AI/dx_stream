"""Build a static HTML dashboard backed by dataset.json."""

from __future__ import annotations

import json
from pathlib import Path

from .aggregator import save_dataset_json


INDEX_HTML = r"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>YOLO26 Benchmark Dashboard</title>
  <link rel="stylesheet" href="styles.css" />
</head>
<body data-active-tab="fps-compare">
  <main class="page">
    <header class="hero">
      <div>
        <p class="eyebrow">YOLO26 Benchmark</p>
        <h1>Performance Dashboard</h1>
        <p class="lede">Compare model performance across environments at a glance.</p>
      </div>
      <div id="meta" class="meta-card"></div>
    </header>

    <nav class="tabs">
      <button class="tab active" data-tab="fps-compare">E2E FPS Overview</button>
      <button class="tab" data-tab="overview">Full Metrics</button>
      <button class="tab" data-tab="detail">Detailed Data</button>
      <button class="tab" data-tab="version-trend">Version Trend</button>
    </nav>

    <!-- FPS Overview Tab -->
    <div id="tab-fps-compare" class="tab-content active">
      <section class="controls run-selector-panel">
        <div class="control-heading">Run Selection by Environment</div>
        <div id="fpsRunSelectors" class="run-selector-grid"></div>
      </section>
      <section class="controls">
        <label>Task
          <select id="fpsTaskFilter">
            <option value="object_detection">OD (Object Detection)</option>
            <option value="pose_estimation">Pose (Pose Estimation)</option>
            <option value="segmentation">Seg (Segmentation)</option>
            <option value="oriented_bbox">OBB (Oriented BBox)</option>
            <option value="classification">Cls (Classification)</option>
          </select>
        </label>
        <label>ORT
          <select id="fpsOrtFilter">
            <option value="on">ON</option>
            <option value="off">OFF</option>
          </select>
        </label>
      </section>

      <section class="panel chart-panel">
        <div class="chart-header">
          <h2 id="fpsChartTitle">E2E FPS by Model Size</h2>
          <p class="chart-subtitle" id="fpsChartSubtitle"></p>
        </div>
        <div class="chart-container">
          <canvas id="fpsCompareChart"></canvas>
        </div>
        <p class="chart-hint" id="fpsChartHint">Click on a bar group to view environment details</p>
      </section>

      <section class="panel env-detail" id="fpsEnvDetail" style="display:none">
        <h2 id="fpsEnvDetailTitle">Environment Details</h2>
        <div class="env-detail-grid env-detail-grid--3">
          <div class="env-detail-col">
            <h3>Host PC</h3>
            <div id="fpsEnvHostInfo"></div>
          </div>
          <div class="env-detail-col">
            <h3>NPU</h3>
            <div id="fpsEnvNpuInfo"></div>
          </div>
          <div class="env-detail-col">
            <h3>Tools</h3>
            <div id="fpsEnvToolsInfo"></div>
          </div>
        </div>
      </section>

      <section class="panel" id="fpsModelMetaPanel" style="display:none">
        <h2 id="fpsModelMetaTitle">Benchmarked Models</h2>
        <div id="fpsModelMetaSection"></div>
      </section>

      <section class="panel" id="fpsE2eTableSection" style="display:none">
        <h2 id="fpsE2eTableTitle">E2E Pipeline (Single-Stream)</h2>
        <div id="fpsE2eTableContent"></div>
      </section>
    </div>

    <!-- Full Metrics Tab -->
    <div id="tab-overview" class="tab-content">
      <section class="controls run-selector-panel">
        <div class="control-heading">Run Selection by Environment</div>
        <div id="overviewRunSelectors" class="run-selector-grid"></div>
      </section>
      <section class="controls">
        <label>Task
          <select id="taskFilter">
            <option value="object_detection">OD (Object Detection)</option>
            <option value="pose_estimation">Pose (Pose Estimation)</option>
            <option value="segmentation">Seg (Segmentation)</option>
            <option value="oriented_bbox">OBB (Oriented BBox)</option>
            <option value="classification">Cls (Classification)</option>
          </select>
        </label>
        <label>Size
          <select id="sizeFilter">
            <option value="n">N (Nano)</option>
            <option value="s">S (Small)</option>
            <option value="m">M (Medium)</option>
            <option value="l">L (Large)</option>
            <option value="x">X (Extra-Large)</option>
          </select>
        </label>
        <label>ORT
          <select id="ortFilter">
            <option value="on">ON</option>
            <option value="off">OFF</option>
          </select>
        </label>
      </section>

      <section class="panel chart-panel">
        <div class="chart-header">
          <h2 id="chartTitle">Environment Performance Comparison</h2>
          <p class="chart-subtitle" id="chartSubtitle"></p>
        </div>
        <div class="chart-container">
          <canvas id="mainChart"></canvas>
        </div>
        <p class="chart-hint" id="chartHint">Click on a bar to view environment details</p>
      </section>

      <section class="panel env-detail" id="envDetail" style="display:none">
        <h2 id="envDetailTitle">Environment Details</h2>
        <div class="env-detail-grid env-detail-grid--3">
          <div class="env-detail-col">
            <h3>Host PC</h3>
            <div id="envHostInfo"></div>
          </div>
          <div class="env-detail-col">
            <h3>NPU</h3>
            <div id="envNpuInfo"></div>
          </div>
          <div class="env-detail-col">
            <h3>Tools</h3>
            <div id="envToolsInfo"></div>
          </div>
        </div>
      </section>

      <section class="panel" id="overviewModelMetaPanel" style="display:none">
        <h2 id="overviewModelMetaTitle">Benchmarked Models</h2>
        <div id="overviewModelMetaSection"></div>
      </section>
    </div>

    <!-- Detailed Data Tab -->
    <div id="tab-detail" class="tab-content">
      <section class="controls">
        <label>Environment <select id="detailEnvFilter"></select></label>
        <label>Run ID <select id="detailRunFilter"></select></label>
        <label>Task
          <select id="detailTaskFilter">
            <option value="all">All Tasks</option>
            <option value="object_detection">OD (Object Detection)</option>
            <option value="pose_estimation">Pose (Pose Estimation)</option>
            <option value="segmentation">Seg (Segmentation)</option>
            <option value="oriented_bbox">OBB (Oriented BBox)</option>
            <option value="classification">Cls (Classification)</option>
          </select>
        </label>
        <label>ORT
          <select id="detailOrtFilter">
            <option value="all">All</option>
            <option value="on">ON</option>
            <option value="off">OFF</option>
          </select>
        </label>
      </section>

      <section class="panel">
        <div id="detailTables"></div>
      </section>
    </div>

    <!-- Version Trend Tab -->
    <div id="tab-version-trend" class="tab-content">
      <section class="controls">
        <label>Environment <select id="trendEnvFilter"></select></label>
        <label>Task
          <select id="trendTaskFilter">
            <option value="object_detection">OD (Object Detection)</option>
            <option value="pose_estimation">Pose (Pose Estimation)</option>
            <option value="segmentation">Seg (Segmentation)</option>
            <option value="oriented_bbox">OBB (Oriented BBox)</option>
            <option value="classification">Cls (Classification)</option>
          </select>
        </label>
        <label>ORT
          <select id="trendOrtFilter">
            <option value="on">ON</option>
            <option value="off">OFF</option>
          </select>
        </label>
        <label>Metrics
          <select id="trendMetricFilter">
            <option value="latency">Latency</option>
            <option value="throughput">Throughput</option>
            <option value="e2e" selected>E2E FPS</option>
            <option value="capacity">Max Channel</option>
          </select>
        </label>
      </section>

      <section class="panel chart-panel">
        <div class="chart-header">
          <h2 id="trendChartTitle">E2E FPS (Single-Channel) Trend</h2>
          <p class="chart-subtitle" id="trendChartSubtitle"></p>
        </div>
        <div class="chart-container">
          <canvas id="trendChart"></canvas>
        </div>
        <p class="chart-hint" id="trendChartHint">Click on a data point group to view snapshot environment details</p>
      </section>

      <section class="panel env-detail" id="trendEnvDetail" style="display:none">
        <h2 id="trendEnvDetailTitle">Environment Details</h2>
        <div class="env-detail-grid env-detail-grid--3">
          <div class="env-detail-col">
            <h3>Host PC</h3>
            <div id="trendEnvHostInfo"></div>
          </div>
          <div class="env-detail-col">
            <h3>NPU</h3>
            <div id="trendEnvNpuInfo"></div>
          </div>
          <div class="env-detail-col">
            <h3>Tools</h3>
            <div id="trendEnvToolsInfo"></div>
          </div>
        </div>
      </section>

      <section class="panel" id="trendModelMetaPanel" style="display:none">
        <h2 id="trendModelMetaTitle">Benchmarked Models</h2>
        <div id="trendModelMetaSection"></div>
      </section>
    </div>
  </main>

  <script id="embedded-dataset" type="application/json">__DATASET_JSON__</script>
  <script src="app.js"></script>
</body>
</html>
"""


APP_JS = r"""/* ===== Constants ===== */
var TASK_MAP = {
  object_detection:  { label: 'Object Detection',    suffix: '1',    short: 'OD'   },
  pose_estimation:   { label: 'Pose Estimation',     suffix: 'pose', short: 'Pose' },
  segmentation:      { label: 'Segmentation',        suffix: 'seg',  short: 'Seg'  },
  oriented_bbox:     { label: 'Oriented BBox (OBB)', suffix: 'obb',  short: 'OBB'  },
  classification:    { label: 'Classification',      suffix: 'cls',  short: 'Cls'  },
};
var TASK_KEYS  = ['object_detection','pose_estimation','segmentation','oriented_bbox','classification'];
var TASK_ORDER = { object_detection: 0, pose_estimation: 1, segmentation: 2, oriented_bbox: 3, classification: 4 };
var SIZE_ORDER  = { n: 0, s: 1, m: 2, l: 3, x: 4 };
var SIZE_KEYS   = ['n', 's', 'm', 'l', 'x'];
var SIZE_LABELS = { n: 'Nano', s: 'Small', m: 'Medium', l: 'Large', x: 'X-Large' };
var SIZE_COLORS = {
  n: { fill: 'rgba(59,130,246,0.70)',  hi: 'rgba(59,130,246,0.95)',  line: 'rgb(59,130,246)',  dim: 'rgba(59,130,246,0.25)' },
  s: { fill: 'rgba(16,185,129,0.70)',  hi: 'rgba(16,185,129,0.95)',  line: 'rgb(16,185,129)',  dim: 'rgba(16,185,129,0.25)' },
  m: { fill: 'rgba(245,158,11,0.70)',  hi: 'rgba(245,158,11,0.95)',  line: 'rgb(245,158,11)',  dim: 'rgba(245,158,11,0.25)' },
  l: { fill: 'rgba(239,68,68,0.70)',   hi: 'rgba(239,68,68,0.95)',   line: 'rgb(239,68,68)',   dim: 'rgba(239,68,68,0.25)' },
  x: { fill: 'rgba(139,92,246,0.70)',  hi: 'rgba(139,92,246,0.95)',  line: 'rgb(139,92,246)',  dim: 'rgba(139,92,246,0.25)' },
};
var TREND_METRICS = [
  { key: 'latency', title: 'Model Latency Trend', metricLabel: 'Model Latency', axisLabel: 'Latency (ms)', resultKind: 'model', family: 'latency', valueKey: 'latency_ms', precision: 2 },
  { key: 'throughput', title: 'Model Throughput Trend', metricLabel: 'Model Throughput', axisLabel: 'Throughput (FPS)', resultKind: 'model', family: 'throughput', valueKey: 'fps', precision: 1 },
  { key: 'e2e', title: 'E2E FPS (Single-Channel) Trend', metricLabel: 'E2E FPS (Single-Channel)', axisLabel: 'E2E FPS', resultKind: 'e2e_single', valueKey: 'avg_e2e_fps', precision: 1 },
  { key: 'capacity', title: 'Max Channel Trend', metricLabel: 'Max Channel', axisLabel: 'Max Channel', resultKind: 'e2e_multi_capacity', valueKey: 'capacity_streams', precision: 0 },
];

function sizeOrd(key) { var v = SIZE_ORDER[key]; return v !== undefined ? v : 99; }

/* ===== State ===== */
var state = {
  dataset: null,
  task: 'object_detection', size: 'n', ort: true,
  selectedEnvId: null, chartData: [],
  fpsTask: 'object_detection', fpsOrt: true,
  fpsSelectedEnvId: null, fpsChartData: [],
  selectedRunIds: {},
  detailEnvId: null, detailTask: 'all', detailOrt: 'all',
  trendHwId: null, trendTask: 'object_detection', trendOrt: true, trendMetric: 'e2e',
  trendData: [], trendSelectedIdx: -1, trendChart: null,
};

/* ===== Helpers ===== */
function getModelName(task, size) { return 'yolo26' + size + '-' + TASK_MAP[task].suffix + '.dxnn'; }
function envLabel(env) { return (env.hw_id || env.hostname) + '\n(' + (env.npu_sku || '?') + ')'; }
function fmt(v, d) { if (v === null || v === undefined) return '-'; var n = Number(v); return Number.isNaN(n) ? '-' : n.toFixed(d === undefined ? 1 : d); }
function modelSizeChar(name) { var m = String(name||'').match(/yolo26([nslmx])/i); return m ? m[1].toLowerCase() : ''; }
function formatInputShape(shape) {
  if (!shape || !Array.isArray(shape)) return '-';
  if (shape.length === 4) return shape[1] + '\u00d7' + shape[2];
  return shape.join('\u00d7');
}
function formatMemMB(mb) { if (mb == null) return '-'; return Number(mb).toFixed(1); }
function escHtml(s) { var d = document.createElement('div'); d.textContent = s; return d.innerHTML; }
function _fmtTemp(lo,hi){if(lo==null&&hi==null)return'-';var a=lo!=null?Math.round(lo):'?';var b=hi!=null?Math.round(hi):'?';return a===b?String(a):a+'~'+b;}
function _fmtClock(lo,hi){if(lo==null&&hi==null)return'\u2014';var a=lo!=null?Math.round(lo):'?';var b=hi!=null?Math.round(hi):'?';return a===b?String(a):a+'~'+b;}
function stripAnsi(s) { return typeof s === 'string' ? s.replace(/\x1b\[[0-9;]*m/g, '') : s; }
function _history(kind){return ((state.dataset.history||{})[kind])||((state.dataset.summaries||{})[kind])||[];}
/* Selected-run comparisons are driven from state.dataset.history.e2e_single and friends. */
function _envById(envId){return (state.dataset.environments||[]).find(function(env){return env.env_id===envId;})||null;}
function _getRunOptions(envId){var rows=(state.dataset.runs||[]).filter(function(r){return r.env_id===envId;});rows.sort(function(a,b){return (b.run_id||'').localeCompare(a.run_id||'');});return rows;}
function _getSelectedRunId(envId){var env=_envById(envId);return state.selectedRunIds[envId]||(env?env.latest_run_id:null);}
function _initSelectedRunIds(){(state.dataset.environments||[]).forEach(function(env){state.selectedRunIds[env.env_id]=env.latest_run_id;});}
function _syncRunSelectors(envId,runId){document.querySelectorAll('select[data-run-env]').forEach(function(sel){if(sel.dataset.runEnv===envId)sel.value=runId;});}
function syncDetailRunFilter(){
  var sel=document.getElementById('detailRunFilter');if(!sel)return;
  var envId=state.detailEnvId;var runs=envId?_getRunOptions(envId):[];
  sel.innerHTML=runs.map(function(run){return '<option value="'+escHtml(run.run_id)+'">'+escHtml(run.run_id)+'</option>';}).join('');
  sel.disabled=!runs.length;
  if(runs.length){sel.value=_getSelectedRunId(envId)||runs[0].run_id;}
}
function _handleRunSelectionChange(envId,runId){state.selectedRunIds[envId]=runId;_syncRunSelectors(envId,runId);refreshFpsCompare(envId);refreshChart(envId);if(state.detailEnvId===envId){syncDetailRunFilter();renderDetailTables();}}
function renderRunSelectors(targetId){
  var target=document.getElementById(targetId);if(!target)return;
  var envs=state.dataset.environments||[];
  if(!envs.length){target.innerHTML='<p class="empty-state small">No environments available.</p>';return;}
  target.innerHTML=envs.map(function(env){
    var runs=_getRunOptions(env.env_id);if(!runs.length)return '';
    var options=runs.map(function(run){return '<option value="'+escHtml(run.run_id)+'">'+escHtml(run.run_id)+'</option>';}).join('');
    return '<label><span>'+escHtml(env.hostname)+' ('+escHtml(env.npu_sku||'?')+')</span><select data-run-env="'+escHtml(env.env_id)+'">'+options+'</select></label>';
  }).join('');
  target.querySelectorAll('select[data-run-env]').forEach(function(sel){
    var envId=sel.dataset.runEnv;sel.value=_getSelectedRunId(envId)||sel.value;
    sel.addEventListener('change',function(){_handleRunSelectionChange(envId,this.value);});
  });
}

/* ===== Dataset ===== */
function loadEmbeddedDataset() {
  var el = document.getElementById('embedded-dataset'); if (!el) return null;
  try { var t = el.textContent.trim(); return (!t || t === '__DATASET_JSON__') ? null : JSON.parse(t); } catch(e) { return null; }
}
async function loadDataset() {
  var ds = loadEmbeddedDataset(); if (ds) return ds;
  var r = await fetch('dataset.json'); if (r.ok) return r.json();
  throw new Error('Cannot load dataset.');
}

/* ===== Environment Info Renderer ===== */
function _infoRows(r) { return r.map(function(row) { return '<div class="info-row"><span class="info-key">'+escHtml(row[0])+'</span><span class="info-val">'+escHtml(String(row[1]||'-'))+'</span></div>'; }).join(''); }
function cleanVer(v) { if (typeof v !== 'string') return v; return v.replace(/^DXRT\s+/i,'').replace(/^v(?=\d)/i,''); }
function renderHostInfo(el, env) {
  var rows = [['Hostname',env.hostname],['OS',env.os],['Kernel',env.kernel],['Architecture',env.arch],['CPU',env.cpu],['CPU Cores',env.cpu_count],['RAM',env.ram_gb?env.ram_gb+' GB':'-']];
  el.innerHTML = _infoRows(rows);
}
function renderToolsInfo(el, env) {
  var rows = [['dx_stream',cleanVer(env.dx_stream_version)||'-'],['GStreamer',cleanVer(env.gstreamer_version)||'-']];
  el.innerHTML = _infoRows(rows);
}
function renderNpuInfo(el, env) {
  var rows = [['Product',env.npu_sku||'-'],['DXRT',cleanVer(env.rt_version)],['RT Driver',cleanVer(env.rt_driver)],['PCIe Driver',cleanVer(env.pcie_driver)],['Firmware',cleanVer(env.firmware)],['Clock',env.npu_clock_mhz?env.npu_clock_mhz+' MHz':'-'],['Memory',env.memory],['Board',(env.board && env.board !== 'unknown') ? env.board : '-'],['PCIe',env.pcie]];
  el.innerHTML = _infoRows(rows);
}

/* ===== Model Metadata (single task) ===== */
function renderModelMetaForTask(container, env, task) {
  var models = (env.benchmarked_models || []).filter(function(m) { return m.task === task; });
  if (!models.length) { container.innerHTML = '<p class="empty-state small">No data for this task.</p>'; return; }
  models.sort(function(a,b) { return sizeOrd(a.size) - sizeOrd(b.size); });
  var html = '<table class="summary-table bench-table"><colgroup><col style="width:34%"><col style="width:8%"><col style="width:14%"><col style="width:14%"><col style="width:15%"><col style="width:15%"></colgroup><thead><tr><th>Model</th><th>Size</th><th>Input</th><th>NPU Mem (MB)</th><th>DXNN Format</th><th>DX-COM</th></tr></thead><tbody>';
  models.forEach(function(m) {
    var input = formatInputShape(m.input_tensor_shape || m.input_size);
    var mem = formatMemMB(m.total_memory_mb);
    html += '<tr><td>' + escHtml(m.name||'-') + '</td><td>' + (m.size||'-').toUpperCase() + '</td><td>' + input + '</td><td>' + mem + '</td><td>' + escHtml(stripAnsi(m.format_version)||'-') + '</td><td>' + escHtml(stripAnsi(m.dxcom_version)||'-') + '</td></tr>';
  });
  html += '</tbody></table>';
  container.innerHTML = html;
}

/* ===== E2E Results Table ===== */
function renderE2eTable(container, envId, task, useOrt, runId) {
  var rows = _history('e2e_single').filter(function(r) {
    return r.env_id === envId && r.run_id===runId && r.task === task && r.use_ort === useOrt;
  });
  rows.sort(function(a,b) { return sizeOrd(modelSizeChar(a.model)) - sizeOrd(modelSizeChar(b.model)); });
  if (!rows.length) { container.innerHTML = '<p class="empty-state">No E2E data for this selection.</p>'; return; }

  var html = '';
  /* Decode path summary from first row with pipeline_caps */
  var caps = null;
  for (var ci=0;ci<rows.length;ci++){if(rows[ci].pipeline_caps){caps=rows[ci].pipeline_caps;break;}}
  if (caps) {
    var parts = [];
    if (caps.video_codec) parts.push('<b>Codec:</b> '+escHtml(caps.video_codec));
    /* Decoder element name (same for all rows in this env) */
    var decName = null;
    for (var di=0;di<rows.length;di++){if(rows[di].decoder && rows[di].decoder!=='unknown'){decName=rows[di].decoder;break;}}
    if (decName) parts.push('<b>Decoder:</b> '+escHtml(decName));
    var decFmt = caps.decoder_src_format||'?';
    var decMem = caps.decoder_src_memory;
    parts.push('<b>Decoder Out:</b> '+escHtml(decFmt)+(decMem?' <span class="tag tag--warn">'+escHtml(decMem)+'</span>':''));
    var ppFmt = caps.dxpreprocess_sink_format||'?';
    var ppMem = caps.dxpreprocess_sink_memory;
    parts.push('<b>Preprocess In:</b> '+escHtml(ppFmt)+(ppMem?' <span class="tag tag--warn">'+escHtml(ppMem)+'</span>':''));
    if (caps.dxpreprocess_backend) parts.push('<b>Preprocess Backend:</b> '+escHtml(caps.dxpreprocess_backend));
    html += '<p class="decode-path-summary">'+parts.join(' &nbsp;|&nbsp; ')+'</p>';
  }

  html += '<table class="summary-table"><thead><tr><th>Model</th><th>E2E FPS</th><th>CPU%</th><th>NPU Avg%</th><th>NPU Max%</th><th>NPU Temp \u00b0C</th><th>NPU MHz</th><th>Host RSS (MiB)</th><th>Runs</th><th>Status</th></tr></thead><tbody>';
  rows.forEach(function(r) {
    var fpsS=fmt(r.avg_e2e_fps,1);if(r.fps_std!=null)fpsS+=' \u00b1'+fmt(r.fps_std,1);
    var tempS=_fmtTemp(r.npu_temp_min_c,r.npu_temp_max_c);
    var clkS=_fmtClock(r.npu_clock_mhz_min,r.npu_clock_mhz_max);
    html += '<tr><td>'+escHtml(r.model)+'</td><td>'+fpsS+'</td><td>'+fmt(r.avg_cpu_pct,0)+'</td><td>'+fmt(r.npu_total_avg_pct,1)+'</td><td>'+fmt(r.npu_total_max_pct,1)+'</td><td>'+tempS+'</td><td>'+clkS+'</td><td>'+fmt(r.max_rss_mib,0)+'</td><td>'+(r.runs||'-')+'/'+(r.requested_runs||'-')+'</td><td>'+escHtml(r.status||'-')+'</td></tr>';
  });
  html += '</tbody></table>';
  container.innerHTML = html;
}

/* ===== Full Metrics Chart (pure Canvas) ===== */
var Chart = {
  _canvas: null, _onClick: null, _hoverIdx: -1, _data: [], _selectedId: null, _raf: null, _ro: null,
  init: function(canvas, onClick) {
    this._canvas = canvas; this._onClick = onClick;
    this._ro = new ResizeObserver(this._resize.bind(this));
    this._ro.observe(canvas.parentElement); this._resize();
    canvas.addEventListener('click', this._handleClick.bind(this));
    canvas.addEventListener('mousemove', this._handleHover.bind(this));
    canvas.addEventListener('mouseleave', this._handleLeave.bind(this));
  },
  update: function(data, selectedId) { this._data = data; this._selectedId = selectedId; this._scheduleDraw(); },
  _resize: function() {
    var el=this._canvas.parentElement, dpr=window.devicePixelRatio||1;
    var w=el.clientWidth, h=el.clientHeight;
    this._canvas.width=w*dpr; this._canvas.height=h*dpr;
    this._canvas.style.width=w+'px'; this._canvas.style.height=h+'px';
    this._canvas.getContext('2d').scale(dpr,dpr); this._scheduleDraw();
  },
  _scheduleDraw: function() { var s=this; if(s._raf) cancelAnimationFrame(s._raf); s._raf=requestAnimationFrame(function(){s._draw();}); },
  _layout: function() { var dpr=window.devicePixelRatio||1; var W=this._canvas.width/dpr,H=this._canvas.height/dpr; var P={top:65,right:90,bottom:90,left:72}; return {W:W,H:H,P:P,CW:W-P.left-P.right,CH:H-P.top-P.bottom}; },
  _niceMax: function(v) { if(v<=0)return 10; var e=Math.pow(10,Math.floor(Math.log10(v))); var f=v/e; return (f<=2?2:f<=5?5:10)*e; },
  _scales: function(data) {
    var maxFps=0,maxLat=0;
    data.forEach(function(d){if(d.throughput)maxFps=Math.max(maxFps,d.throughput);if(d.e2eFps)maxFps=Math.max(maxFps,d.e2eFps);if(d.latency)maxLat=Math.max(maxLat,d.latency);});
    return {fpsCeil:this._niceMax(maxFps*1.3),latCeil:this._niceMax(maxLat*1.35)};
  },
  _draw: function() {
    var data=this._data,cv=this._canvas,ctx=cv.getContext('2d');var lay=this._layout();var W=lay.W,H=lay.H,P=lay.P,CW=lay.CW,CH=lay.CH;ctx.clearRect(0,0,W,H);
    if(!data.length){ctx.fillStyle='#888';ctx.font='14px sans-serif';ctx.textAlign='center';ctx.fillText('No data for this selection',W/2,H/2);return;}
    var sc=this._scales(data);var fpsCeil=sc.fpsCeil,latCeil=sc.latCeil;
    var n=data.length,gW=CW/n,bW=gW*0.34;
    var fpsY=function(v){return P.top+CH-(v/fpsCeil)*CH;};var latY=function(v){return P.top+CH-(v/latCeil)*CH;};
    var gX=function(i){return P.left+i*gW+gW*0.1;};var midX=function(i){return P.left+i*gW+gW*0.5;};
    var C_TP={fill:'rgba(91,141,239,0.60)',hi:'rgba(91,141,239,0.90)',line:'rgb(91,141,239)',dim:'rgba(91,141,239,0.25)'};
    var C_E2E={fill:'rgba(46,204,113,0.60)',hi:'rgba(46,204,113,0.90)',line:'rgb(46,204,113)',dim:'rgba(46,204,113,0.25)'};
    var C_LAT='rgb(231,76,60)';var C_MAX='rgb(136,84,208)';var self=this;
    /* Grid */ctx.strokeStyle='rgba(0,0,0,0.065)';ctx.lineWidth=1;for(var g=0;g<=5;g++){var gy=P.top+CH*g/5;ctx.beginPath();ctx.moveTo(P.left,gy);ctx.lineTo(P.left+CW,gy);ctx.stroke();}
    /* Axes */ctx.strokeStyle='rgba(0,0,0,0.18)';ctx.lineWidth=1.5;[[P.left,P.top,P.left,P.top+CH],[P.left,P.top+CH,P.left+CW,P.top+CH],[P.left+CW,P.top,P.left+CW,P.top+CH]].forEach(function(l){ctx.beginPath();ctx.moveTo(l[0],l[1]);ctx.lineTo(l[2],l[3]);ctx.stroke();});
    /* Y left */ctx.textAlign='right';for(var t=0;t<=5;t++){var tv=fpsCeil*(5-t)/5,ty=P.top+CH*t/5;ctx.fillStyle='#444';ctx.font='11px sans-serif';ctx.fillText(Math.round(tv),P.left-6,ty+4);}
    /* Y right */ctx.textAlign='left';for(var t2=0;t2<=5;t2++){var tv2=latCeil*(5-t2)/5,ty2=P.top+CH*t2/5;ctx.fillStyle=C_LAT;ctx.font='11px sans-serif';ctx.fillText(Math.round(tv2),P.left+CW+8,ty2+4);}
    /* Axis titles */
    ctx.save();ctx.fillStyle='#333';ctx.font='bold 12px sans-serif';ctx.textAlign='center';ctx.translate(14,P.top+CH/2);ctx.rotate(-Math.PI/2);ctx.fillText('FPS',0,0);ctx.restore();
    ctx.save();ctx.fillStyle=C_LAT;ctx.font='bold 12px sans-serif';ctx.textAlign='center';ctx.translate(W-14,P.top+CH/2);ctx.rotate(Math.PI/2);ctx.fillText('Latency (ms)',0,0);ctx.restore();
    /* Bars */
    data.forEach(function(d,i){
      var sel=d.envId===self._selectedId;var hi=sel||i===self._hoverIdx;
      if(d.throughput!=null){var bx=gX(i),by=fpsY(d.throughput),bh=P.top+CH-by;ctx.fillStyle=sel?C_TP.hi:(self._selectedId&&!sel?C_TP.dim:(hi?C_TP.hi:C_TP.fill));ctx.fillRect(bx,by,bW-2,bh);ctx.strokeStyle=C_TP.line;ctx.lineWidth=sel?2:1;ctx.strokeRect(bx,by,bW-2,bh);ctx.fillStyle=C_TP.line;ctx.font='bold 10px sans-serif';ctx.textAlign='center';ctx.fillText(Math.round(d.throughput),bx+(bW-2)/2,by-4);}
      if(d.e2eFps!=null){var bx2=gX(i)+bW,by2=fpsY(d.e2eFps),bh2=P.top+CH-by2;ctx.fillStyle=sel?C_E2E.hi:(self._selectedId&&!sel?C_E2E.dim:(hi?C_E2E.hi:C_E2E.fill));ctx.fillRect(bx2+1,by2,bW-2,bh2);ctx.strokeStyle=C_E2E.line;ctx.lineWidth=sel?2:1;ctx.strokeRect(bx2+1,by2,bW-2,bh2);ctx.fillStyle='#1a1a1a';ctx.font='bold 10px sans-serif';ctx.textAlign='center';ctx.fillText(Math.round(d.e2eFps),bx2+1+(bW-2)/2,by2-4);}
      if(d.maxChannels!=null&&d.e2eFps!=null){var badgeX=gX(i)+bW+(bW-2)/2;var badgeY=fpsY(d.e2eFps)-20;var txt='Max '+d.maxChannels+'ch';ctx.font='bold 9px sans-serif';var tw=ctx.measureText(txt).width;var px=4,py=2,rr=4;var rx=badgeX-tw/2-px,ry=badgeY-8-py;var rw=tw+px*2,rh=12+py*2;ctx.fillStyle='rgba(136,84,208,0.15)';ctx.beginPath();ctx.moveTo(rx+rr,ry);ctx.lineTo(rx+rw-rr,ry);ctx.quadraticCurveTo(rx+rw,ry,rx+rw,ry+rr);ctx.lineTo(rx+rw,ry+rh-rr);ctx.quadraticCurveTo(rx+rw,ry+rh,rx+rw-rr,ry+rh);ctx.lineTo(rx+rr,ry+rh);ctx.quadraticCurveTo(rx,ry+rh,rx,ry+rh-rr);ctx.lineTo(rx,ry+rr);ctx.quadraticCurveTo(rx,ry,rx+rr,ry);ctx.closePath();ctx.fill();ctx.strokeStyle=C_MAX;ctx.lineWidth=1;ctx.stroke();ctx.fillStyle=C_MAX;ctx.textAlign='center';ctx.fillText(txt,badgeX,badgeY);}
      if(sel){ctx.save();ctx.strokeStyle='rgba(27,107,88,0.8)';ctx.lineWidth=3;ctx.setLineDash([6,3]);var sx=gX(i)-4,sw=bW*2+6;ctx.strokeRect(sx,P.top,sw,CH);ctx.setLineDash([]);ctx.restore();}
    });
    /* Latency line */
    var pts=data.map(function(d,i){return{x:midX(i),y:d.latency!=null?latY(d.latency):null,v:d.latency};});
    ctx.strokeStyle=C_LAT;ctx.lineWidth=2;ctx.setLineDash([8,5]);ctx.beginPath();var started=false;
    pts.forEach(function(p){if(p.y==null)return;if(!started){ctx.moveTo(p.x,p.y);started=true;}else{ctx.lineTo(p.x,p.y);}});ctx.stroke();ctx.setLineDash([]);
    pts.forEach(function(p){if(p.y==null)return;ctx.save();ctx.translate(p.x,p.y);ctx.rotate(Math.PI/4);ctx.fillStyle=C_LAT;ctx.fillRect(-5,-5,10,10);ctx.strokeStyle='#fff';ctx.lineWidth=2;ctx.strokeRect(-5,-5,10,10);ctx.restore();ctx.fillStyle=C_LAT;ctx.font='bold 10px sans-serif';ctx.textAlign='center';ctx.fillText(p.v.toFixed(1)+' ms',p.x,p.y-14);});
    /* X labels */ctx.fillStyle='#333';data.forEach(function(d,i){var parts=d.label.split('\n'),lx=midX(i);parts.forEach(function(part,pi){ctx.font=pi===0?'600 11px sans-serif':'11px sans-serif';ctx.textAlign='center';ctx.fillText(part,lx,P.top+CH+16+pi*14);});});
    /* Legend */
    var items=[{c:C_LAT,bc:C_LAT,label:'NPU Latency (Single-Core)',line:true},{c:C_TP.fill,bc:C_TP.line,label:'NPU Throughput (Multi-Core)',line:false},{c:C_E2E.fill,bc:C_E2E.line,label:'E2E FPS (Single-Stream)',line:false},{c:C_MAX,bc:C_MAX,label:'Max Channels (\u2265 30fps)',line:false,badge:true}];
    var lx=P.left,ly=22;items.forEach(function(it){if(it.line){ctx.strokeStyle=it.c;ctx.lineWidth=2;ctx.setLineDash([6,4]);ctx.beginPath();ctx.moveTo(lx,ly);ctx.lineTo(lx+20,ly);ctx.stroke();ctx.setLineDash([]);ctx.save();ctx.translate(lx+10,ly);ctx.rotate(Math.PI/4);ctx.fillStyle=it.c;ctx.fillRect(-4,-4,8,8);ctx.restore();}else if(it.badge){ctx.fillStyle='rgba(136,84,208,0.15)';ctx.fillRect(lx,ly-7,18,14);ctx.strokeStyle=it.bc;ctx.lineWidth=1;ctx.strokeRect(lx,ly-7,18,14);ctx.fillStyle=it.c;ctx.font='bold 8px sans-serif';ctx.textAlign='center';ctx.fillText('ch',lx+9,ly+3);}else{ctx.fillStyle=it.c;ctx.fillRect(lx,ly-7,18,14);ctx.strokeStyle=it.bc;ctx.lineWidth=1;ctx.strokeRect(lx,ly-7,18,14);}ctx.fillStyle='#333';ctx.font='12px sans-serif';ctx.textAlign='left';ctx.fillText(it.label,lx+24,ly+4);lx+=ctx.measureText(it.label).width+44;});
  },
  _hitTest: function(e){var rect=this._canvas.getBoundingClientRect();var x=e.clientX-rect.left;var lay=this._layout();if(x<lay.P.left||x>lay.P.left+lay.CW||!this._data.length)return -1;return Math.min(Math.floor((x-lay.P.left)/(lay.CW/this._data.length)),this._data.length-1);},
  _handleClick: function(e){var i=this._hitTest(e);if(i>=0&&this._onClick)this._onClick(i,this._data[i]);},
  _handleHover: function(e){var i=this._hitTest(e);if(i!==this._hoverIdx){this._hoverIdx=i;this._canvas.style.cursor=i>=0?'pointer':'default';this._scheduleDraw();}},
  _handleLeave: function(){this._hoverIdx=-1;this._canvas.style.cursor='default';this._scheduleDraw();},
};

/* ===== FPS Compare Chart ===== */
var FpsChart = {
  _canvas: null, _onClick: null, _hoverIdx: -1, _data: [], _selectedId: null, _raf: null, _ro: null,
  init: function(canvas, onClick) {
    this._canvas=canvas; this._onClick=onClick;
    this._ro=new ResizeObserver(this._resize.bind(this));
    this._ro.observe(canvas.parentElement); this._resize();
    canvas.addEventListener('click',this._handleClick.bind(this));
    canvas.addEventListener('mousemove',this._handleHover.bind(this));
    canvas.addEventListener('mouseleave',this._handleLeave.bind(this));
  },
  update: function(data,selectedId){this._data=data;this._selectedId=selectedId;this._scheduleDraw();},
  _resize: function(){var el=this._canvas.parentElement,dpr=window.devicePixelRatio||1;var w=el.clientWidth,h=el.clientHeight;this._canvas.width=w*dpr;this._canvas.height=h*dpr;this._canvas.style.width=w+'px';this._canvas.style.height=h+'px';this._canvas.getContext('2d').scale(dpr,dpr);this._scheduleDraw();},
  _scheduleDraw: function(){var s=this;if(s._raf)cancelAnimationFrame(s._raf);s._raf=requestAnimationFrame(function(){s._draw();});},
  _layout: function(){var dpr=window.devicePixelRatio||1;var W=this._canvas.width/dpr,H=this._canvas.height/dpr;var P={top:55,right:30,bottom:90,left:72};return {W:W,H:H,P:P,CW:W-P.left-P.right,CH:H-P.top-P.bottom};},
  _niceMax: function(v){if(v<=0)return 10;var e=Math.pow(10,Math.floor(Math.log10(v)));var f=v/e;return (f<=1.5?2:f<=3?4:f<=6?8:10)*e;},
  _draw: function() {
    var data=this._data,cv=this._canvas,ctx=cv.getContext('2d');var lay=this._layout();
    var W=lay.W,H=lay.H,P=lay.P,CW=lay.CW,CH=lay.CH;ctx.clearRect(0,0,W,H);
    if(!data.length){ctx.fillStyle='#888';ctx.font='14px sans-serif';ctx.textAlign='center';ctx.fillText('No data for this selection',W/2,H/2);return;}
    /* Adaptive Y-axis */
    var maxFps=0,minFps=Infinity;
    data.forEach(function(d){SIZE_KEYS.forEach(function(sz){if(d.sizes[sz]!=null){maxFps=Math.max(maxFps,d.sizes[sz]);minFps=Math.min(minFps,d.sizes[sz]);}});});
    if(minFps===Infinity)minFps=0;
    var range=maxFps-minFps;
    var floor=Math.max(0,Math.floor((minFps-range*0.3)/10)*10);
    if(range<maxFps*0.15)floor=Math.max(0,Math.floor(minFps*0.7/10)*10);
    var fpsCeil=this._niceMax((maxFps-floor)*1.2)+floor;
    var n=data.length;var gW=CW/n;var barAreaW=gW*0.82;var subW=barAreaW/SIZE_KEYS.length;var padLeft=(gW-barAreaW)/2;var self=this;
    var fpsY=function(v){return P.top+CH-((v-floor)/(fpsCeil-floor))*CH;};
    var gX=function(i,si){return P.left+i*gW+padLeft+si*subW;};
    var midX=function(i){return P.left+i*gW+gW/2;};
    /* Max channels lookup */
    var capMap={};
    _history('e2e_multi_capacity').forEach(function(r){
      if(r.use_ort!==state.fpsOrt)return;if((r.task||'')!==state.fpsTask)return;
      if(r.run_id!==_getSelectedRunId(r.env_id))return;
      var sz=r.size||modelSizeChar(r.model);
      var k=r.env_id+'|'+sz;
      if(!capMap[k]||r.capacity_streams>capMap[k])capMap[k]=r.capacity_streams;
    });
    /* Grid */ctx.strokeStyle='rgba(0,0,0,0.065)';ctx.lineWidth=1;for(var g=0;g<=5;g++){var gy=P.top+CH*g/5;ctx.beginPath();ctx.moveTo(P.left,gy);ctx.lineTo(P.left+CW,gy);ctx.stroke();}
    /* Axes */ctx.strokeStyle='rgba(0,0,0,0.18)';ctx.lineWidth=1.5;[[P.left,P.top,P.left,P.top+CH],[P.left,P.top+CH,P.left+CW,P.top+CH]].forEach(function(l){ctx.beginPath();ctx.moveTo(l[0],l[1]);ctx.lineTo(l[2],l[3]);ctx.stroke();});
    /* Y ticks */ctx.textAlign='right';ctx.fillStyle='#444';ctx.font='11px sans-serif';
    for(var t=0;t<=5;t++){var tv=fpsCeil-(fpsCeil-floor)*t/5,ty=P.top+CH*t/5;ctx.fillText(Math.round(tv),P.left-6,ty+4);}
    /* Y title */ctx.save();ctx.fillStyle='#333';ctx.font='bold 12px sans-serif';ctx.textAlign='center';ctx.translate(14,P.top+CH/2);ctx.rotate(-Math.PI/2);ctx.fillText('E2E FPS',0,0);ctx.restore();
    /* Bars */
    data.forEach(function(d,i){
      var sel=d.envId===self._selectedId;var hi=sel||i===self._hoverIdx;
      SIZE_KEYS.forEach(function(sz,si){
        var v=d.sizes[sz];if(v==null)return;
        var bx=gX(i,si),by=fpsY(v),bh=P.top+CH-by;var sc=SIZE_COLORS[sz];
        var barFill=sel?sc.hi:(self._selectedId&&!sel?sc.dim:(hi?sc.hi:sc.fill));
        ctx.fillStyle=barFill;ctx.fillRect(bx+1,by,subW-2,bh);
        ctx.strokeStyle=sc.line;ctx.lineWidth=sel?2:1;ctx.strokeRect(bx+1,by,subW-2,bh);
        ctx.fillStyle=sc.line;ctx.font='bold 9px sans-serif';ctx.textAlign='center';ctx.fillText(Math.round(v),bx+subW/2,by-4);
        /* Max Ch badge per size */
        var capKey=d.envId+'|'+sz;var cap=capMap[capKey];
        if(cap!=null){
          var bt=cap+'ch';ctx.font='bold 8px sans-serif';var btw=ctx.measureText(bt).width;
          var bcx=bx+subW/2,bcy=by-18;var bpx=3,bpy=1,brr=3;
          var brx=bcx-btw/2-bpx,bry=bcy-6-bpy;var brw=btw+bpx*2,brh=10+bpy*2;
          ctx.fillStyle='rgba(136,84,208,0.12)';ctx.beginPath();
          ctx.moveTo(brx+brr,bry);ctx.lineTo(brx+brw-brr,bry);ctx.quadraticCurveTo(brx+brw,bry,brx+brw,bry+brr);ctx.lineTo(brx+brw,bry+brh-brr);ctx.quadraticCurveTo(brx+brw,bry+brh,brx+brw-brr,bry+brh);ctx.lineTo(brx+brr,bry+brh);ctx.quadraticCurveTo(brx,bry+brh,brx,bry+brh-brr);ctx.lineTo(brx,bry+brr);ctx.quadraticCurveTo(brx,bry,brx+brr,bry);ctx.closePath();ctx.fill();
          ctx.strokeStyle='rgb(136,84,208)';ctx.lineWidth=0.8;ctx.stroke();
          ctx.fillStyle='rgb(136,84,208)';ctx.textAlign='center';ctx.fillText(bt,bcx,bcy);
        }
      });
      if(sel){ctx.save();ctx.strokeStyle='rgba(27,107,88,0.9)';ctx.lineWidth=3;ctx.setLineDash([6,3]);var sx=P.left+i*gW+2;ctx.strokeRect(sx,P.top-2,gW-4,CH+4);ctx.setLineDash([]);ctx.restore();}
    });
    /* X labels */ctx.fillStyle='#333';data.forEach(function(d,i){var parts=d.label.split('\n'),lx=midX(i);parts.forEach(function(part,pi){ctx.font=pi===0?'600 11px sans-serif':'11px sans-serif';ctx.textAlign='center';ctx.fillText(part,lx,P.top+CH+16+pi*14);});});
    /* Legend */
    var lx=P.left,ly=22;
    SIZE_KEYS.forEach(function(sz){var sc=SIZE_COLORS[sz];ctx.fillStyle=sc.fill;ctx.fillRect(lx,ly-7,18,14);ctx.strokeStyle=sc.line;ctx.lineWidth=1;ctx.strokeRect(lx,ly-7,18,14);var label=sz.toUpperCase()+' ('+SIZE_LABELS[sz]+')';ctx.fillStyle='#333';ctx.font='12px sans-serif';ctx.textAlign='left';ctx.fillText(label,lx+22,ly+4);lx+=ctx.measureText(label).width+40;});
    ctx.fillStyle='rgba(136,84,208,0.12)';ctx.fillRect(lx,ly-7,18,14);ctx.strokeStyle='rgb(136,84,208)';ctx.lineWidth=1;ctx.strokeRect(lx,ly-7,18,14);ctx.fillStyle='rgb(136,84,208)';ctx.font='bold 8px sans-serif';ctx.textAlign='center';ctx.fillText('ch',lx+9,ly+3);ctx.fillStyle='#333';ctx.font='12px sans-serif';ctx.textAlign='left';ctx.fillText('Max Ch (\u2265 30fps)',lx+22,ly+4);
  },
  _hitTest: function(e){var rect=this._canvas.getBoundingClientRect();var x=e.clientX-rect.left;var lay=this._layout();if(x<lay.P.left||x>lay.P.left+lay.CW||!this._data.length)return -1;return Math.min(Math.floor((x-lay.P.left)/(lay.CW/this._data.length)),this._data.length-1);},
  _handleClick: function(e){var i=this._hitTest(e);if(i>=0&&this._onClick)this._onClick(i,this._data[i]);},
  _handleHover: function(e){var i=this._hitTest(e);if(i!==this._hoverIdx){this._hoverIdx=i;this._canvas.style.cursor=i>=0?'pointer':'default';this._scheduleDraw();}},
  _handleLeave: function(){this._hoverIdx=-1;this._canvas.style.cursor='default';this._scheduleDraw();},
};

/* ===== Chart data helpers ===== */
function getChartData() {
  var sz=state.size;var task=state.task;var useOrt=state.ort;var results=[];
  var modelRows=_history('model');var e2eRows=_history('e2e_single');var capRows=_history('e2e_multi_capacity');
  (state.dataset.environments||[]).forEach(function(env){
    var eid=env.env_id;var runId=_getSelectedRunId(eid);
    var tRow=modelRows.find(function(r){return r.env_id===eid&&r.run_id===runId&&r.size===sz&&r.task===task&&r.use_ort===useOrt&&r.family==='throughput';});
    var lRow=modelRows.find(function(r){return r.env_id===eid&&r.run_id===runId&&r.size===sz&&r.task===task&&r.use_ort===useOrt&&r.family==='latency';});
    var eRow=e2eRows.find(function(r){return r.env_id===eid&&r.run_id===runId&&r.size===sz&&r.task===task&&r.use_ort===useOrt;});
    var cRow=capRows.find(function(r){return r.env_id===eid&&r.run_id===runId&&r.size===sz&&r.task===task&&r.use_ort===useOrt;});
    if(tRow||eRow){results.push({env:env,envId:eid,label:envLabel(env),throughput:tRow?tRow.fps:null,e2eFps:eRow?eRow.avg_e2e_fps:null,latency:lRow?lRow.latency_ms:null,maxChannels:cRow?cRow.capacity_streams:null});}
  });
  results.sort(function(a,b){return(a.e2eFps||0)-(b.e2eFps||0);});return results;
}
function refreshChart(preferredEnvId) {
  var data=getChartData();state.chartData=data;
  var model=getModelName(state.task,state.size);
  document.getElementById('chartSubtitle').textContent=TASK_MAP[state.task].label+'  \u00b7  '+model+'  \u00b7  ORT '+(state.ort?'ON':'OFF');
  if(!data.length){
    state.selectedEnvId=null;
    document.getElementById('envDetail').style.display='none';
    document.getElementById('overviewModelMetaPanel').style.display='none';
    Chart.update(data,null);return;
  }
  var selected=data[0];
  if(preferredEnvId){for(var i=0;i<data.length;i++){if(data[i].envId===preferredEnvId){selected=data[i];break;}}}
  state.selectedEnvId=selected.envId;renderEnvDetail(selected.env,{scroll:false});
  Chart.update(data,state.selectedEnvId);
}
function getFpsCompareData() {
  var useOrt=state.fpsOrt;var task=state.fpsTask;var results=[];var e2eRows=_history('e2e_single');
  (state.dataset.environments||[]).forEach(function(env){
    var eid=env.env_id;var runId=_getSelectedRunId(eid);var sizes={};var hasAny=false;
    SIZE_KEYS.forEach(function(sz){var eRow=e2eRows.find(function(r){return r.env_id===eid&&r.run_id===runId&&r.size===sz&&r.task===task&&r.use_ort===useOrt;});if(eRow){sizes[sz]=eRow.avg_e2e_fps;hasAny=true;}else{sizes[sz]=null;}});
    if(hasAny){results.push({env:env,envId:eid,label:envLabel(env),sizes:sizes});}
  });
  results.sort(function(a,b){var sum=function(d){var s=0;SIZE_KEYS.forEach(function(sz){if(d.sizes[sz]!=null)s+=d.sizes[sz];});return s;};return sum(a)-sum(b);});return results;
}
function refreshFpsCompare(preferredEnvId) {
  var data=getFpsCompareData();state.fpsChartData=data;
  document.getElementById('fpsChartSubtitle').textContent=TASK_MAP[state.fpsTask].label+'  \u00b7  ORT '+(state.fpsOrt?'ON':'OFF')+'  \u00b7  All Sizes (N / S / M / L / X)';
  if(!data.length){
    state.fpsSelectedEnvId=null;
    document.getElementById('fpsEnvDetail').style.display='none';
    document.getElementById('fpsModelMetaPanel').style.display='none';
    document.getElementById('fpsE2eTableSection').style.display='none';
    FpsChart.update(data,null);
    return;
  }
  var selected=data[0];var idx=0;
  if(preferredEnvId){for(var i=0;i<data.length;i++){if(data[i].envId===preferredEnvId){selected=data[i];idx=i;break;}}}
  handleFpsEnvClick(idx,selected,{scroll:false});
}

function handleFpsEnvClick(idx,d,options) {
  options=options||{};
  state.fpsSelectedEnvId=d.envId;
  var panel=document.getElementById('fpsEnvDetail');panel.style.display='';
  document.getElementById('fpsEnvDetailTitle').textContent=d.env.hostname+' ('+(d.env.npu_sku||'?')+')';
  renderHostInfo(document.getElementById('fpsEnvHostInfo'),d.env);
  renderNpuInfo(document.getElementById('fpsEnvNpuInfo'),d.env);
  renderToolsInfo(document.getElementById('fpsEnvToolsInfo'),d.env);
  var metaPanel=document.getElementById('fpsModelMetaPanel');metaPanel.style.display='';
  document.getElementById('fpsModelMetaTitle').textContent='Benchmarked Models – '+TASK_MAP[state.fpsTask].label;
  renderModelMetaForTask(document.getElementById('fpsModelMetaSection'),d.env,state.fpsTask);
  var e2eSection=document.getElementById('fpsE2eTableSection');e2eSection.style.display='';
  var runId=_getSelectedRunId(d.envId);
  document.getElementById('fpsE2eTableTitle').textContent='E2E Pipeline \u2013 '+TASK_MAP[state.fpsTask].label+' \u00b7 ORT '+(state.fpsOrt?'ON':'OFF')+' \u00b7 '+runId;
  renderE2eTable(document.getElementById('fpsE2eTableContent'),d.envId,state.fpsTask,state.fpsOrt,runId);
  /* View trend link */
  var hwId=_envToHwId(d.env);
  if(hwId&&_hwIdHasSnapshots(hwId)){
    var link=document.createElement('p');link.className='trend-link';link.innerHTML='<a href="#" id="fpsTrendLink">\u2192 View version trend for this environment</a>';
    document.getElementById('fpsE2eTableContent').appendChild(link);
    document.getElementById('fpsTrendLink').addEventListener('click',function(e){e.preventDefault();_switchToTrend(hwId);});
  }
  if(options.scroll!==false){panel.scrollIntoView({behavior:'smooth',block:'nearest'});}
  FpsChart.update(state.fpsChartData,state.fpsSelectedEnvId);
}

function initFpsFilters() {
  document.getElementById('fpsTaskFilter').value=state.fpsTask;
  document.getElementById('fpsOrtFilter').value=state.fpsOrt?'on':'off';
  document.getElementById('fpsTaskFilter').addEventListener('change',function(){state.fpsTask=this.value;refreshFpsCompare();});
  document.getElementById('fpsOrtFilter').addEventListener('change',function(){state.fpsOrt=this.value==='on';refreshFpsCompare();});
}

/* ===== Environment Detail (Full Metrics) ===== */
function renderEnvDetail(env,options) {
  options=options||{};
  var panel=document.getElementById('envDetail');panel.style.display='';
  document.getElementById('envDetailTitle').textContent=env.hostname+' ('+(env.npu_sku||'?')+')';
  renderHostInfo(document.getElementById('envHostInfo'),env);
  renderNpuInfo(document.getElementById('envNpuInfo'),env);
  renderToolsInfo(document.getElementById('envToolsInfo'),env);
  var metaPanel=document.getElementById('overviewModelMetaPanel');metaPanel.style.display='';
  document.getElementById('overviewModelMetaTitle').textContent='Benchmarked Models – '+TASK_MAP[state.task].label;
  renderModelMetaForTask(document.getElementById('overviewModelMetaSection'),env,state.task);
  /* View trend link */
  var hwId=_envToHwId(env);
  if(hwId&&_hwIdHasSnapshots(hwId)){
    var link=document.createElement('p');link.className='trend-link';link.innerHTML='<a href="#" id="overviewTrendLink">\u2192 View version trend for this environment</a>';
    document.getElementById('overviewModelMetaSection').appendChild(link);
    document.getElementById('overviewTrendLink').addEventListener('click',function(e){e.preventDefault();_switchToTrend(hwId);});
  }
  if(options.scroll!==false)panel.scrollIntoView({behavior:'smooth',block:'nearest'});
}

/* ===== Tabs ===== */
function initTabs() {
  document.querySelectorAll('.tab').forEach(function(btn){
    btn.addEventListener('click',function(){
      var target=this.dataset.tab;
      document.querySelectorAll('.tab').forEach(function(b){b.classList.remove('active');});
      document.querySelectorAll('.tab-content').forEach(function(c){c.classList.remove('active');});
      this.classList.add('active');document.getElementById('tab-'+target).classList.add('active');
      if(target==='fps-compare')FpsChart._resize();
      if(target==='overview')Chart._resize();
      if(target==='detail')renderDetailTables();
      if(target==='version-trend')resizeTrendChart();
    });
  });
}

/* ===== Overview Filters ===== */
function initOverviewFilters() {
  document.getElementById('taskFilter').value=state.task;
  document.getElementById('sizeFilter').value=state.size;
  document.getElementById('ortFilter').value=state.ort?'on':'off';
  document.getElementById('taskFilter').addEventListener('change',function(){state.task=this.value;refreshChart();});
  document.getElementById('sizeFilter').addEventListener('change',function(){state.size=this.value;refreshChart();});
  document.getElementById('ortFilter').addEventListener('change',function(){state.ort=this.value==='on';refreshChart();});
}

/* ===== Detail Tab ===== */
function initDetailTab() {
  var sel=document.getElementById('detailEnvFilter');
  var runSel=document.getElementById('detailRunFilter');
  sel.innerHTML=(state.dataset.environments||[]).map(function(e){return '<option value="'+e.env_id+'">'+escHtml(e.hostname)+' ('+escHtml(e.npu_sku)+')</option>';}).join('');
  if(state.dataset.environments.length){state.detailEnvId=state.dataset.environments[0].env_id;sel.value=state.detailEnvId;}
  syncDetailRunFilter();
  sel.addEventListener('change',function(){state.detailEnvId=this.value;syncDetailRunFilter();renderDetailTables();});
  runSel.addEventListener('change',function(){if(state.detailEnvId)_handleRunSelectionChange(state.detailEnvId,this.value);});
  document.getElementById('detailTaskFilter').addEventListener('change',function(){state.detailTask=this.value;renderDetailTables();});
  document.getElementById('detailOrtFilter').addEventListener('change',function(){state.detailOrt=this.value;renderDetailTables();});
}

function renderDetailTables() {
  var target=document.getElementById('detailTables');var envId=state.detailEnvId;
  if(!envId){target.innerHTML='<div class="empty-state">No environment selected.</div>';return;}
  var runId=_getSelectedRunId(envId);
  var latMap={},tpMap={},e2eMap={},capMap={},taskModels={};
  _history('model').forEach(function(r){if(r.env_id!==envId||r.run_id!==runId)return;var k=r.model+'|'+(r.use_ort?'on':'off');if(r.family==='latency')latMap[k]=r.latency_ms;if(r.family==='throughput')tpMap[k]=r.fps;if(!taskModels[r.task])taskModels[r.task]={};taskModels[r.task][r.model]=true;});
  _history('e2e_single').forEach(function(r){if(r.env_id!==envId||r.run_id!==runId)return;e2eMap[r.model+'|'+(r.use_ort?'on':'off')]=r;if(!taskModels[r.task])taskModels[r.task]={};taskModels[r.task][r.model]=true;});
  _history('e2e_multi_capacity').forEach(function(r){if(r.env_id!==envId||r.run_id!==runId)return;capMap[r.model+'|'+(r.use_ort?'on':'off')]=r.capacity_streams;if(!taskModels[r.task])taskModels[r.task]={};taskModels[r.task][r.model]=true;});
  var tasks=Object.keys(taskModels).sort(function(a,b){var oa=TASK_ORDER[a],ob=TASK_ORDER[b];return(oa!==undefined?oa:99)-(ob!==undefined?ob:99);});
  if(state.detailTask!=='all')tasks=tasks.filter(function(t){return t===state.detailTask;});
  if(!tasks.length){target.innerHTML='<div class="empty-state">No data.</div>';return;}
  target.innerHTML=tasks.map(function(task){
    var models=Object.keys(taskModels[task]).sort(function(a,b){var d=sizeOrd(modelSizeChar(a))-sizeOrd(modelSizeChar(b));return d!==0?d:a.localeCompare(b);});
    var body=models.map(function(model){var ortRows=[true,false].filter(function(useOrt){return state.detailOrt==='all'||state.detailOrt===(useOrt?'on':'off');});var fpsOn=null,fpsOff=null,capOn=null,capOff=null;ortRows.forEach(function(useOrt){var k=model+'|'+(useOrt?'on':'off');var e2e=e2eMap[k]||{};if(useOrt){fpsOn=e2e.avg_e2e_fps!=null?Number(e2e.avg_e2e_fps):null;capOn=capMap[k]!=null?Number(capMap[k]):null;}else{fpsOff=e2e.avg_e2e_fps!=null?Number(e2e.avg_e2e_fps):null;capOff=capMap[k]!=null?Number(capMap[k]):null;}});var bestFpsOrt=null,bestCapOrt=null;if(fpsOn!=null&&fpsOff!=null){bestFpsOrt=fpsOn>=fpsOff?'on':'off';}else if(fpsOn!=null){bestFpsOrt='on';}else if(fpsOff!=null){bestFpsOrt='off';}if(capOn!=null&&capOff!=null){bestCapOrt=capOn>=capOff?'on':'off';}else if(capOn!=null){bestCapOrt='on';}else if(capOff!=null){bestCapOrt='off';}var sz=modelSizeChar(model);var szLabel=sz?sz.toUpperCase():'-';return ortRows.map(function(useOrt){var k=model+'|'+(useOrt?'on':'off');var e2e=e2eMap[k]||{};var ortKey=useOrt?'on':'off';var fpsVal=fmt(e2e.avg_e2e_fps,1);var capVal=capMap[k]!=null?capMap[k]:null;var fpsTd=bestFpsOrt===ortKey?'<td class="cell-best">'+fpsVal+'</td>':'<td>'+fpsVal+'</td>';var capTd=bestCapOrt===ortKey?'<td class="cell-best">'+(capVal!=null?capVal:'-')+'</td>':'<td>'+(capVal!=null?capVal:'-')+'</td>';return '<tr><td>'+escHtml(model)+'</td><td>'+szLabel+'</td><td>'+(useOrt?'ON':'OFF')+'</td><td>'+fmt(latMap[k],2)+'</td><td>'+fmt(tpMap[k],1)+'</td>'+fpsTd+capTd+'</tr>';}).join('');}).join('');
    return '<section class="task-section"><h3>'+(TASK_MAP[task]?TASK_MAP[task].label:task)+'</h3><table class="summary-table detail-table"><colgroup><col style="width:28%"><col style="width:7%"><col style="width:7%"><col style="width:15%"><col style="width:16%"><col style="width:13%"><col style="width:14%"></colgroup><thead><tr><th>Model</th><th>Size</th><th>ORT</th><th>NPU Latency (ms)</th><th>NPU Throughput (FPS)</th><th>E2E FPS</th><th>Max Channels</th></tr></thead><tbody>'+body+'</tbody></table></section>';
  }).join('');
}

/* ===== Meta ===== */
function renderMeta() {
  var m=state.dataset.meta||{};
  document.getElementById('meta').innerHTML=
    '<div><strong>Environments</strong><span>'+(m.environment_count||0)+'</span></div>'+
    '<div><strong>Generated</strong><span>'+(m.generated_at?new Date(m.generated_at).toLocaleDateString():'-')+'</span></div>';
}

/* ===== Version Trend helpers ===== */
function _envToHwId(env){
  if(!env)return null;
  if(env.hw_id)return env.hw_id;
  var h=env.hostname||'unknown',s=env.npu_sku||'unknown';
  return (h+'_'+s).replace(/[^A-Za-z0-9_.\-]/g,'_').replace(/_+/g,'_').replace(/^_|_$/g,'');
}
function _hwIdHasSnapshots(hwId){
  var snaps=state.dataset.snapshots||[];
  for(var i=0;i<snaps.length;i++){if(snaps[i].hw_id===hwId)return true;}
  return false;
}
function _getUniqueHwIds(){
  var snaps=state.dataset.snapshots||[];
  var seen={},ids=[];
  snaps.forEach(function(s){if(!seen[s.hw_id]){seen[s.hw_id]=true;ids.push(s.hw_id);}});
  return ids;
}
function _switchToTrend(hwId){
  document.querySelectorAll('.tab').forEach(function(b){b.classList.remove('active');});
  document.querySelectorAll('.tab-content').forEach(function(c){c.classList.remove('active');});
  document.querySelector('[data-tab="version-trend"]').classList.add('active');
  document.getElementById('tab-version-trend').classList.add('active');
  var sel=document.getElementById('trendEnvFilter');
  sel.value=hwId;state.trendHwId=hwId;
  refreshTrend();resizeTrendChart();
}

function _trendMetricByKey(metricKey){return TREND_METRICS.find(function(metric){return metric.key===metricKey;})||TREND_METRICS[0];}
function _formatTrendValue(metric,value){
  if(value==null)return '-';
  if(metric.precision===0)return String(Math.round(Number(value)));
  return Number(value).toFixed(metric.precision);
}
function _latestTrendPointIndex(data){
  return (data.length&&data[0].points&&data[0].points.length)?(data[0].points.length-1):-1;
}
function _snapshotMetricValue(snap,task,useOrt,metric,sizeKey){
  var rows=(snap.results&&snap.results[metric.resultKind])||[];
  var row=rows.find(function(candidate){
    if(candidate.size!==sizeKey||candidate.use_ort!==useOrt)return false;
    if(candidate.task!==task)return false;
    if(metric.family)return candidate.family===metric.family;
    return true;
  });
  if(!row)return null;
  return row[metric.valueKey];
}
function createTrendChart(canvas,onClick){
  var chart={_canvas:canvas,_metric:TREND_METRICS[0],_onClick:onClick,_hoverIdx:-1,_data:[],_selectedIdx:-1,_raf:null,_ro:null};
  chart.setMetric=function(metric){this._metric=metric||TREND_METRICS[0];this._scheduleDraw();};
  chart.update=function(data,selectedIdx){this._data=data;this._selectedIdx=selectedIdx;this._scheduleDraw();};
  chart._resize=function(){var el=this._canvas.parentElement,dpr=window.devicePixelRatio||1;var w=el.clientWidth,h=el.clientHeight;this._canvas.width=w*dpr;this._canvas.height=h*dpr;this._canvas.style.width=w+'px';this._canvas.style.height=h+'px';this._canvas.getContext('2d').scale(dpr,dpr);this._scheduleDraw();};
  chart._scheduleDraw=function(){var s=this;if(s._raf)cancelAnimationFrame(s._raf);s._raf=requestAnimationFrame(function(){s._draw();});};
  chart._layout=function(){var dpr=window.devicePixelRatio||1;var W=this._canvas.width/dpr,H=this._canvas.height/dpr;var P={top:55,right:30,bottom:80,left:72};return{W:W,H:H,P:P,CW:W-P.left-P.right,CH:H-P.top-P.bottom};};
  chart._niceMax=function(v){if(v<=0)return 10;var e=Math.pow(10,Math.floor(Math.log10(v)));var f=v/e;return(f<=2?2:f<=5?5:10)*e;};
  chart._draw=function(){
    var data=this._data,cv=this._canvas,ctx=cv.getContext('2d');
    var lay=this._layout();var W=lay.W,H=lay.H,P=lay.P,CW=lay.CW,CH=lay.CH;
    ctx.clearRect(0,0,W,H);
    if(!data.length||!data[0].points||!data[0].points.length){ctx.fillStyle='#888';ctx.font='14px sans-serif';ctx.textAlign='center';ctx.fillText((state.dataset.snapshots||[]).length?('No trend data for '+this._metric.metricLabel.toLowerCase()+' with this selection'):'No snapshot history available. Build the dashboard from nested results/{hw_id}/{run_id} data to see version trend.',W/2,H/2);return;}
    var nPts=data[0].points.length;var maxVal=0,minVal=Infinity;
    data.forEach(function(line){line.points.forEach(function(point){if(point.value!=null){maxVal=Math.max(maxVal,point.value);minVal=Math.min(minVal,point.value);}});});
    if(minVal===Infinity)minVal=0;
    var range=maxVal-minVal;var pad=Math.max(range*0.2,maxVal*0.02,1);var rawFloor=minVal-pad;var rawCeil=maxVal+pad;var ystep=(rawCeil-rawFloor)/5;var ymag=Math.pow(10,Math.floor(Math.log10(Math.max(ystep,1e-9))));var ynorm=ystep/ymag;var niceStep=(ynorm<=1?1:ynorm<=2?2:ynorm<=5?5:10)*ymag;var floor=Math.max(0,Math.floor(rawFloor/niceStep)*niceStep);var ceil=Math.ceil(rawCeil/niceStep)*niceStep;if(ceil<=floor)ceil=floor+niceStep*5;
    var xMargin=nPts<=1?0:Math.max(40,Math.min(CW*0.08,80));var xSpan=CW-2*xMargin;var self=this;var xPos=function(i){return P.left+xMargin+(nPts===1?xSpan/2:i*(xSpan/Math.max(nPts-1,1)));};var yPos=function(v){return P.top+CH-((v-floor)/(ceil-floor||1))*CH;};var hasSelection=self._selectedIdx>=0&&self._selectedIdx<nPts;var groupWidth=nPts===1?Math.min(xSpan*0.6,96):Math.max(36,Math.min(90,(xSpan/Math.max(nPts-1,1))-14));
    ctx.strokeStyle='rgba(0,0,0,0.065)';ctx.lineWidth=1;for(var g=0;g<=5;g++){var gy=P.top+CH*g/5;ctx.beginPath();ctx.moveTo(P.left,gy);ctx.lineTo(P.left+CW,gy);ctx.stroke();}
    ctx.strokeStyle='rgba(0,0,0,0.18)';ctx.lineWidth=1.5;ctx.beginPath();ctx.moveTo(P.left,P.top);ctx.lineTo(P.left,P.top+CH);ctx.stroke();ctx.beginPath();ctx.moveTo(P.left,P.top+CH);ctx.lineTo(P.left+CW,P.top+CH);ctx.stroke();
    ctx.textAlign='right';ctx.fillStyle='#444';ctx.font='11px sans-serif';for(var t=0;t<=5;t++){var tv=ceil-(ceil-floor)*t/5;ctx.fillText(Math.round(tv),P.left-6,P.top+CH*t/5+4);}ctx.save();ctx.fillStyle='#333';ctx.font='bold 12px sans-serif';ctx.textAlign='center';ctx.translate(14,P.top+CH/2);ctx.rotate(-Math.PI/2);ctx.fillText(this._metric.axisLabel,0,0);ctx.restore();
    if(hasSelection){var selectedX=xPos(self._selectedIdx);var groupLeft=Math.max(P.left,Math.min(selectedX-groupWidth/2,P.left+CW-groupWidth));ctx.save();ctx.fillStyle='rgba(27,107,88,0.06)';ctx.fillRect(groupLeft,P.top-2,groupWidth,CH+4);ctx.strokeStyle='rgba(27,107,88,0.9)';ctx.lineWidth=3;ctx.setLineDash([6,3]);ctx.strokeRect(groupLeft,P.top-2,groupWidth,CH+4);ctx.setLineDash([]);ctx.restore();}
    var _labelGap=13;function _spreadLabels(items){items.sort(function(a,b){return a.baseY-b.baseY;});for(var pass=0;pass<4;pass++){for(var j=1;j<items.length;j++){var gap=items[j].y-items[j-1].y;if(gap<_labelGap){var shift=(_labelGap-gap)/2;items[j-1].y-=Math.ceil(shift);items[j].y+=Math.ceil(shift);}}items.sort(function(a,b){return a.y-b.y;});}return items;}
    var colLabels={};data.forEach(function(line,li){line.points.forEach(function(point,i){if(point.value==null)return;if(!colLabels[i])colLabels[i]=[];colLabels[i].push({lineIdx:li,baseY:yPos(point.value)-10,y:yPos(point.value)-10,value:point.value});});});Object.keys(colLabels).forEach(function(k){_spreadLabels(colLabels[k]);});
    data.forEach(function(line,li){var sc=SIZE_COLORS[line.size]||SIZE_COLORS.n;ctx.save();ctx.globalAlpha=hasSelection?0.72:1;ctx.strokeStyle=sc.line;ctx.lineWidth=2.5;ctx.beginPath();var started=false;line.points.forEach(function(point,i){if(point.value==null)return;var x=xPos(i),y=yPos(point.value);if(!started){ctx.moveTo(x,y);started=true;}else{ctx.lineTo(x,y);}});ctx.stroke();ctx.restore();line.points.forEach(function(point,i){if(point.value==null)return;var x=xPos(i),y=yPos(point.value);var isSelectedColumn=i===self._selectedIdx;var isHoveredColumn=i===self._hoverIdx;var r=isSelectedColumn?7:(isHoveredColumn?6:5);ctx.fillStyle=hasSelection?(isSelectedColumn?sc.hi:(isHoveredColumn?sc.fill:sc.dim)):(isHoveredColumn?sc.hi:sc.fill);ctx.beginPath();ctx.arc(x,y,r,0,Math.PI*2);ctx.fill();ctx.strokeStyle=sc.line;ctx.lineWidth=isSelectedColumn?3:1.5;ctx.stroke();var labelY=y-10;var cl=colLabels[i];if(cl){for(var ci=0;ci<cl.length;ci++){if(cl[ci].lineIdx===li){labelY=cl[ci].y;break;}}}var isEmphasized=isSelectedColumn||isHoveredColumn;var labelText=_formatTrendValue(self._metric,point.value);ctx.font=isEmphasized?'bold 11px sans-serif':'10px sans-serif';ctx.textAlign='center';ctx.globalAlpha=hasSelection?(isEmphasized?1:0.7):1;if(isEmphasized){ctx.strokeStyle='#fff';ctx.lineWidth=3;ctx.lineJoin='round';ctx.strokeText(labelText,x,labelY);ctx.fillStyle='#111';ctx.fillText(labelText,x,labelY);}else{ctx.strokeStyle='#fff';ctx.lineWidth=3;ctx.lineJoin='round';ctx.strokeText(labelText,x,labelY);ctx.fillStyle='#111';ctx.fillText(labelText,x,labelY);}ctx.globalAlpha=1;});});
    if(data[0]&&data[0].points){data[0].points.forEach(function(point,i){var x=xPos(i);var isSelectedColumn=i===self._selectedIdx;var isHoveredColumn=i===self._hoverIdx;ctx.fillStyle=hasSelection?(isSelectedColumn?'#333':(isHoveredColumn?'#5d6572':'rgba(93,101,114,0.55)')):'#333';ctx.font=isSelectedColumn?'600 11px sans-serif':'11px sans-serif';ctx.textAlign='center';ctx.fillText(point.dateLabel||'',x,P.top+CH+16);if(point.swLabel){ctx.fillStyle=hasSelection?(isSelectedColumn?'#888':'rgba(93,101,114,0.45)'):'#888';ctx.font='10px sans-serif';ctx.fillText(point.swLabel,x,P.top+CH+30);}});}
    var lx=P.left,ly=22;SIZE_KEYS.forEach(function(sz){var sc=SIZE_COLORS[sz];ctx.fillStyle=sc.fill;ctx.beginPath();ctx.arc(lx+6,ly,5,0,Math.PI*2);ctx.fill();ctx.strokeStyle=sc.line;ctx.lineWidth=1.5;ctx.stroke();var label=sz.toUpperCase()+' ('+SIZE_LABELS[sz]+')';ctx.fillStyle='#333';ctx.font='12px sans-serif';ctx.textAlign='left';ctx.fillText(label,lx+16,ly+4);lx+=ctx.measureText(label).width+35;});
  };
  chart._hitTest=function(e){var rect=this._canvas.getBoundingClientRect();var x=e.clientX-rect.left;var lay=this._layout();var data=this._data;if(!data.length||!data[0].points||!data[0].points.length)return -1;var nPts=data[0].points.length;var xM=nPts<=1?0:Math.max(40,Math.min(lay.CW*0.08,80));var xS=lay.CW-2*xM;if(nPts===1)return Math.abs(x-(lay.P.left+xM+xS/2))<30?0:-1;var step=xS/Math.max(nPts-1,1);var idx=Math.round((x-lay.P.left-xM)/step);if(idx<0||idx>=nPts)return -1;var hitX=lay.P.left+xM+idx*step;return Math.abs(x-hitX)<Math.max(20,step*0.4)?idx:-1;};
  chart._handleClick=function(e){var i=this._hitTest(e);if(i>=0&&this._onClick)this._onClick(i);};
  chart._handleHover=function(e){var i=this._hitTest(e);if(i!==this._hoverIdx){this._hoverIdx=i;this._canvas.style.cursor=i>=0?'pointer':'default';this._scheduleDraw();}};
  chart._handleLeave=function(){this._hoverIdx=-1;this._canvas.style.cursor='default';this._scheduleDraw();};
  chart._ro=new ResizeObserver(chart._resize.bind(chart));chart._ro.observe(canvas.parentElement);canvas.addEventListener('click',chart._handleClick.bind(chart));canvas.addEventListener('mousemove',chart._handleHover.bind(chart));canvas.addEventListener('mouseleave',chart._handleLeave.bind(chart));chart._resize();
  return chart;
}
function resizeTrendChart(){if(state.trendChart)state.trendChart._resize();}
function getTrendData(hwId,task,useOrt,metricKey){
  var metric=_trendMetricByKey(metricKey);var snaps=(state.dataset.snapshots||[]).filter(function(s){return s.hw_id===hwId;});snaps.sort(function(a,b){return(a.run_id||'').localeCompare(b.run_id||'');});if(!snaps.length)return[];var lines=SIZE_KEYS.map(function(sz){return{size:sz,points:[]};});
  snaps.forEach(function(snap){var ts=snap.timestamp?snap.timestamp.substring(0,10):(snap.run_id||'').replace(/(\d{4})(\d{2})(\d{2}).*/,'$1-$2-$3');var sw=snap.sw_versions||{};var swLabel='dxs '+(sw.dx_stream||'?');SIZE_KEYS.forEach(function(sz,si){var value=_snapshotMetricValue(snap,task,useOrt,metric,sz);lines[si].points.push({value:value!=null?Number(value):null,dateLabel:ts,swLabel:swLabel,run_id:snap.run_id,snap:snap});});});
  return lines;
}
function hideTrendEnvDetail(){var panel=document.getElementById('trendEnvDetail');if(panel)panel.style.display='none';var metaPanel=document.getElementById('trendModelMetaPanel');if(metaPanel)metaPanel.style.display='none';}
function renderTrendEnvDetail(snap,options){
  options=options||{};var panel=document.getElementById('trendEnvDetail');if(!panel)return;var env=snap&&snap.environment;if(!env){panel.style.display='none';document.getElementById('trendModelMetaPanel').style.display='none';return;}panel.style.display='';var metric=_trendMetricByKey(state.trendMetric);var dateStr=snap.timestamp?snap.timestamp.substring(0,10):snap.run_id;document.getElementById('trendEnvDetailTitle').textContent=(metric.metricLabel||'Metric')+' \u00b7 '+(env.hostname||'Environment')+' ('+(env.npu_sku||'?')+') \u00b7 '+dateStr+' \u00b7 '+snap.run_id;renderHostInfo(document.getElementById('trendEnvHostInfo'),env);renderNpuInfo(document.getElementById('trendEnvNpuInfo'),env);renderToolsInfo(document.getElementById('trendEnvToolsInfo'),env);
  var metaPanel=document.getElementById('trendModelMetaPanel');metaPanel.style.display='';document.getElementById('trendModelMetaTitle').textContent='Benchmarked Models \u2013 '+TASK_MAP[state.trendTask].label+' \u00b7 '+snap.run_id;renderModelMetaForTask(document.getElementById('trendModelMetaSection'),env,state.trendTask);
  if(options.scroll!==false)panel.scrollIntoView({behavior:'smooth',block:'nearest'});
}
function refreshTrend(){
  var hwId=state.trendHwId;var task=state.trendTask;var useOrt=state.trendOrt;var metric=_trendMetricByKey(state.trendMetric);var data=getTrendData(hwId,task,useOrt,metric.key);var selectedIdx=_latestTrendPointIndex(data);state.trendData=data;state.trendSelectedIdx=selectedIdx;document.getElementById('trendChartTitle').textContent=metric.title;document.getElementById('trendChartSubtitle').textContent='Metric: '+metric.metricLabel+'  \u00b7  '+(TASK_MAP[task]?TASK_MAP[task].label:task)+'  \u00b7  ORT '+(useOrt?'ON':'OFF')+'  \u00b7  All Sizes (N / S / M / L / X)';if(state.trendChart)state.trendChart.setMetric(metric);if(selectedIdx>=0){handleTrendPointClick(selectedIdx,{scroll:false});}else{hideTrendEnvDetail();if(state.trendChart)state.trendChart.update(data,-1);} 
}
function handleTrendPointClick(idx,options){
  options=options||{};state.trendSelectedIdx=idx;var data=state.trendData||[];if(state.trendChart)state.trendChart.update(data,idx);var snap=null;if(data.length&&data[0].points[idx])snap=data[0].points[idx].snap;if(!snap){hideTrendEnvDetail();return;}renderTrendEnvDetail(snap,options);
}
function initTrendChart(){
  var canvas=document.getElementById('trendChart');if(canvas)state.trendChart=createTrendChart(canvas,function(idx){handleTrendPointClick(idx);});
}
function initTrendTab(){
  var snaps=state.dataset.snapshots||[];var sel=document.getElementById('trendEnvFilter');var hwIds=_getUniqueHwIds();sel.innerHTML=hwIds.map(function(id){return '<option value="'+escHtml(id)+'">'+escHtml(id)+'</option>';}).join('');if(hwIds.length){state.trendHwId=hwIds[0];sel.value=hwIds[0];}state.trendTask='object_detection';state.trendOrt=true;state.trendMetric='e2e';document.getElementById('trendMetricFilter').value=state.trendMetric;sel.addEventListener('change',function(){state.trendHwId=this.value;refreshTrend();});document.getElementById('trendTaskFilter').addEventListener('change',function(){state.trendTask=this.value;refreshTrend();});document.getElementById('trendOrtFilter').addEventListener('change',function(){state.trendOrt=this.value==='on';refreshTrend();});document.getElementById('trendMetricFilter').addEventListener('change',function(){state.trendMetric=this.value;refreshTrend();});refreshTrend();
}

/* ===== Main ===== */
async function main() {
  try{state.dataset=await loadDataset();}catch(e){document.querySelector('.chart-container').innerHTML='<div class="empty-state" style="margin:80px auto">Failed to load dataset: '+e.message+'</div>';return;}
  _initSelectedRunIds();renderMeta();initTabs();renderRunSelectors('fpsRunSelectors');renderRunSelectors('overviewRunSelectors');initFpsFilters();initOverviewFilters();
  FpsChart.init(document.getElementById('fpsCompareChart'),handleFpsEnvClick);
  Chart.init(document.getElementById('mainChart'),function(idx,d){state.selectedEnvId=d.envId;renderEnvDetail(d.env,{scroll:true});Chart.update(state.chartData,state.selectedEnvId);});
  refreshFpsCompare();refreshChart();initDetailTab();
  /* Version Trend tab */
  if((state.dataset.snapshots||[]).length){
    initTrendChart();
    initTrendTab();
  }
}
main();
"""


STYLES_CSS = r""":root {
  --bg: #f4f1eb;
  --panel: rgba(255, 255, 255, 0.92);
  --ink: #1c2430;
  --muted: #5d6572;
  --accent: #1b6b58;
  --accent-soft: #deeee7;
  --border: rgba(28, 36, 48, 0.12);
  --table-stripe: rgba(27, 107, 88, 0.04);
  --tab-active: #1b6b58;
  --tab-inactive: #e8e4dd;
  font-family: 'IBM Plex Sans', 'Pretendard', sans-serif;
}
*, *::before, *::after { box-sizing: border-box; }
body { margin: 0; background: radial-gradient(circle at top left, #fffdf8, var(--bg)); color: var(--ink); }
.page { max-width: 1400px; margin: 0 auto; padding: 32px; }

.hero { display: flex; justify-content: space-between; align-items: flex-start; gap: 24px; margin-bottom: 24px; }
.hero h1 { margin: 4px 0 8px; }
.eyebrow { margin: 0; letter-spacing: 0.08em; text-transform: uppercase; color: var(--accent); font-weight: 700; font-size: 13px; }
.lede { color: var(--muted); max-width: 60ch; margin: 0; }
.meta-card { background: var(--panel); border: 1px solid var(--border); border-radius: 14px; backdrop-filter: blur(12px); min-width: 280px; padding: 16px 20px; font-size: 14px; }
.meta-card div { display: flex; justify-content: space-between; gap: 16px; padding: 4px 0; }

.tabs { display: flex; gap: 4px; margin-bottom: 20px; }
.tab { padding: 10px 24px; border: none; border-radius: 10px 10px 0 0; background: var(--tab-inactive); color: var(--muted); font-size: 14px; font-weight: 600; cursor: pointer; transition: background 0.2s, color 0.2s; }
.tab:hover { background: #d9d5ce; }
.tab.active { background: var(--tab-active); color: #fff; }
.tab-content { display: none; }
.tab-content.active { display: block; }

.controls { display: flex; flex-wrap: wrap; gap: 16px; padding: 16px 20px; margin-bottom: 20px; background: var(--panel); border: 1px solid var(--border); border-radius: 14px; backdrop-filter: blur(12px); }
.run-selector-panel { display: block; }
.control-heading { font-size: 13px; font-weight: 700; color: var(--muted); margin-bottom: 14px; text-transform: uppercase; letter-spacing: 0.04em; }
.run-selector-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(220px, 1fr)); gap: 16px; }
.run-selector-grid label { display: flex; flex-direction: column; gap: 6px; font-size: 13px; font-weight: 600; color: var(--muted); }
.controls label { display: flex; flex-direction: column; gap: 6px; font-size: 13px; font-weight: 600; color: var(--muted); }
select { min-width: 200px; padding: 10px 12px; border-radius: 10px; border: 1px solid var(--border); background: #fff; font-size: 14px; color: var(--ink); }
select:focus { outline: 2px solid var(--accent); outline-offset: -1px; }

.panel { background: var(--panel); border: 1px solid var(--border); border-radius: 18px; backdrop-filter: blur(12px); padding: 24px; margin-bottom: 20px; }
.panel h2 { margin-top: 0; margin-bottom: 4px; }
.chart-panel { padding: 24px 24px 16px; }
.chart-header { margin-bottom: 8px; }
.chart-subtitle { margin: 0; font-size: 14px; color: var(--muted); }
.chart-container { position: relative; width: 100%; height: 480px; }
.chart-hint { margin: 8px 0 0; font-size: 13px; color: var(--muted); text-align: center; }

.env-detail { animation: slideIn 0.25s ease-out; }
@keyframes slideIn { from { opacity: 0; transform: translateY(-10px); } to { opacity: 1; transform: translateY(0); } }
.env-detail h2 { margin-bottom: 16px; }
.env-detail-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 32px; }
.env-detail-grid--3 { grid-template-columns: 1fr 1fr 1fr; }
.model-meta-table--wide { font-size: 14px; }
.model-meta-table--wide th, .model-meta-table--wide td { padding: 10px 12px; }
.bench-table { table-layout: fixed; width: 100%; }
.bench-table td:nth-child(n+3), .bench-table th:nth-child(n+3) { text-align: right; }
.env-detail-col h3 { margin: 0 0 12px; font-size: 15px; color: var(--accent); border-bottom: 2px solid var(--accent-soft); padding-bottom: 6px; }
.info-row { display: flex; justify-content: space-between; gap: 12px; padding: 6px 0; border-bottom: 1px solid var(--border); font-size: 14px; }
.info-key { font-weight: 600; color: var(--muted); min-width: 100px; }
.info-val { text-align: right; word-break: break-word; }

.model-meta-title { margin: 24px 0 12px; font-size: 16px; color: var(--accent); border-top: 2px solid var(--accent-soft); padding-top: 16px; }
.model-meta-grid { display: grid; grid-template-columns: repeat(5, 1fr); gap: 12px; }
.model-meta-card { background: rgba(27,107,88,0.03); border: 1px solid var(--border); border-radius: 10px; padding: 10px; }
.model-meta-card h4 { margin: 0 0 8px; font-size: 13px; color: var(--accent); text-align: center; }
.model-meta-card.empty-card { display: flex; flex-direction: column; align-items: center; justify-content: center; min-height: 100px; }
.model-meta-table { width: 100%; border-collapse: collapse; font-size: 11px; }
.model-meta-table th, .model-meta-table td { padding: 4px 5px; border-bottom: 1px solid var(--border); text-align: center; }
.model-meta-table th { background: var(--accent-soft); font-size: 10px; font-weight: 700; text-transform: uppercase; }
.empty-state.small { font-size: 12px; padding: 10px; }

.decode-path-summary { font-size: 13px; color: var(--muted); margin: 0 0 10px 0; line-height: 1.8; }
.decode-path-summary b { color: var(--fg); font-weight: 600; }
.tag { display: inline-block; padding: 1px 6px; border-radius: 4px; font-size: 11px; font-weight: 600; }
.tag--warn { background: #fff3cd; color: #856404; }
.tag--ok { background: #d4edda; color: #155724; }

.trend-link { margin: 10px 0 0; font-size: 13px; }
.trend-link a { color: var(--accent); text-decoration: none; font-weight: 600; }
.trend-link a:hover { text-decoration: underline; }
.snapshot-sw-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 0 32px; max-width: 500px; }

.task-section + .task-section { margin-top: 28px; }
.task-section h3 { margin-top: 0; }
.detail-table { table-layout: fixed; width: 100%; }
.cell-best { font-weight: 700; }
.empty-state { padding: 20px; color: var(--muted); border: 1px dashed var(--border); border-radius: 12px; text-align: center; }
table { width: 100%; border-collapse: collapse; font-size: 14px; }
th, td { padding: 10px 12px; border-bottom: 1px solid var(--border); text-align: left; vertical-align: top; }
thead { background: var(--accent-soft); }
tbody tr:nth-child(even) { background: var(--table-stripe); }
.summary-table td:nth-child(n + 3), .summary-table th:nth-child(n + 3) { text-align: right; }

@media (max-width: 980px) {
  .page { padding: 16px; }
  .hero { flex-direction: column; }
  .controls { flex-direction: column; }
  .meta-card { min-width: 100%; }
  select { min-width: 100%; }
  .env-detail-grid { grid-template-columns: 1fr; gap: 16px; }
  .chart-container { height: 360px; }
  .summary-table, table { display: block; overflow-x: auto; }
  .model-meta-grid { grid-template-columns: repeat(2, 1fr); }
}

@media print {
  :root { -webkit-print-color-adjust: exact; print-color-adjust: exact; }
  body { background: #fff; }
  .page { max-width: none; padding: 0; }
  .tabs, .chart-hint, .controls { display: none !important; }
  .hero, .panel, .task-section { break-inside: avoid; page-break-inside: avoid; }
  .hero { margin-bottom: 14px; }
  .tab-content { display: none !important; }
  .tab-content.active { display: block !important; }
  .panel { margin-bottom: 14px; padding: 16px; box-shadow: none; backdrop-filter: none; }
  .chart-container { height: 420px; }
  table { font-size: 12px; }
  th, td { padding: 8px 10px; }
}
"""


def build_static_dashboard(dataset: dict, output_dir: Path) -> Path:
    output_dir.mkdir(parents=True, exist_ok=True)
    save_dataset_json(dataset, output_dir / "dataset.json")
    index_html = INDEX_HTML.replace("__DATASET_JSON__", json.dumps(dataset))
    (output_dir / "index.html").write_text(index_html, encoding="utf-8")
    (output_dir / "app.js").write_text(APP_JS, encoding="utf-8")
    (output_dir / "styles.css").write_text(STYLES_CSS, encoding="utf-8")
    return output_dir / "index.html"
