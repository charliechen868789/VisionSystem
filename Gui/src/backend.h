#ifndef BACKEND_H
#define BACKEND_H

#include <QObject>
#include <QString>

class Backend : public QObject
{
    Q_OBJECT

    // ── Control Hardware ──────────────────────────────────────────────────
    Q_PROPERTY(bool gpio0     READ gpio0     WRITE setGpio0     NOTIFY gpio0Changed)
    Q_PROPERTY(bool gpio1     READ gpio1     WRITE setGpio1     NOTIFY gpio1Changed)
    Q_PROPERTY(bool pwmEnable READ pwmEnable WRITE setPwmEnable NOTIFY pwmEnableChanged)
    Q_PROPERTY(bool spiBus    READ spiBus    WRITE setSpiBus    NOTIFY spiBusChanged)

    // ── Video Control ─────────────────────────────────────────────────────
    Q_PROPERTY(int    resolution READ resolution WRITE setResolution NOTIFY resolutionChanged)
    Q_PROPERTY(int    brightness READ brightness WRITE setBrightness NOTIFY brightnessChanged)
    Q_PROPERTY(int    videoSource READ videoSource WRITE setVideoSource NOTIFY videoSourceChanged)

    // ── Settings ──────────────────────────────────────────────────────────
    Q_PROPERTY(bool autoStart    READ autoStart    WRITE setAutoStart    NOTIFY autoStartChanged)
    Q_PROPERTY(bool debugLogging READ debugLogging WRITE setDebugLogging NOTIFY debugLoggingChanged)
    Q_PROPERTY(bool watchdog     READ watchdog     WRITE setWatchdog     NOTIFY watchdogChanged)
    Q_PROPERTY(bool lowPower     READ lowPower     WRITE setLowPower     NOTIFY lowPowerChanged)
    Q_PROPERTY(QString firmwareVersion READ firmwareVersion CONSTANT)

    // ── Connection ────────────────────────────────────────────────────────
    Q_PROPERTY(QString ethIp     READ ethIp     NOTIFY ethIpChanged)
    Q_PROPERTY(bool    ethUp     READ ethUp     NOTIFY ethUpChanged)
    Q_PROPERTY(bool    wifiUp    READ wifiUp    NOTIFY wifiUpChanged)
    Q_PROPERTY(QString wifiIp    READ wifiIp    NOTIFY wifiIpChanged)
    Q_PROPERTY(bool    cmdPort   READ cmdPort   NOTIFY cmdPortChanged)
    Q_PROPERTY(bool    dataPort  READ dataPort  NOTIFY dataPortChanged)

public:
    explicit Backend(QObject *parent = nullptr);

    // Control Hardware
    bool gpio0()     const { return m_gpio0; }
    bool gpio1()     const { return m_gpio1; }
    bool pwmEnable() const { return m_pwmEnable; }
    bool spiBus()    const { return m_spiBus; }

    void setGpio0(bool v);
    void setGpio1(bool v);
    void setPwmEnable(bool v);
    void setSpiBus(bool v);

    // Video
    int resolution()  const { return m_resolution; }
    int brightness()  const { return m_brightness; }
    int videoSource() const { return m_videoSource; }

    void setResolution(int v);
    void setBrightness(int v);
    void setVideoSource(int v);

    // Settings
    bool autoStart()    const { return m_autoStart; }
    bool debugLogging() const { return m_debugLogging; }
    bool watchdog()     const { return m_watchdog; }
    bool lowPower()     const { return m_lowPower; }
    QString firmwareVersion() const { return QStringLiteral("v2.4.1"); }

    void setAutoStart(bool v);
    void setDebugLogging(bool v);
    void setWatchdog(bool v);
    void setLowPower(bool v);

    // Connection (read-only — polled/refreshed via scanNetwork())
    QString ethIp()   const { return m_ethIp; }
    bool    ethUp()   const { return m_ethUp; }
    bool    wifiUp()  const { return m_wifiUp; }
    QString wifiIp()  const { return m_wifiIp; }
    bool    cmdPort() const { return m_cmdPort; }
    bool    dataPort()const { return m_dataPort; }

public slots:
    void scanNetwork();   // called from Connection page "Scan" button

signals:
    void gpio0Changed();
    void gpio1Changed();
    void pwmEnableChanged();
    void spiBusChanged();

    void resolutionChanged();
    void brightnessChanged();
    void videoSourceChanged();

    void autoStartChanged();
    void debugLoggingChanged();
    void watchdogChanged();
    void lowPowerChanged();

    void ethIpChanged();
    void ethUpChanged();
    void wifiUpChanged();
    void wifiIpChanged();
    void cmdPortChanged();
    void dataPortChanged();

private:
    // Control Hardware
    bool m_gpio0     = false;
    bool m_gpio1     = true;
    bool m_pwmEnable = false;
    bool m_spiBus    = true;

    // Video
    int m_resolution  = 0;   // index into ["1920×1080","1280×720","640×480"]
    int m_brightness  = 75;
    int m_videoSource = 0;   // 0=MIPI CSI-2, 1=USB, 2=File

    // Settings
    bool m_autoStart    = true;
    bool m_debugLogging = false;
    bool m_watchdog     = true;
    bool m_lowPower     = false;

    // Connection
    QString m_ethIp   = QStringLiteral("192.168.1.42");
    bool    m_ethUp   = true;
    bool    m_wifiUp  = false;
    QString m_wifiIp  = QStringLiteral("—");
    bool    m_cmdPort = true;
    bool    m_dataPort= true;
};

#endif // BACKEND_H
