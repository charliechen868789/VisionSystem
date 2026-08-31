#ifndef BACKEND_H
#define BACKEND_H
#include <QObject>
#include <QString>
#include <QByteArray>
#include "hub_publisher.h"
#include "app_config.h"
#include "video_item.h"

class Backend : public QObject
{
    Q_OBJECT

    // Hardware
    Q_PROPERTY(bool gpio0      READ gpio0      WRITE setGpio0      NOTIFY gpio0Changed)
    Q_PROPERTY(bool gpio1      READ gpio1      WRITE setGpio1      NOTIFY gpio1Changed)
    Q_PROPERTY(bool pwmEnable  READ pwmEnable  WRITE setPwmEnable  NOTIFY pwmEnableChanged)
    Q_PROPERTY(bool spiBus     READ spiBus     WRITE setSpiBus     NOTIFY spiBusChanged)

    // Video
    Q_PROPERTY(int  resolution   READ resolution   WRITE setResolution   NOTIFY resolutionChanged)
    Q_PROPERTY(int  brightness   READ brightness   WRITE setBrightness   NOTIFY brightnessChanged)
    Q_PROPERTY(int  videoSource  READ videoSource  WRITE setVideoSource  NOTIFY videoSourceChanged)
    Q_PROPERTY(int  frameRate    READ frameRate    WRITE setFrameRate    NOTIFY frameRateChanged)
    Q_PROPERTY(bool nightMode    READ nightMode    WRITE setNightMode    NOTIFY nightModeChanged)
    Q_PROPERTY(bool flipHorizontal READ flipHorizontal WRITE setFlipHorizontal NOTIFY flipHorizontalChanged)
    Q_PROPERTY(bool flipVertical   READ flipVertical   WRITE setFlipVertical   NOTIFY flipVerticalChanged)
    Q_PROPERTY(bool recordToFile   READ recordToFile   WRITE setRecordToFile   NOTIFY recordToFileChanged)
    Q_PROPERTY(bool rtspOut        READ rtspOut        WRITE setRtspOut        NOTIFY rtspOutChanged)
    Q_PROPERTY(bool showOverlays   READ showOverlays   WRITE setShowOverlays   NOTIFY showOverlaysChanged)

    // Settings
    Q_PROPERTY(bool autoStart    READ autoStart    WRITE setAutoStart    NOTIFY autoStartChanged)
    Q_PROPERTY(bool debugLogging READ debugLogging WRITE setDebugLogging NOTIFY debugLoggingChanged)
    Q_PROPERTY(bool watchdog     READ watchdog     WRITE setWatchdog     NOTIFY watchdogChanged)
    Q_PROPERTY(bool lowPower     READ lowPower     WRITE setLowPower     NOTIFY lowPowerChanged)
    Q_PROPERTY(QString firmwareVersion READ firmwareVersion CONSTANT)

    // Connection
    Q_PROPERTY(QString ethIp    READ ethIp    NOTIFY ethIpChanged)
    Q_PROPERTY(bool    ethUp    READ ethUp    NOTIFY ethUpChanged)
    Q_PROPERTY(bool    wifiUp     READ wifiUp     NOTIFY wifiUpChanged)
    Q_PROPERTY(QString wifiIp     READ wifiIp     NOTIFY wifiIpChanged)
    Q_PROPERTY(double  wifiSignal READ wifiSignal NOTIFY wifiSignalChanged)
    Q_PROPERTY(bool    cmdPort  READ cmdPort  NOTIFY cmdPortChanged)
    Q_PROPERTY(bool    dataPort READ dataPort NOTIFY dataPortChanged)

    // System info
    Q_PROPERTY(double  temperature READ temperature NOTIFY temperatureChanged)
    Q_PROPERTY(double  cpuPercent  READ cpuPercent  NOTIFY cpuPercentChanged)
    Q_PROPERTY(double  memPercent  READ memPercent  NOTIFY memPercentChanged)
    Q_PROPERTY(double  gpuPercent  READ gpuPercent  NOTIFY gpuPercentChanged)
    Q_PROPERTY(QString uptime      READ uptime      NOTIFY uptimeChanged)

