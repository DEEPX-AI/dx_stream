"""Model catalog – discover .dxnn models and classify by task/size.

Builds a sorted list of (model_name, abs_path, task, size) tuples
from a model directory, with filtering by task and size.
"""

import re
from dataclasses import dataclass
from pathlib import Path

from .config import TASK_MAP, SIZES, MODEL_DIR


@dataclass
class ModelEntry:
    """One YOLO26 model file."""
    name: str          # e.g. "yolo26s.dxnn" or "yolo26s-pose.dxnn"
    path: Path         # absolute path
    task: str          # e.g. "object_detection"
    task_suffix: str   # e.g. "od", "pose", "seg"
    size: str          # e.g. "n", "s", "m", "l", "x"

    def __str__(self) -> str:
        return f"{self.name} (task={self.task}, size={self.size})"


# Pattern: yolo26{size}.dxnn  OR  yolo26{size}-{suffix}.dxnn
_MODEL_RE = re.compile(r"^yolo26([nslmx])(?:-(.+))?\.dxnn$")

# sort keys
_SIZE_ORDER = {s: i for i, s in enumerate(SIZES)}
_TASK_ORDER = {k: i for i, k in enumerate(TASK_MAP.keys())}


def discover_models(model_dir: Path = MODEL_DIR) -> list[ModelEntry]:
    """Scan *model_dir* for .dxnn files and return classified entries."""
    entries = []
    for p in sorted(model_dir.glob("*.dxnn")):
        m = _MODEL_RE.match(p.name)
        if not m:
            continue
        size = m.group(1)
        suffix = m.group(2) or "od"  # no suffix → object detection
        task = TASK_MAP.get(suffix)
        if task is None:
            continue
        entries.append(ModelEntry(
            name=p.name,
            path=p.resolve(),
            task=task,
            task_suffix=suffix,
            size=size,
        ))
    entries.sort(key=lambda e: (_TASK_ORDER.get(e.task_suffix, 99), _SIZE_ORDER.get(e.size, 99)))
    return entries


def filter_models(
    entries: list[ModelEntry],
    task: str | None = None,
    sizes: list[str] | None = None,
) -> list[ModelEntry]:
    """Filter model entries by task name and/or sizes."""
    result = entries
    if task:
        result = [e for e in result if e.task == task]
    if sizes:
        result = [e for e in result if e.size in sizes]
    return result
