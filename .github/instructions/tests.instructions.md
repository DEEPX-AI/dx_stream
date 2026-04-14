---
applyTo: "test/**"
---

# Tests — Contextual Instructions

Working on dx_stream test files.

## Required Context
- `.deepx/skills/dx-validate.md`
- `.deepx/instructions/testing-patterns.md`

## Rules
- pytest markers: `@pytest.mark.npu_required`, `@pytest.mark.slow`, `@pytest.mark.smoke`
- No-NPU tests: `pytest test/ -m "not npu_required"`
