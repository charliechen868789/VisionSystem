# gui-app — Qt/QML Launcher for Jetson Nano

4-tile home screen launcher with dark-glass UI. Backend is a C++ `QObject`
exposed to QML as a context property (`backend`).

## Project layout

```
gui-app/
├── backend.cpp               C++ backend – hardware/network logic
├── backend.h                 Q_PROPERTY declarations for all pages
├── main.cpp                  App entry – registers `backend` context property
├── gui-app.pro               qmake project
├── resources.qrc             Qt resource bundle
├── background.png            ← copy your background image here
└── qml/
    ├── main.qml              Home screen (StackView + 2×2 NavButton grid)
    ├── NavButton.qml         Reusable dark-glass tile button
    ├── BasePage.qml          Shared sub-page base (title bar + Back button)
    ├── ControlHardwarePage.qml
    ├── VideoControlPage.qml
    ├── SettingsPage.qml
    └── ConnectionPage.qml
```

## Prerequisites (Jetson Nano — L4T Ubuntu 18.04/20.04)

```bash
sudo apt install qt5-default \
    qml-module-qtquick2 \
    qml-module-qtquick-controls2 \
    qml-module-qtquick-layouts \
    qml-module-qtgraphicaleffects
```

## Build

```bash
cp /path/to/background.png gui-app/
cd gui-app
qmake && make -j4
```

## Run

```bash
# X11 desktop
./gui-app

# Headless / EGLFS (recommended for production on Nano)
# Plain `eglfs` defaults to the DRM/KMS backend, which fails with
# "Could not find DRM device!" on boards with no /dev/dri (this board's
# L4T display driver is the legacy tegra_dc stack, not DRM/KMS). NVIDIA's
# Tegra EGL device stack is present instead (libEGL_nvidia.so.0,
# eglfs_kms_egldevice plugin, EGL_EXT_platform_device) — select it
# explicitly to get real GPU-accelerated rendering:
QT_QPA_PLATFORM=eglfs QT_QPA_EGLFS_INTEGRATION=eglfs_kms_egldevice ./gui-app

# Framebuffer fallback (software rendering — do NOT use if the above
# works; this SDK's Qt Quick software backend has a scenegraph bug where
# the dirty-region tracker always collapses to empty and nothing ever
# gets painted, confirmed even with a trivial one-Rectangle QML file)
QT_QPA_PLATFORM=linuxfb QT_QPA_FB=/dev/fb0 ./gui-app
```

## Extending the backend

All page state lives in `Backend` (`backend.h` / `backend.cpp`).
Add a `Q_PROPERTY` + getter/setter/signal, implement the setter, then bind
in QML:

```qml
// Read
Text { text: backend.someValue }

// Write
Switch { onToggled: backend.someValue = checked }

// Call a slot
Button { onClicked: backend.doSomething() }
```

## Auto-start (systemd)

```ini
# /etc/systemd/system/gui-app.service
[Unit]
Description=GUI App Launcher
After=graphical.target

[Service]
User=nvidia
Environment=QT_QPA_PLATFORM=eglfs
Environment=QT_QPA_EGLFS_INTEGRATION=eglfs_kms_egldevice
ExecStart=/home/nvidia/gui-app/gui-app
Restart=on-failure

[Install]
WantedBy=graphical.target
```
