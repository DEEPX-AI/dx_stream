#!/bin/bash
# Download benchmark videos from AWS and extract to assets/videos/.
set -euo pipefail

SCRIPT_DIR=$(realpath "$(dirname "$0")")
BASE_URL="https://sdk.deepx.ai"
SOURCE_PATH="res/video/benchmark_videos.tar.gz"
OUTPUT_DIR="$SCRIPT_DIR/assets/videos"
REQUIRED_VIDEOS=(
    "od_benchmark_video.mp4"
    "obb_benchmark_video.mp4"
)

mkdir -p "$OUTPUT_DIR"

missing_videos=()
for video in "${REQUIRED_VIDEOS[@]}"; do
    if [ ! -s "$OUTPUT_DIR/$video" ]; then
        missing_videos+=("$video")
    fi
done

if [ ${#missing_videos[@]} -eq 0 ]; then
    echo "[OK] Benchmark videos already ready in $OUTPUT_DIR"
    ls -lh "$OUTPUT_DIR"/*.mp4 2>/dev/null || true
    exit 0
fi

ARCHIVE=$(mktemp "$OUTPUT_DIR/.benchmark_video.XXXXXX.tar.gz")
EXTRACT_DIR=$(mktemp -d "$OUTPUT_DIR/.benchmark_video_extract.XXXXXX")

cleanup() {
    rm -f "$ARCHIVE"
    rm -rf "$EXTRACT_DIR"
}

trap cleanup EXIT

echo "[INFO] Missing benchmark videos: ${missing_videos[*]}"
echo "[INFO] Downloading benchmark videos archive to $OUTPUT_DIR"
if ! curl -fSL -o "$ARCHIVE" "$BASE_URL/$SOURCE_PATH"; then
    echo "[ERROR] Failed to download $BASE_URL/$SOURCE_PATH"
    exit 1
fi

echo "[INFO] Extracting ..."
tar -xzf "$ARCHIVE" -C "$EXTRACT_DIR"

for video in "${REQUIRED_VIDEOS[@]}"; do
    extracted=$(find "$EXTRACT_DIR" -type f -name "$video" -print -quit)
    if [ -z "$extracted" ]; then
        echo "[ERROR] Required benchmark video not found after extraction: $video"
        exit 1
    fi
    mv "$extracted" "$OUTPUT_DIR/$video"
done

echo "[OK] Benchmark videos ready in $OUTPUT_DIR"
ls -lh "$OUTPUT_DIR"/*.mp4 2>/dev/null || true
