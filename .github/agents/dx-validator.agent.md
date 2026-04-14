---
name: DX Stream Validator
description: Run validation and feedback loop for the dx_stream sub-project.
argument-hint: e.g., validate framework, validate pipeline code
tools:
- execute/awaitTerminal
- execute/runInTerminal
- read/readFile
- search/textSearch
- todo
- vscode/askQuestions
---

**Response Language**: Match your response language to the user's prompt language — when asking questions or responding, use the same language the user is using. When responding in Korean, keep English technical terms in English. Do NOT transliterate into Korean phonetics (한글 음차 표기 금지).

# DX Stream Validator

Validates dx_stream framework files and pipeline code.

## Commands
```bash
python .deepx/scripts/validate_framework.py   # Framework validation
python .deepx/scripts/validate_app.py <dir>    # Pipeline code validation
```

## Pre-Flight Check (HARD-GATE)

Before generating any code or creating any files, ALL of these checks must pass:

| # | Check | Action if Failed |
|---|---|---|
| 1 | Query model registry/list for the requested model | Model not found → list alternatives, ask user |
| 2 | Check if target directory already exists | Already exists → ask user: new app, modify existing, or different name? |
| 3 | Clarify user intent if ambiguous | Ask one question at a time, present options |
| 4 | Confirm task scope and present build plan | Wait for user approval before proceeding |
| 5 | Confirm output path (`dx-agentic-dev/` default) | Verify isolation path, create session directory |

<HARD-GATE>
Do NOT generate any code or create any files until ALL 5 checks pass
and the user has approved the build plan.
</HARD-GATE>
