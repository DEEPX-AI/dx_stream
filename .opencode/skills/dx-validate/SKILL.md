---
name: dx-validate
description: Run pipeline validation checks for dx_stream framework and application code
---

# Validation

Read `.deepx/skills/dx-validate.md` for full patterns.

## Quick Reference
```bash
python .deepx/scripts/validate_framework.py
python .deepx/scripts/validate_app.py <dir>
pytest test/ -m "not npu_required"
```
