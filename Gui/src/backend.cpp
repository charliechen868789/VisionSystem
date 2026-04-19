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
    connect(&m_pub, &HubPublisher::videoFrameReceived,
            this,   &Backend::onVideoFrameReceived);
    connect(&m_pub, &HubPublisher::aiResultReceived,
            this,   &Backend::onAiResultReceived);
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

void Backend::setAiModel(int v)
{
    if (m_aiModel == v) return;
    m_aiModel = v;

    m_pub.publishControlAction("ai_model", QString::number(v));

    emit aiModelChanged();
}

void Backend::setAiConfidence(double v)
{
    if (m_aiConfidence == v) return;
    m_aiConfidence = v;

    m_pub.publishControlAction("ai_confidence", QString::number(v));

    emit aiConfidenceChanged();
}

void Backend::setObjectDetection(bool v)
{
    if (m_objectDetection == v) return;
    m_objectDetection = v;
    m_pub.publishControlAction("object_detection", boolStr(v));
    emit objectDetectionChanged();
}

void Backend::setFaceDetection(bool v)
{
    if (m_faceDetection == v) return;
    m_faceDetection = v;
    m_pub.publishControlAction("face_detection", boolStr(v));
    emit faceDetectionChanged();
}

void Backend::setTrackingEnabled(bool v)
{
    if (m_trackingEnabled == v) return;
    m_trackingEnabled = v;
    m_pub.publishControlAction("tracking_enabled", boolStr(v));
    emit trackingEnabledChanged();
}

void Backend::setPoseEstimation(bool v)
{
    if (m_poseEstimation == v) return;
    m_poseEstimation = v;
    m_pub.publishControlAction("pose_estimation", boolStr(v));
    emit poseEstimationChanged();
}

void Backend::setAnomalyDetection(bool v)
{
    if (m_anomalyDetection == v) return;
    m_anomalyDetection = v;
    m_pub.publishControlAction("anomaly_detection", boolStr(v));
    emit anomalyDetectionChanged();
}

void Backend::setFrameRate(int v)
{
    if (m_frameRate == v) return;
    m_frameRate = v;
    m_pub.publishControlAction("frame_rate", QString::number(v));
    emit frameRateChanged();
}

void Backend::setNightMode(bool v)
{
    if (m_nightMode == v) return;
    m_nightMode = v;
    m_pub.publishControlAction("night_mode", boolStr(v));
    emit nightModeChanged();
}

void Backend::setFlipHorizontal(bool v)
{
    if (m_flipHorizontal == v) return;
    m_flipHorizontal = v;
    m_pub.publishControlAction("flip_horizontal", boolStr(v));
    emit flipHorizontalChanged();
}

void Backend::setFlipVertical(bool v)
{
    if (m_flipVertical == v) return;
    m_flipVertical = v;
    m_pub.publishControlAction("flip_vertical", boolStr(v));
    emit flipVerticalChanged();
}

void Backend::setRecordToFile(bool v)
{
    if (m_recordToFile == v) return;
    m_recordToFile = v;
    m_pub.publishControlAction("record_to_file", boolStr(v));
    emit recordToFileChanged();
}

void Backend::setRtspOut(bool v)
{
    if (m_rtspOut == v) return;
    m_rtspOut = v;
    m_pub.publishControlAction("rtsp_out", boolStr(v));
    emit rtspOutChanged();
}

void Backend::setShowOverlays(bool v)
{
    if (m_showOverlays == v) return;
    m_showOverlays = v;
    m_pub.publishControlAction("show_overlays", boolStr(v));
    emit showOverlaysChanged();
}


void Backend::onVideoFrameReceived(uint32_t w, uint32_t h,
                                   uint32_t seq, QByteArray jpeg)
{
    qDebug() << "[Backend] videoFrame seq=" << seq
             << "videoItem=" << (m_videoItem ? "OK" : "NULL");  // ADD
    m_videoFrame = jpeg;
    emit videoFrameChanged();
    if (m_videoItem)
        m_videoItem->setFrame(jpeg, seq);
}

void Backend::onAiResultReceived(QString model, QString label,
                                  double conf, uint32_t frameSeq)
{
    Q_UNUSED(model)
    m_aiLabel      = label;
    m_aiConfidence = conf;
    m_aiFrameSeq   = frameSeq;
    emit aiLabelChanged();
    emit aiConfidenceChanged();
    emit aiFrameSeqChanged();

    // Feed AI result into VideoItem overlay
    if (m_videoItem)
        m_videoItem->setAiResult(label, conf, frameSeq);
}

void Backend::registerVideoItem(QObject *item)
{
    m_videoItem = qobject_cast<VideoItem*>(item);
    if (m_videoItem)
        qDebug() << "[Backend] VideoItem registered";
    else
        qWarning() << "[Backend] registerVideoItem: cast failed";
}