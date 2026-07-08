#!/usr/bin/env bash
# setup_benchmark_env.sh — One-time environment setup for benchmark automation
#
# Configures:
#   1. Passwordless sudo for dxrt.service restart (crash recovery)
#   2. Passwordless sudo for dmesg (kernel log collection on incidents)
#   3. Passwordless sudo for journalctl (dxrt service log collection on incidents)
#   4. systemd-journal group membership (journal access without sudo)
#
# Usage:
#   sudo ./setup_benchmark_env.sh          # auto-detect current $SUDO_USER
#   sudo ./setup_benchmark_env.sh <user>   # specify a target user explicitly

set -euo pipefail

SUDOERS_FILE="/etc/sudoers.d/benchmark-dxrt"

# ── Determine target user ─────────────────────────────────────────────────
TARGET_USER="${1:-${SUDO_USER:-}}"

if [[ -z "${TARGET_USER}" ]]; then
    echo "ERROR: Cannot determine target user."
    echo "Usage: sudo $0 [username]"
    exit 1
fi

# Verify user exists
if ! id "${TARGET_USER}" &>/dev/null; then
    echo "ERROR: User '${TARGET_USER}' does not exist on this system."
    exit 1
fi

# ── Must run as root ──────────────────────────────────────────────────────
if [[ "$(id -u)" -ne 0 ]]; then
    echo "ERROR: This script must be run with sudo (or as root)."
    echo "Usage: sudo $0 [username]"
    exit 1
fi

echo "[setup] Target user: ${TARGET_USER}"

# ── Resolve command paths ─────────────────────────────────────────────────
SYSTEMCTL_BIN="$(command -v systemctl 2>/dev/null || echo /usr/bin/systemctl)"
DMESG_BIN="$(command -v dmesg 2>/dev/null || echo /usr/bin/dmesg)"
JOURNALCTL_BIN="$(command -v journalctl 2>/dev/null || echo /usr/bin/journalctl)"

# ── Install sudoers rules ─────────────────────────────────────────────────
# Build multi-line sudoers content
SUDOERS_CONTENT="# Benchmark automation: passwordless sudo for dxrt crash recovery and incident log collection
${TARGET_USER} ALL=(ALL) NOPASSWD: ${SYSTEMCTL_BIN} restart dxrt.service
${TARGET_USER} ALL=(ALL) NOPASSWD: ${DMESG_BIN} --time-format=iso -T
${TARGET_USER} ALL=(ALL) NOPASSWD: ${JOURNALCTL_BIN} -u dxrt.service *"

# Check if already fully configured
if [[ -f "${SUDOERS_FILE}" ]] && \
   grep -qF "restart dxrt.service" "${SUDOERS_FILE}" && \
   grep -qF "${DMESG_BIN}" "${SUDOERS_FILE}" && \
   grep -qF "${JOURNALCTL_BIN}" "${SUDOERS_FILE}"; then
    echo "[setup] Sudoers rules already configured. Skipping."
else
    echo "${SUDOERS_CONTENT}" > "${SUDOERS_FILE}"
    chmod 0440 "${SUDOERS_FILE}"

    # Validate with visudo — rollback on failure
    if visudo -cf "${SUDOERS_FILE}" &>/dev/null; then
        echo "[setup] Sudoers rules installed: ${SUDOERS_FILE}"
        echo "        - systemctl restart dxrt.service"
        echo "        - dmesg (kernel log for incident collection)"
        echo "        - journalctl (service log for incident collection)"
    else
        echo "ERROR: Sudoers validation failed. Removing broken file."
        rm -f "${SUDOERS_FILE}"
        exit 1
    fi
fi

# ── Add user to systemd-journal group (journal access without sudo) ───────
if getent group systemd-journal &>/dev/null; then
    if id -nG "${TARGET_USER}" | grep -qw systemd-journal; then
        echo "[setup] User '${TARGET_USER}' already in systemd-journal group."
    else
        usermod -aG systemd-journal "${TARGET_USER}"
        echo "[setup] Added '${TARGET_USER}' to systemd-journal group."
        echo "        (re-login required for group to take effect)"
    fi
else
    echo "[setup] systemd-journal group not found. Skipping group membership."
fi

# ── Verify passwordless sudo works ────────────────────────────────────────
echo ""
echo "[setup] Verifying passwordless sudo ..."

# Test dmesg
if sudo -n -u "${TARGET_USER}" -- sudo -n "${DMESG_BIN}" --version &>/dev/null 2>&1; then
    echo "  sudo -n dmesg          — OK"
else
    echo "  sudo -n dmesg          — installed (cannot verify as ${TARGET_USER})"
fi

# Test journalctl
if sudo -n -u "${TARGET_USER}" -- sudo -n "${JOURNALCTL_BIN}" --version &>/dev/null 2>&1; then
    echo "  sudo -n journalctl     — OK"
else
    echo "  sudo -n journalctl     — installed (cannot verify as ${TARGET_USER})"
fi

# Test systemctl
echo "  sudo -n systemctl restart dxrt.service — rule installed"

# ── Summary ───────────────────────────────────────────────────────────────
echo ""
echo "=== Setup Complete ==="
echo "  User:  ${TARGET_USER}"
echo "  Rule:  ${SUDOERS_FILE}"
echo "  Group: systemd-journal"
echo ""
echo "  Passwordless sudo enabled for:"
echo "    - systemctl restart dxrt.service  (crash recovery)"
echo "    - dmesg                           (kernel log collection)"
echo "    - journalctl                      (service log collection)"
echo ""
echo "To remove this configuration later:"
echo "  sudo rm ${SUDOERS_FILE}"
