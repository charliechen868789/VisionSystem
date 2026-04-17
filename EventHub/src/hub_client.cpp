#include "hub_client.h"

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdio>
#include <zmq.hpp>

HubClient::HubClient(std::string host, uint16_t port, EventCallback cb)
    : m_host(std::move(host)), m_port(port), m_cb(std::move(cb))
    , m_reader([this](const uint8_t *data, size_t len)
      {
            fprintf(stdout, "[HubClient][FRAME] received frame %zu bytes\n", len);

            pfas::ScreenEvent ev;
            if (ev.ParseFromArray(data, (int)len)) {

                fprintf(stdout,
                    "[HubClient][PARSED] type=%d timestamp=%llu\n",
                    ev.event_type(),
                    (unsigned long long)ev.timestamp_ms());

                m_cb(ev);  // pass to dispatcher

            } else {
                fprintf(stderr,
                    "[HubClient][ERROR] proto parse failed (%zu bytes)\n", len);
            }
      })
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
    if (m_stopPipe[1] != -1) { char c='q'; ::write(m_stopPipe[1],&c,1); }
    if (m_thread.joinable()) m_thread.join();
}

void HubClient::runLoop() {
    zmq::context_t ctx(1);
    zmq::socket_t sub(ctx, zmq::socket_type::sub);
    sub.connect("tcp://127.0.0.1:9003");
    sub.set(zmq::sockopt::subscribe, ""); // Important: Subscribe to all

    while (m_running) {
        zmq::message_t msg;
        auto res = sub.recv(msg, zmq::recv_flags::none);
        if (res) {
            pfas::ScreenEvent ev;
            if (ev.ParseFromArray(msg.data(), msg.size())) {
                m_cb(ev); // This triggers EventDispatcher::onSystemInfo
            }
        }
    }
}

void HubClient::connect_()
{
    if (m_bev) { bufferevent_free(m_bev); m_bev = nullptr; }

    m_bev = bufferevent_socket_new(m_base, -1, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(m_bev, onRead, nullptr, onConnect, this);
    bufferevent_enable(m_bev, EV_READ | EV_WRITE);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(m_port);
    inet_pton(AF_INET, m_host.c_str(), &addr.sin_addr);

    bufferevent_socket_connect(m_bev, (sockaddr*)&addr, sizeof(addr));
    fprintf(stdout, "[HubClient] connecting %s:%u\n", m_host.c_str(), m_port);
}

void HubClient::scheduleRetry()
{
    if (m_bev) { bufferevent_free(m_bev); m_bev = nullptr; }
    if (m_retryEv) event_free(m_retryEv);
    m_retryEv = event_new(m_base, -1, EV_TIMEOUT, onRetry, this);
    timeval tv{5, 0};
    event_add(m_retryEv, &tv);
    fprintf(stdout, "[HubClient] retry in 5s\n");
}

// -- callbacks ----------------------------------------------------------------

void HubClient::onConnect(bufferevent*, short events, void *ctx)
{
    auto *self = static_cast<HubClient*>(ctx);

    if (events & BEV_EVENT_CONNECTED) {
        fprintf(stdout,
            "[HubClient][CONNECTED] %s:%u\n",
            self->m_host.c_str(), self->m_port);

    } else if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        fprintf(stderr, "[HubClient][DISCONNECTED]\n");
        self->scheduleRetry();
    }
}

void HubClient::onRead(bufferevent *bev, void *ctx)
{
    auto  *self  = static_cast<HubClient*>(ctx);
    evbuffer *in = bufferevent_get_input(bev);
    size_t avail = evbuffer_get_length(in);

    if (avail == 0) return;

    fprintf(stdout, "[HubClient][RX RAW] %zu bytes\n", avail);

    std::vector<uint8_t> tmp(avail);
    evbuffer_remove(in, tmp.data(), avail);

    self->m_reader.feed(tmp.data(), avail);
}

void HubClient::onRetry(evutil_socket_t, short, void *ctx)
{
    static_cast<HubClient*>(ctx)->connect_();
}

void HubClient::onStop(evutil_socket_t, short, void *ctx)
{
    event_base_loopbreak(static_cast<event_base*>(ctx));
}