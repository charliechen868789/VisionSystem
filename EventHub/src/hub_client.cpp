#include "hub_client.h"
#include <cstdio>
#include "../../common/zmq_compat.h"

HubClient::HubClient(std::vector<std::string> endpoints, EventCallback cb)
    : m_endpoints(std::move(endpoints))
    , m_cb(std::move(cb))
{}

HubClient::~HubClient() { stop(); }

void HubClient::start()
{
    m_running = true;
    m_thread  = std::thread([this]{ runLoop(); });
}

void HubClient::stop()
{
    m_running = false;
    if (m_thread.joinable()) m_thread.join();
}

void HubClient::runLoop()
{
    zmq::context_t ctx(1);
    zmq::socket_t  sub(ctx, zmq::socket_type::sub);
    zmq_set_subscribe(sub, "");
    zmq_set_rcvtimeo(sub, 500);  // 500ms timeout for clean shutdown

    for (const auto &ep : m_endpoints) {
        try {
            sub.connect(ep);
            fprintf(stdout, "[HubClient] subscribed to %s\n", ep.c_str());
        } catch (const zmq::error_t &e) {
            fprintf(stderr, "[HubClient] connect %s failed: %s\n",
                    ep.c_str(), e.what());
        }
    }

    while (m_running) {
        zmq::message_t msg;
        auto res = sub.recv(msg);
        if (!res) continue;   // timeout — check m_running

        pfas::ScreenEvent ev;
        if (ev.ParseFromArray(msg.data(), (int)msg.size())) {
            fprintf(stdout, "[HubClient] rx type=%d ts=%llu\n",
                    ev.event_type(),
                    (unsigned long long)ev.timestamp_ms());
            m_cb(ev);
        } else {
            fprintf(stderr, "[HubClient] proto parse failed (%zu bytes)\n",
                    msg.size());
        }
    }

    sub.close();
    ctx.close();
    fprintf(stdout, "[HubClient] stopped\n");
}