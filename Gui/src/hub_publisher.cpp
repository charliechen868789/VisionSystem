#include "hub_publisher.h"
#include <QDebug>
#include <chrono>

static uint64_t nowMs() {
    using namespace std::chrono;
    return (uint64_t)duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count();
}

// ── HubReceiver ───────────────────────────────────────────────────────────────
HubReceiver::HubReceiver(const std::string &host, uint16_t port,
                         zmq::context_t &ctx, QObject *parent)
    : QThread(parent), m_host(host), m_port(port), m_ctx(ctx) {}

void HubReceiver::stop()
{
    m_running = false;
    wait();
}

void HubReceiver::run()
{
    zmq::socket_t sub(m_ctx, zmq::socket_type::sub);
    sub.set(zmq::sockopt::subscribe, "");
    sub.set(zmq::sockopt::rcvtimeo, 500);   // 500 ms poll so we can check m_running

    std::string ep = "tcp://" + m_host + ":" + std::to_string(m_port);
    try {
        sub.connect(ep);
        qDebug() << "[HubReceiver] connected to" << QString::fromStdString(ep);
    } catch (const zmq::error_t &e) {
        qWarning() << "[HubReceiver] connect failed:" << e.what();
        return;
    }

    while (m_running) {
        zmq::message_t msg;
        auto res = sub.recv(msg);
        if (!res) continue;   // timeout — loop and check m_running

        pfas::ScreenEvent ev;
        if (!ev.ParseFromArray(msg.data(), (int)msg.size())) continue;

        if (ev.event_type() == pfas::SYSTEM_INFO) {
            const auto &si = ev.system_info();
            emit systemInfoReceived(
                si.cpu_percent(),
                si.mem_percent(),
                si.temp_celsius(),
                QString::fromStdString(si.uptime()));
        }
    }
    sub.close();
}

// ── HubPublisher ──────────────────────────────────────────────────────────────
HubPublisher::HubPublisher(const QString &pubHost, uint16_t pubPort,
                           const QString &subHost, uint16_t subPort,
                           QObject *parent)
    : QObject(parent)
    , m_ctx(1)
    , m_sock(m_ctx, zmq::socket_type::pub)
{
    try {
        std::string ep = "tcp://" + pubHost.toStdString()
                       + ":" + std::to_string(pubPort);
        m_sock.bind(ep);
        m_connected = true;
        qDebug() << "[HubPublisher] bound to" << QString::fromStdString(ep);
    } catch (const zmq::error_t &e) {
        qWarning() << "[HubPublisher] bind failed:" << e.what();
    }

    // Start background receiver for EventHub→GUI events
    m_receiver = new HubReceiver(subHost.toStdString(), subPort, m_ctx, this);
    connect(m_receiver, &HubReceiver::systemInfoReceived,
            this,       &HubPublisher::systemInfoReceived);
    m_receiver->start();
}

HubPublisher::~HubPublisher()
{
    if (m_receiver) { m_receiver->stop(); }
    m_sock.close();
    m_ctx.close();
}

void HubPublisher::publish(const pfas::ScreenEvent &ev)
{
    if (!m_connected) return;
    std::string bytes;
    if (!ev.SerializeToString(&bytes)) return;
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