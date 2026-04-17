#pragma once
#include <QObject>
#include <QString>
#include <QThread>
#include <zmq.hpp>
#include "screen_event.pb.h"

// Receives inbound ScreenEvents from EventHub on a background thread
class HubReceiver : public QThread
{
    Q_OBJECT
public:
    explicit HubReceiver(const std::string &host, uint16_t port,
                         zmq::context_t &ctx, QObject *parent = nullptr);
    void stop();

signals:
    void systemInfoReceived(double cpu, double mem, double temp, QString uptime);

protected:
    void run() override;

private:
    std::string    m_host;
    uint16_t       m_port;
    zmq::context_t &m_ctx;
    bool           m_running = true;
};

class HubPublisher : public QObject
{
    Q_OBJECT
public:
    explicit HubPublisher(const QString &pubHost = "127.0.0.1",
                          uint16_t       pubPort = 9000,
                          const QString &subHost = "127.0.0.1",
                          uint16_t       subPort = 9005,
                          QObject       *parent  = nullptr);
    ~HubPublisher();

    bool isConnected() const { return m_connected; }

    void publish(const pfas::ScreenEvent &ev);
    void publishControlAction(const QString &action, const QString &value);

signals:
    void systemInfoReceived(double cpu, double mem, double temp, QString uptime);

private:
    zmq::context_t m_ctx;
    zmq::socket_t  m_sock;
    bool           m_connected = false;
    HubReceiver   *m_receiver  = nullptr;
};