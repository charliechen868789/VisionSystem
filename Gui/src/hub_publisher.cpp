#include "hub_publisher.h"
#include <QDebug>
#include <chrono>
#include "../../common/zmq_compat.h"

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
    zmq_set_subscribe(sub, "");
    zmq_set_rcvtimeo(sub, 500);

    // Small delay — lets EventHub finish binding before we connect
    QThread::msleep(500);

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
        if (!res) continue;

        pfas::ScreenEvent ev;
        if (!ev.ParseFromArray(msg.data(), (int)msg.size())) {
            qWarning() << "[HubReceiver] parse failed size=" << msg.size();
            continue;
        }

        qDebug() << "[HubReceiver] rx type=" << ev.event_type()
                 << "port=" << m_port;   // debug — remove later

        switch (ev.event_type()) {

        case pfas::SYSTEM_INFO: {
            const auto &si = ev.system_info();
            emit systemInfoReceived(
                si.cpu_percent(), si.mem_percent(),
                si.temp_celsius(),
                QString::fromStdString(si.uptime()),
                si.gpu_percent(),
                si.wifi_signal_percent(),
                si.wifi_connected());
            break;
        }

        case pfas::VIDEO_FRAME: {
            const auto &vf = ev.video_frame();
            qDebug() << "[HubReceiver] VIDEO_FRAME seq=" << vf.frame_seq()
                     << "size=" << vf.jpeg_data().size();
            QByteArray jpeg(vf.jpeg_data().data(),
                            (int)vf.jpeg_data().size());
            emit videoFrameReceived(vf.width(), vf.height(),
                                    vf.frame_seq(), jpeg);
            break;
        }

    case pfas::AI_RESULT: {
        const auto &ai = ev.ai_result();

        QList<GuiDetection> dets;
        for (const auto &d : ai.detections()) {
            GuiDetection gd;
            gd.label      = QString::fromStdString(d.label());
            gd.confidence = d.confidence();
            gd.x          = d.x();
            gd.y          = d.y();
            gd.w          = d.width();
            gd.h          = d.height();
            dets.append(gd);
        }

        qDebug() << "[HubReceiver] AI_RESULT label=" << QString::fromStdString(ai.label())
                << "detections=" << dets.size();

        emit aiResultReceived(
            QString::fromStdString(ai.model()),
            QString::fromStdString(ai.label()),
            ai.confidence(),
            ai.frame_seq(),
            dets);
        break;
    }

        default:
            qDebug() << "[HubReceiver] unhandled type=" << ev.event_type();
            break;
        }
    }

    sub.close();
}

// ── HubPublisher ──────────────────────────────────────────────────────────────
HubPublisher::HubPublisher(const QString &pubHost, uint16_t pubPort,
                           const QString &sysHost, uint16_t sysPort,
                           const QString &vidHost, uint16_t vidPort,
                           const QString &aiHost,  uint16_t aiPort,
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

    auto makeReceiver = [&](const QString &host, uint16_t port) {
        auto *r = new HubReceiver(host.toStdString(), port, m_ctx, this);
        return r;
    };

    // System info receiver :9005
    m_sysReceiver = makeReceiver(sysHost, sysPort);
    connect(m_sysReceiver, &HubReceiver::systemInfoReceived,
            this,          &HubPublisher::systemInfoReceived);
    m_sysReceiver->start();

    // Video frame receiver :9006
    m_vidReceiver = makeReceiver(vidHost, vidPort);
    connect(m_vidReceiver, &HubReceiver::videoFrameReceived,
            this,          &HubPublisher::videoFrameReceived);
    m_vidReceiver->start();

    // AI result receiver :9007
    m_aiReceiver = makeReceiver(aiHost, aiPort);
    connect(m_aiReceiver, &HubReceiver::aiResultReceived,
            this,         &HubPublisher::aiResultReceived);
    m_aiReceiver->start();
}
HubPublisher::~HubPublisher()
{
    // Stop all receivers before closing context
    for (auto *r : {m_sysReceiver, m_vidReceiver, m_aiReceiver}) {
        if (r) { r->stop(); delete r; }
    }
    m_sysReceiver = m_vidReceiver = m_aiReceiver = nullptr;
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