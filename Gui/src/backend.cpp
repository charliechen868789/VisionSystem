#include "backend.h"
#include <QDebug>

Backend::Backend(HubPublisher &publisher, const GuiConfig &cfg, QObject *parent)
    : QObject(parent)
    , m_pub(publisher)
    , m_gpio0     (cfg.gpio0)
    , m_gpio1     (cfg.gpio1)
    , m_pwmEnable (cfg.pwm_enable)
    , m_spiBus    (cfg.spi_bus)
    , m_brightness (cfg.brightness)
    , m_resolution (cfg.resolution)
    , m_videoSource(cfg.video_source)
    , m_autoStart  (cfg.auto_start)
    , m_debugLogging(cfg.debug_logging)
    , m_watchdog   (cfg.watchdog)
    , m_lowPower   (cfg.low_power)
    , m_firmwareVersion(QString::fromStdString(cfg.firmware_version))
{
    qDebug() << "[Backend] initialised from config";
    connect(&m_pub, &HubPublisher::systemInfoReceived,
            this,   &Backend::onSystemInfoReceived);
}

static QString boolStr(bool v) { return v ? "true" : "false"; }

void Backend::setGpio0(bool v)
{
    if (m_gpio0 == v) return;
    m_gpio0 = v;
    qDebug() << "[Backend] GPIO0 =" << v;
    m_pub.publishControlAction("gpio0", boolStr(v));
    emit gpio0Changed();
}

void Backend::setGpio1(bool v)
{
    if (m_gpio1 == v) return;
    m_gpio1 = v;
    qDebug() << "[Backend] GPIO1 =" << v;
    m_pub.publishControlAction("gpio1", boolStr(v));
    emit gpio1Changed();
}

void Backend::setPwmEnable(bool v)
{
    if (m_pwmEnable == v) return;
    m_pwmEnable = v;
    qDebug() << "[Backend] PWM =" << v;
    m_pub.publishControlAction("pwm_enable", boolStr(v));
    emit pwmEnableChanged();
}

void Backend::setSpiBus(bool v)
{
    if (m_spiBus == v) return;
    m_spiBus = v;
    qDebug() << "[Backend] SPI =" << v;
    m_pub.publishControlAction("spi_bus", boolStr(v));
    emit spiBusChanged();
}

void Backend::setResolution(int v)
{
    if (m_resolution == v) return;
    m_resolution = v;
    m_pub.publishControlAction("resolution", QString::number(v));
    emit resolutionChanged();
}

void Backend::setBrightness(int v)
{
    if (m_brightness == v) return;
    m_brightness = v;
    m_pub.publishControlAction("brightness", QString::number(v));
    emit brightnessChanged();
}

void Backend::setVideoSource(int v)
{
    if (m_videoSource == v) return;
    m_videoSource = v;
    m_pub.publishControlAction("video_source", QString::number(v));
    emit videoSourceChanged();
}

void Backend::setAutoStart(bool v)
{
    if (m_autoStart == v) return;
    m_autoStart = v;
    m_pub.publishControlAction("auto_start", boolStr(v));
    emit autoStartChanged();
}

void Backend::setDebugLogging(bool v)
{
    if (m_debugLogging == v) return;
    m_debugLogging = v;
    m_pub.publishControlAction("debug_logging", boolStr(v));
    emit debugLoggingChanged();
}

void Backend::setWatchdog(bool v)
{
    if (m_watchdog == v) return;
    m_watchdog = v;
    m_pub.publishControlAction("watchdog", boolStr(v));
    emit watchdogChanged();
}

void Backend::setLowPower(bool v)
{
    if (m_lowPower == v) return;
    m_lowPower = v;
    m_pub.publishControlAction("low_power", boolStr(v));
    emit lowPowerChanged();
}

void Backend::scanNetwork()
{
    qDebug() << "[Backend] scanNetwork()";
    m_wifiUp = !m_wifiUp;
    m_wifiIp = m_wifiUp ? QStringLiteral("192.168.1.55") : QStringLiteral("—");
    emit wifiUpChanged();
    emit wifiIpChanged();
}

// ADD to existing backend.cpp — everything else unchanged

void Backend::requestSystemInfo()
{
    qDebug() << "[Backend] requestSystemInfo()";
    m_pub.publishControlAction("get_system_info", "1");
}

void Backend::onSystemInfoReceived(double cpu, double mem,
                                   double temp, QString uptime)
{
    qDebug() << "[Backend] systemInfo cpu=" << cpu
             << "mem=" << mem << "temp=" << temp;

    if (m_temperature != temp)  { m_temperature = temp;   emit temperatureChanged(); }
    if (m_cpuPercent  != cpu)   { m_cpuPercent  = cpu;    emit cpuPercentChanged();  }
    if (m_memPercent  != mem)   { m_memPercent  = mem;    emit memPercentChanged();  }
    if (m_uptime      != uptime){ m_uptime      = uptime; emit uptimeChanged();      }
}