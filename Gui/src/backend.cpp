#include "backend.h"
#include <QDebug>

Backend::Backend(QObject *parent) : QObject(parent)
{
    qDebug() << "[Backend] initialised";
}

// ── Control Hardware ──────────────────────────────────────────────────────

void Backend::setGpio0(bool v)
{
    if (m_gpio0 == v) return;
    m_gpio0 = v;
    qDebug() << "[Backend] GPIO0 =" << v;
    // TODO: write sysfs / libgpiod
    emit gpio0Changed();
}

void Backend::setGpio1(bool v)
{
    if (m_gpio1 == v) return;
    m_gpio1 = v;
    qDebug() << "[Backend] GPIO1 =" << v;
    emit gpio1Changed();
}

void Backend::setPwmEnable(bool v)
{
    if (m_pwmEnable == v) return;
    m_pwmEnable = v;
    qDebug() << "[Backend] PWM enable =" << v;
    emit pwmEnableChanged();
}

void Backend::setSpiBus(bool v)
{
    if (m_spiBus == v) return;
    m_spiBus = v;
    qDebug() << "[Backend] SPI bus =" << v;
    emit spiBusChanged();
}

// ── Video ─────────────────────────────────────────────────────────────────

void Backend::setResolution(int v)
{
    if (m_resolution == v) return;
    m_resolution = v;
    qDebug() << "[Backend] Resolution index =" << v;
    emit resolutionChanged();
}

void Backend::setBrightness(int v)
{
    if (m_brightness == v) return;
    m_brightness = v;
    emit brightnessChanged();
}

void Backend::setVideoSource(int v)
{
    if (m_videoSource == v) return;
    m_videoSource = v;
    qDebug() << "[Backend] Video source =" << v;
    emit videoSourceChanged();
}

// ── Settings ──────────────────────────────────────────────────────────────

void Backend::setAutoStart(bool v)
{
    if (m_autoStart == v) return;
    m_autoStart = v;
    qDebug() << "[Backend] AutoStart =" << v;
    emit autoStartChanged();
}

void Backend::setDebugLogging(bool v)
{
    if (m_debugLogging == v) return;
    m_debugLogging = v;
    qDebug() << "[Backend] DebugLogging =" << v;
    emit debugLoggingChanged();
}

void Backend::setWatchdog(bool v)
{
    if (m_watchdog == v) return;
    m_watchdog = v;
    qDebug() << "[Backend] Watchdog =" << v;
    emit watchdogChanged();
}

void Backend::setLowPower(bool v)
{
    if (m_lowPower == v) return;
    m_lowPower = v;
    qDebug() << "[Backend] LowPower =" << v;
    emit lowPowerChanged();
}

// ── Connection ────────────────────────────────────────────────────────────

void Backend::scanNetwork()
{
    qDebug() << "[Backend] scanNetwork() called";
    // TODO: replace with real interface interrogation
    // e.g. QProcess("ip addr") or QNetworkInterface::allInterfaces()

    // Simulated refresh — toggle wifiUp for demo
    m_wifiUp = !m_wifiUp;
    m_wifiIp = m_wifiUp ? QStringLiteral("192.168.1.55") : QStringLiteral("—");
    emit wifiUpChanged();
    emit wifiIpChanged();
}
