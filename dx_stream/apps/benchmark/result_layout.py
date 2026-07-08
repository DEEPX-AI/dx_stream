"""Result layout helpers for nested benchmark results directories."""

from __future__ import annotations

import re
from pathlib import Path


def _format_hw_config(h1_cards: int, m1_modules: int) -> str:
    parts: list[str] = []
    if h1_cards > 0:
        parts.append("H1" if h1_cards == 1 else f"H1x{h1_cards}")
    if m1_modules > 0:
        parts.append("M1" if m1_modules == 1 else f"M1x{m1_modules}")
    return "-".join(parts) if parts else "unknown"


def _parse_topology_from_raw(raw: str) -> tuple[int, int] | None:
    device_count = 0
    device_boards: list[str | None] = []
    current_board: str | None = None
    for line in raw.splitlines():
        stripped = line.strip()
        if stripped.startswith("* Device"):
            if device_count > 0:
                device_boards.append(current_board)
            current_board = None
            device_count += 1
        elif "Board" in stripped and ":" in stripped and "Chip" not in stripped:
            match = re.search(r":\s*(.+)", stripped)
            if match:
                current_board = match.group(1).strip().split(",")[0].strip()

    if device_count > 0:
        device_boards.append(current_board)
    if device_count == 0:
        return None

    h1_chips = sum(1 for board in device_boards if board and "H1" in board.upper())
    h1_cards = (h1_chips + 3) // 4 if h1_chips > 0 else 0
    m1_modules = max(0, device_count - h1_chips)
    return h1_cards, m1_modules


def _parse_topology_from_sku(sku: str) -> tuple[int, int] | None:
    if not sku or sku == "unknown":
        return None

    h1_match = re.search(r"H1(?:\s*[x×]\s*(\d+))?", sku, re.IGNORECASE)
    m1_match = re.search(r"M1(?:\s*[x×]\s*(\d+))?", sku, re.IGNORECASE)
    if not h1_match and not m1_match:
        return None

    h1_cards = int(h1_match.group(1)) if h1_match and h1_match.group(1) else (1 if h1_match else 0)
    m1_modules = int(m1_match.group(1)) if m1_match and m1_match.group(1) else (1 if m1_match else 0)
    return h1_cards, m1_modules


def _get_hw_config(npu: dict) -> str:
    hw_config = npu.get("hw_config")
    if hw_config and hw_config != "unknown":
        return str(hw_config)

    h1_cards = npu.get("h1_cards")
    m1_modules = npu.get("m1_modules")
    if h1_cards is not None or m1_modules is not None:
        return _format_hw_config(int(h1_cards or 0), int(m1_modules or 0))

    raw = npu.get("raw")
    if isinstance(raw, str):
        parsed = _parse_topology_from_raw(raw)
        if parsed is not None:
            return _format_hw_config(*parsed)

    sku = npu.get("sku")
    if isinstance(sku, str):
        parsed = _parse_topology_from_sku(sku)
        if parsed is not None:
            return _format_hw_config(*parsed)

    device_count = int(npu.get("device_count", 0) or 0)
    if device_count > 0:
        return _format_hw_config(0, device_count)
    return "unknown"


def make_hw_id(fingerprint: dict) -> str:
    """Generate hardware identifier from environment fingerprint.

    Format: {name}_{hw_config}
    Uses product_name if set via --product-name, otherwise falls back to hostname.
    """
    host = fingerprint.get("host", {})
    npu = fingerprint.get("npu", {})
    name = fingerprint.get("product_name") or host.get("hostname", "unknown")
    hw_config = _get_hw_config(npu)
    raw = f"{name}_{hw_config}"
    raw = re.sub(r"[^A-Za-z0-9_.-]", "_", raw)
    return re.sub(r"_+", "_", raw).strip("_")


def iter_result_dirs(root: Path) -> list[Path]:
    """Return nested run directories under results/{hw_id}/{run_id}."""
    if not root.is_dir():
        return []

    flat_dirs: list[Path] = []
    run_dirs: list[Path] = []
    for child in sorted(root.iterdir()):
        if not child.is_dir() or child.name == "dashboard":
            continue
        if (child / "environment.json").exists():
            flat_dirs.append(child)
            continue
        for grandchild in sorted(child.iterdir()):
            if grandchild.is_dir() and (grandchild / "environment.json").exists():
                run_dirs.append(grandchild)

    if flat_dirs:
        sample = ", ".join(path.name for path in flat_dirs[:3])
        raise ValueError(
            f"Flat result layout is no longer supported: {sample}. "
            "Expected results/{hw_id}/{run_id}."
        )

    return run_dirs