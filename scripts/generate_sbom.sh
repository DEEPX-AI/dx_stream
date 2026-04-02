#!/bin/bash
# Generate CycloneDX SBOM for the full DX-Stream project
# Scans all meson.build files + Python bindings
#
# Usage:
#   ./scripts/generate_sbom.sh                    # default output: bom.cdx.json
#   ./scripts/generate_sbom.sh -o my_sbom.json    # custom output path

SCRIPT_DIR=$(realpath "$(dirname "$0")")
PROJECT_ROOT=$(realpath "${SCRIPT_DIR}/..")

source "${PROJECT_ROOT}/scripts/color_env.sh"
source "${PROJECT_ROOT}/scripts/common_util.sh"

print_colored "Generating SBOM (full project scan)..." "INFO"
python3 "${SCRIPT_DIR}/generate_sbom.py" "$@"

if [ $? -eq 0 ]; then
    print_colored "SBOM generation complete." "SUCCESS"
else
    print_colored "SBOM generation failed." "ERROR"
    exit 1
fi
