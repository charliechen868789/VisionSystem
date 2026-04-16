#pragma once
#include <QObject>
#include <QString>
#include <zmq.hpp>
#include "screen_event.pb.h"

class HubPublisher : public QObject
{
    Q_OBJECT
public:
    explicit HubPublisher(const QString &host = "127.0.0.1",
                          uint16_t       port = 9000,
                          QObject       *parent = nullptr);
    ~HubPublisher();

    bool isConnected() const { return m_connected; }

    void publish(const pfas::ScreenEvent &ev);
    void publishControlAction(const QString &action, const QString &value);

private:
    zmq::context_t m_ctx;
    zmq::socket_t  m_sock;
    bool           m_connected = false;
};