QT += quick quickcontrols2

CONFIG += c++11
CONFIG += link_pkgconfig
CONFIG -= thread
LIBS += -lpthread

TARGET   = gui-app
TEMPLATE = app

INCLUDEPATH += src

SOURCES += \
    src/main.cpp \
    src/backend.cpp

HEADERS += \
    src/backend.h

RESOURCES += resources.qrc

DEFINES += QT_DEPRECATED_WARNINGS

# ── Jetson Nano deployment ────────────────────────────────────────────────
# Build:
#   qmake && make -j4
#
# Run X11:
#   ./gui-app
#
# Run EGLFS (headless, no X11):
#   QT_QPA_PLATFORM=eglfs ./gui-app
#
# Run linuxfb:
#   QT_QPA_PLATFORM=linuxfb QT_QPA_FB=/dev/fb0 ./gui-app