    // AI
    Q_PROPERTY(int    aiModel          READ aiModel          WRITE setAiModel          NOTIFY aiModelChanged)
    Q_PROPERTY(double aiConfidence     READ aiConfidence     WRITE setAiConfidence     NOTIFY aiConfidenceChanged)
    Q_PROPERTY(bool   objectDetection  READ objectDetection  WRITE setObjectDetection  NOTIFY objectDetectionChanged)
    Q_PROPERTY(bool   faceDetection    READ faceDetection    WRITE setFaceDetection    NOTIFY faceDetectionChanged)
    Q_PROPERTY(bool   trackingEnabled  READ trackingEnabled  WRITE setTrackingEnabled  NOTIFY trackingEnabledChanged)
    Q_PROPERTY(bool   poseEstimation   READ poseEstimation   WRITE setPoseEstimation   NOTIFY poseEstimationChanged)
    Q_PROPERTY(bool   anomalyDetection READ anomalyDetection WRITE setAnomalyDetection NOTIFY anomalyDetectionChanged)

    // Video stream
    Q_PROPERTY(QByteArray videoFrame READ videoFrame NOTIFY videoFrameChanged)
    Q_PROPERTY(QString    aiLabel    READ aiLabel    NOTIFY aiLabelChanged)
    Q_PROPERTY(uint       aiFrameSeq READ aiFrameSeq NOTIFY aiFrameSeqChanged)
    Q_PROPERTY(QStringList cameraList   READ cameraList   NOTIFY cameraListChanged)
    Q_PROPERTY(int         activeCamera READ activeCamera NOTIFY activeCameraChanged)
public:
    explicit Backend(HubPublisher &publisher,
                     const GuiConfig &cfg,
                     QObject *parent = nullptr);

    // Hardware
    bool    gpio0()        const { return m_gpio0; }
    bool    gpio1()        const { return m_gpio1; }
    bool    pwmEnable()    const { return m_pwmEnable; }
    bool    spiBus()       const { return m_spiBus; }

    // Video
    int     resolution()   const { return m_resolution; }
    int     brightness()   const { return m_brightness; }
    int     videoSource()  const { return m_videoSource; }
    int     frameRate()    const { return m_frameRate; }
    bool    nightMode()    const { return m_nightMode; }
    bool    flipHorizontal()const{ return m_flipHorizontal; }
    bool    flipVertical() const { return m_flipVertical; }
    bool    recordToFile() const { return m_recordToFile; }
    bool    rtspOut()      const { return m_rtspOut; }
    bool    showOverlays() const { return m_showOverlays; }

    // Settings
    bool    autoStart()    const { return m_autoStart; }
    bool    debugLogging() const { return m_debugLogging; }
    bool    watchdog()     const { return m_watchdog; }
    bool    lowPower()     const { return m_lowPower; }
    QString firmwareVersion() const { return m_firmwareVersion; }

    // Connection
    QString ethIp()    const { return m_ethIp; }
    bool    ethUp()    const { return m_ethUp; }
    bool    wifiUp()     const { return m_wifiUp; }
    QString wifiIp()     const { return m_wifiIp; }
    double  wifiSignal() const { return m_wifiSignal; }
    bool    cmdPort()  const { return m_cmdPort; }
    bool    dataPort() const { return m_dataPort; }

    // System info
    double  temperature() const { return m_temperature; }
    double  cpuPercent()  const { return m_cpuPercent; }
    double  memPercent()  const { return m_memPercent; }
    double  gpuPercent()  const { return m_gpuPercent; }
    QString uptime()      const { return m_uptime; }

    // AI
    int    aiModel()          const { return m_aiModel; }
    double aiConfidence()     const { return m_aiConfidence; }
    bool   objectDetection()  const { return m_objectDetection; }
    bool   faceDetection()    const { return m_faceDetection; }
    bool   trackingEnabled()  const { return m_trackingEnabled; }
    bool   poseEstimation()   const { return m_poseEstimation; }
    bool   anomalyDetection() const { return m_anomalyDetection; }

    // Video stream
    QByteArray videoFrame()  const { return m_videoFrame; }
    QString    aiLabel()     const { return m_aiLabel; }
    uint       aiFrameSeq()  const { return m_aiFrameSeq; }
    QStringList cameraList()   const { return m_cameraList; }
    int         activeCamera() const { return m_activeCamera; }

