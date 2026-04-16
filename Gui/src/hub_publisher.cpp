#include "hub_publisher.h"
#include <QDebug>
#include <chrono>

static uint64_t nowMs()
{
    using namespace std::chrono;
    return (uint64_t)duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count();
}

HubPublisher::HubPublisher(const QString &host, uint16_t port, QObject *parent)
    : QObject(parent)
    , m_ctx(1)
    , m_sock(m_ctx, zmq::socket_type::pub)
{
    try {
        std::string ep = "tcp://" + host.toStdString()
                       + ":" + std::to_string(port);
        m_sock.bind(ep);
        m_connected = true;
        qDebug() << "[HubPublisher] bound to" << QString::fromStdString(ep);
    } catch (const zmq::error_t &e) {
        qWarning() << "[HubPublisher] bind failed:" << e.what();
    }
}

HubPublisher::~HubPublisher()
{
    m_sock.close();
    m_ctx.close();
}

void HubPublisher::publish(const pfas::ScreenEvent &ev)
{
    if (!m_connected) return;
    std::string bytes;
    if (!ev.SerializeToString(&bytes)) {
        qWarning() << "[HubPublisher] serialize failed";
        return;
    }
    zmq::message_t msg(bytes.data(), bytes.size());
    try {
        m_sock.send(msg, zmq::send_flags::dontwait);
    } catch (const zmq::error_t &e) {
        qWarning() << "[HubPublisher] send failed:" << e.what();
    }
}

void HubPublisher::publishControlAction(const QString &action, const QString &value)
{
    pfas::ScreenEvent ev;
    ev.set_timestamp_ms(nowMs());
    ev.set_event_type(pfas::CONTROL_ACTION);
    ev.mutable_control_action()->set_action(action.toStdString());
    ev.mutable_control_action()->set_value(value.toStdString());
    qDebug() << "[HubPublisher] CONTROL_ACTION" << action << "=" << value;
    publish(ev);
}