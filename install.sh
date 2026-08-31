#!/usr/bin/env bash
# Build and install aeroboard (VisionSystem) onto a Jetson Nano (or any
# Debian/Ubuntu box). Installs binaries to /usr/bin, configs to
# /etc/aeroboard, creates /opt/models and /var/log/aeroboard, and writes
# (but does not enable) systemd units for eventhub and gui-app.
#
# Usage:
#   ./install.sh                # apt deps + configure + build + install
#   ./install.sh --skip-deps    # skip "apt-get install" step
#   ./install.sh --skip-build   # reuse existing ./build directory
#   ./install.sh --no-gui       # don't build/install the Qt gui-app
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

BUILD_DIR="build"
SKIP_DEPS=0
SKIP_BUILD=0
WITH_GUI=1

for arg in "$@"; do
    case "$arg" in
        --skip-deps)  SKIP_DEPS=1 ;;
        --skip-build) SKIP_BUILD=1 ;;
        --no-gui)     WITH_GUI=0 ;;
        -h|--help)
            grep '^#' "$0" | sed 's/^# \{0,1\}//'
            exit 0
            ;;
        *)
            echo "Unknown option: $arg" >&2
            exit 1
            ;;
    esac
done

if [[ $EUID -eq 0 ]]; then
    echo "Run this as a normal user (it will sudo for the install steps), not as root." >&2
    exit 1
fi

# ── 1. Dependencies ─────────────────────────────────────────────────────────
if [[ $SKIP_DEPS -eq 0 ]]; then
    echo "==> Installing build dependencies"
    APT_PKGS=(
        build-essential cmake pkg-config
        libprotobuf-dev protobuf-compiler
        libzmq3-dev libevent-dev libcurl4-openssl-dev
        libopencv-dev nlohmann-json3-dev
    )
    if [[ $WITH_GUI -eq 1 ]]; then
        APT_PKGS+=(
            qt5-default
            qml-module-qtquick2
            qml-module-qtquick-controls2
            qml-module-qtquick-layouts
            qml-module-qtgraphicaleffects
        )
    fi
    sudo apt-get update
    sudo apt-get install -y --no-install-recommends "${APT_PKGS[@]}"
else
    echo "==> Skipping dependency install (--skip-deps)"
fi

# ── 2. Configure + build ────────────────────────────────────────────────────
if [[ $SKIP_BUILD -eq 0 ]]; then
    echo "==> Configuring (cmake -S . -B $BUILD_DIR)"
    cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release

    echo "==> Building"
    cmake --build "$BUILD_DIR" -j"$(nproc)"
else
    echo "==> Skipping build (--skip-build), using existing $BUILD_DIR"
fi

# ── 3. Install binaries + configs (CMake install() rules) ──────────────────
echo "==> Installing binaries to /usr/bin and configs to /etc/aeroboard"
sudo cmake --install "$BUILD_DIR"

# ── 4. Runtime directories ──────────────────────────────────────────────────
echo "==> Creating runtime directories"
sudo mkdir -p /opt/models
sudo mkdir -p /var/log/aeroboard
sudo chown "$(id -u):$(id -g)" /var/log/aeroboard

if [[ -z "$(ls -A /opt/models 2>/dev/null)" ]]; then
    echo
    echo "NOTE: /opt/models is empty. config/ai.json's active model (index 0)"
    echo "      expects yolov4-tiny.weights, yolov4-tiny.cfg and coco.names there."
    echo "      Copy your model files in before starting aeroboard-ai, or edit"
    echo "      /etc/aeroboard/ai.json to point at wherever you keep them."
    echo
fi

# ── 5. systemd units (written, not enabled) ─────────────────────────────────
echo "==> Writing systemd unit files (not enabled/started)"

sudo tee /etc/systemd/system/aeroboard-eventhub.service >/dev/null <<'EOF'
[Unit]
Description=aeroboard EventHub (orchestrates video/ai/system/sensor workers)
After=network.target

[Service]
ExecStart=/usr/bin/eventhub --config /etc/aeroboard/hub_config.json
Restart=on-failure

[Install]
WantedBy=multi-user.target
EOF

if [[ $WITH_GUI -eq 1 ]]; then
    sudo tee /etc/systemd/system/aeroboard-gui.service >/dev/null <<EOF
[Unit]
Description=aeroboard GUI launcher
After=graphical.target aeroboard-eventhub.service

[Service]
User=$(id -un)
Environment=QT_QPA_PLATFORM=eglfs
ExecStart=/usr/bin/gui-app --config /etc/aeroboard/gui_config.json
Restart=on-failure

[Install]
WantedBy=graphical.target
EOF
fi

sudo systemctl daemon-reload

echo
echo "==> Install complete."
echo
echo "Binaries : /usr/bin/{eventhub,aeroboard-video,aeroboard-sensor,aeroboard-system,aeroboard-ai$( [[ $WITH_GUI -eq 1 ]] && echo ',gui-app')}"
echo "Configs  : /etc/aeroboard/*.json"
echo "Models   : /opt/models (populate before starting aeroboard-ai)"
echo "Logs     : /var/log/aeroboard/eventhub.log"
echo
echo "Run manually:"
echo "  sudo eventhub --config /etc/aeroboard/hub_config.json"
if [[ $WITH_GUI -eq 1 ]]; then
    echo "  QT_QPA_PLATFORM=eglfs gui-app --config /etc/aeroboard/gui_config.json"
fi
echo
echo "Or enable on boot:"
echo "  sudo systemctl enable --now aeroboard-eventhub.service"
if [[ $WITH_GUI -eq 1 ]]; then
    echo "  sudo systemctl enable --now aeroboard-gui.service"
fi