    // Setters
    void setGpio0(bool v);       void setGpio1(bool v);
    void setPwmEnable(bool v);   void setSpiBus(bool v);
    void setResolution(int v);   void setBrightness(int v);
    void setVideoSource(int v);  void setFrameRate(int v);
    void setNightMode(bool v);   void setFlipHorizontal(bool v);
    void setFlipVertical(bool v);void setRecordToFile(bool v);
    void setRtspOut(bool v);     void setShowOverlays(bool v);
    void setAutoStart(bool v);   void setDebugLogging(bool v);
    void setWatchdog(bool v);    void setLowPower(bool v);
    void setAiModel(int v);      void setAiConfidence(double v);
    void setObjectDetection(bool v);  void setFaceDetection(bool v);
    void setTrackingEnabled(bool v);  void setPoseEstimation(bool v);
    void setAnomalyDetection(bool v);

public slots:
    void scanNetwork();
    void requestSystemInfo();
    void registerVideoItem(QObject *item);   // called from QML
    void setStreamEnable(bool enabled);    // ADD
    void switchCamera(int idx);            // ADD
private slots:
    void onSystemInfoReceived(double cpu, double mem,
                              double temp, QString uptime, double gpu,
                              double wifiSignal, bool wifiConnected);
    void onVideoFrameReceived(uint32_t w, uint32_t h,
                              uint32_t seq, QByteArray jpeg);
    void onAiResultReceived  (QString model, QString label,
                              double conf, uint32_t frameSeq);

signals:
    void gpio0Changed();       void gpio1Changed();
    void pwmEnableChanged();   void spiBusChanged();
    void resolutionChanged();  void brightnessChanged();
    void videoSourceChanged(); void frameRateChanged();
    void nightModeChanged();   void flipHorizontalChanged();
    void flipVerticalChanged();void recordToFileChanged();
    void rtspOutChanged();     void showOverlaysChanged();
    void autoStartChanged();   void debugLoggingChanged();
    void watchdogChanged();    void lowPowerChanged();
    void ethIpChanged();       void ethUpChanged();
    void wifiUpChanged();      void wifiIpChanged();
    void cmdPortChanged();     void dataPortChanged();
    void temperatureChanged(); void cpuPercentChanged();
    void memPercentChanged();  void uptimeChanged();
    void gpuPercentChanged();  void wifiSignalChanged();
    void aiModelChanged();     void aiConfidenceChanged();
    void objectDetectionChanged(); void faceDetectionChanged();
    void trackingEnabledChanged(); void poseEstimationChanged();
    void anomalyDetectionChanged();
    void videoFrameChanged();  void aiLabelChanged();
    void aiFrameSeqChanged();
    void cameraListChanged();
    void activeCameraChanged();

private:
    HubPublisher &m_pub;
    VideoItem    *m_videoItem = nullptr;

    // Hardware
    bool    m_gpio0, m_gpio1, m_pwmEnable, m_spiBus;

    // Video
    int     m_resolution, m_brightness, m_videoSource;
    int     m_frameRate       = 1;
    bool    m_nightMode       = false;
    bool    m_flipHorizontal  = false;
    bool    m_flipVertical    = false;
    bool    m_recordToFile    = false;
    bool    m_rtspOut         = false;
    bool    m_showOverlays    = true;

    // Settings
    bool    m_autoStart, m_debugLogging, m_watchdog, m_lowPower;
    QString m_firmwareVersion;

    // Connection
    QString m_ethIp    = "192.168.1.42";
    bool    m_ethUp    = true;
    bool    m_wifiUp   = false;
    QString m_wifiIp   = "—";
    bool    m_cmdPort  = true;
    bool    m_dataPort = true;

    // System info
    double  m_temperature = 0.0;
    double  m_cpuPercent  = 0.0;
    double  m_memPercent  = 0.0;
    double  m_gpuPercent  = 0.0;
    double  m_wifiSignal  = 0.0;
    QString m_uptime      = "—";

    // AI
    int    m_aiModel          = 0;
    double m_aiConfidence     = 0.6;
    bool   m_objectDetection  = false;
    bool   m_faceDetection    = false;
    bool   m_trackingEnabled  = false;
    bool   m_poseEstimation   = false;
    bool   m_anomalyDetection = false;

    // Video stream
    QByteArray m_videoFrame;
    QString    m_aiLabel    = "—";
    uint       m_aiFrameSeq = 0;
    QStringList m_cameraList;
    int         m_activeCamera = 0;
};
#endif