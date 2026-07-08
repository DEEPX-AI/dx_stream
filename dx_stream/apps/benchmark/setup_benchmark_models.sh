#!/bin/bash
# Download benchmark models listed in model_list.json to assets/models/.
set -euo pipefail

SCRIPT_DIR=$(realpath "$(dirname "$0")")
BASE_URL="https://sdk.deepx.ai/modelzoo/dxnn"
MODEL_LIST_JSON="$SCRIPT_DIR/model_list.json"
OUTPUT_DIR="$SCRIPT_DIR/assets/models"

if ! command -v jq >/dev/null 2>&1; then
    echo "[ERROR] jq is required. Install with: sudo apt-get install -y jq"
    exit 1
fi

if [ ! -f "$MODEL_LIST_JSON" ]; then
    echo "[ERROR] model_list.json not found: $MODEL_LIST_JSON"
    exit 1
fi

MODEL_VERSION=$(jq -r '.version' "$MODEL_LIST_JSON")
mapfile -t MODEL_FILES < <(jq -r '.models[]' "$MODEL_LIST_JSON")

mkdir -p "$OUTPUT_DIR"

echo "[INFO] Downloading ${#MODEL_FILES[@]} models (version=$MODEL_VERSION) to $OUTPUT_DIR"

for model in "${MODEL_FILES[@]}"; do
    dest="$OUTPUT_DIR/$model"
    if [ -s "$dest" ]; then
        echo "  [SKIP] $model (already exists)"
        continue
    fi
    if [ -e "$dest" ]; then
        echo "  [REDO] $model (empty or incomplete file)"
    fi
    url="$BASE_URL/$MODEL_VERSION/$model"
    tmp_dest=$(mktemp "$OUTPUT_DIR/.${model}.XXXXXX.part")
    echo "  [GET]  $model"
    if ! curl -fSL -o "$tmp_dest" "$url"; then
        echo "[ERROR] Failed to download $url"
        rm -f "$tmp_dest"
        exit 1
    fi
    mv "$tmp_dest" "$dest"
done

for model in "${MODEL_FILES[@]}"; do
    dest="$OUTPUT_DIR/$model"
    if [ ! -s "$dest" ]; then
        echo "[ERROR] Required model is still missing or empty: $dest"
        exit 1
    fi
done

echo "[OK] All models ready in $OUTPUT_DIR"
