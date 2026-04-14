---
name: dx-build-pipeline-app
description: Build GStreamer pipeline application for dx_stream with 13 elements and 6 pipeline categories (single, multi, cascaded, tiled, parallel, broker)
---

# Build Pipeline App

Read `.deepx/skills/dx-build-pipeline-app.md` for complete patterns and templates.

## Quick Reference
1. Classify pipeline category (single/multi/cascaded/tiled/parallel/broker)
2. Select model and postprocess library
3. Construct pipeline string with proper element ordering
4. Ensure preprocess-id matching and queue placement
5. Validate with `python .deepx/scripts/validate_app.py <dir>`
