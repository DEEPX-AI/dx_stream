# Multi-Stream Domain (application/x-dxvideoraw)

DX-STREAM uses a dedicated caps domain — **`application/x-dxvideoraw`** — to merge multiple `video/x-raw` streams into a single processing chain while preserving each stream's identity, lifecycle, and per-frame metadata. This chapter is a single reference for how that domain works; element-specific pages link back here.

When the multi-stream domain is not used (single-stream pipelines), every DX-STREAM element behaves like a normal GStreamer element negotiating `video/x-raw`. This chapter is only relevant when you place elements between **DxInputSelector** and **DxOutputSelector**.

---

## Domain Boundary

```
[upstream] N × video/x-raw
        ↓ (N sink pads)
    dxinputselector                       ← domain entry
        ↓ 1 src pad, caps = application/x-dxvideoraw
    middle elements …                     ← all negotiate dxvideoraw
        ↓
    dxoutputselector                      ← domain exit
        ↓ N src pads, caps = video/x-raw
[downstream] N × video/x-raw
```

- Inside the domain, all elements negotiate `application/x-dxvideoraw` on both sink and src.
- Standard GStreamer elements (e.g., `videoconvert`, `videoscale`, `compositor`) cannot accept `application/x-dxvideoraw` and will fail caps negotiation. The caps system itself prevents incorrect placement.

---

## Caps Definition

```
application/x-dxvideoraw,
  width=(int)[1, MAX],
  height=(int)[1, MAX],
  framerate=(fraction)[0/1, MAX]
```

Caps fields are intentionally *ranges* and `format` is omitted to accept any pixel format. Actual per-buffer values (`stream_id`, `width`, `height`, `format`) are carried inside **`DXFrameMeta`** attached to each `GstBuffer`. This is what lets a single chain process multiple streams that may differ in resolution and color format.

---

## Element Placement Matrix

| Element | Inside domain | Outside domain | Notes |
|---|:-:|:-:|---|
| `dxpreprocess`, `dxinfer`, `dxpostprocess`, `dxosd`, `dxrate`, `dxtracker` | OK | OK | Dual mode — behavior is chosen from caps |
| `dxmsgconv`, `dxmsgbroker` | OK | OK | Dual, caps-agnostic (operate on metadata) |
| `dxscale`, `dxconvert` | NOT allowed | OK | Need stable single caps; multi-stream multi-resolution unsupported |
| `dxgather` | NOT allowed | OK | Merges branches that share a single source — does not support multiple stream IDs |
| `dxinputselector`, `dxoutputselector` | boundary only | NOT used | Domain entry / exit |

`dxscale` and `dxconvert` must be placed **per stream upstream of `dxinputselector`** or **per output downstream of `dxoutputselector`**:

```
src1 ! decode1 ! dxscale ! dxconvert ! \
src2 ! decode2 ! dxscale ! dxconvert !  dxinputselector ! ... ! dxoutputselector ! ...
src3 ! decode3 ! dxscale ! dxconvert ! /
```

---

## Per-Buffer Identity

Every buffer inside the domain carries a **`DXFrameMeta`**. Dual-mode elements use `DXFrameMeta` (not caps) to read the buffer's actual dimensions, format, and stream identity:

| Field | Purpose |
|---|---|
| `_stream_id` | Logical stream index assigned by `dxinputselector` from the sink pad number |
| `_width`, `_height`, `_format` | Per-buffer dimensions/format (may differ between buffers) |

If a buffer arriving at `dxinputselector` has no `DXFrameMeta`, the selector creates one and sets `_stream_id` from the sink pad number.

---

## Event Model

Inside the domain, two layers of events coexist:

- **L1A — domain-wide standard events.** A single `STREAM_START`, `CAPS` (`application/x-dxvideoraw`), `SEGMENT`, and (eventually) `EOS` for the entire merged chain. `dxinputselector` emits these once; intermediate elements forward them through standard GStreamer paths.
- **L1B — pipeline-wide standard events.** `FLUSH_START`, `FLUSH_STOP`, `RECONFIGURE`. These are not stream-specific and are propagated normally.
- **L2 — per-stream lifecycle events.** Each input stream's original `STREAM_START`, `CAPS` (`video/x-raw`), `SEGMENT`, `TAG`, `EOS`, `GAP` is **wrapped** as a `CUSTOM_DOWNSTREAM` event of type `application/x-dx-wrapped-event` that carries the stream-id. Upstream-direction `QoS` and `RECONFIGURE` are wrapped similarly as `CUSTOM_UPSTREAM` events.

Downstream behavior:

- `dxinputselector` emits L1A sticky events once, and wraps every per-stream sticky event as an L2 event.
- Intermediate (dual-mode) elements either ignore L2 events (they are not their own caps) or peek at them to update per-stream state (e.g., `dxosd` learns each stream's `video_info` from wrapped CAPS).
- `dxoutputselector` drops L1A events (the global merged ones), broadcasts L1B (FLUSH/RECONFIGURE) to every output, and unwraps each L2 event to the matching srcpad for that stream-id.

Upstream behavior:

- A `QoS` event arriving on one of `dxoutputselector`'s src pads is wrapped with the stream-id and sent upstream. `dxinputselector` unwraps it and routes it to the corresponding sink (so a slow stream throttles its own source instead of throttling every input).

You do not need to construct or inspect L2 events yourself — the boundary elements handle all wrap/unwrap.

---

## Query Policy

`dxinputselector` and `dxoutputselector` answer standard GStreamer queries so that LATENCY, ALLOCATION, POSITION, and DURATION work transparently across the domain. In particular:

- **LATENCY**: each intermediate element adds its own time to the upstream latency; `dxoutputselector` forwards LATENCY queries from each src pad to the sink peer.
- **ALLOCATION**: dual-mode elements in domain mode request `DXFrameMeta` as a buffer meta during allocation.
- **CAPS / ACCEPT_CAPS**: standard responses against the pad template (`application/x-dxvideoraw` inside the domain).

LATENCY inside the domain is reported as a single, stream-agnostic value — the same NPU, kernel and queues are shared by all streams.

---

## Troubleshooting

- **"Could not link element …"** between `dxinputselector` and a downstream element usually means the downstream element does not accept `application/x-dxvideoraw`. See `Troubleshooting and FAQ → Domain caps mismatch`.
- **Missing `DXFrameMeta`** inside the domain means a buffer entered the chain without going through `dxinputselector`. See `Troubleshooting and FAQ → DXFrameMeta missing`.

---
