#pragma once
#include <event2/event.h>
#include <event2/util.h>
#include <string>
#include <functional>
#include <atomic>
#include <thread>
#include "frame_reader.h"
#include "screen_event.pb.h"

struct event_base;
struct bufferevent;
struct event;

class HubClient
{
public:
    using EventCallback = std::function<void(const pfas::ScreenEvent&)>;

    HubClient(std::string host, uint16_t port, EventCallback cb);
    ~HubClient();

    void start();   // launches background thread
    void stop();

private:
    // libevent callbacks
    static void onConnect(bufferevent*, short events, void *ctx);
    static void onRead   (bufferevent*, void *ctx);
    static void onStop   (evutil_socket_t, short, void *ctx);
    static void onRetry  (evutil_socket_t, short, void *ctx);

    void connect_();
    void scheduleRetry();
    void runLoop();

    std::string    m_host;
    uint16_t       m_port;
    EventCallback  m_cb;

    event_base    *m_base      = nullptr;
    bufferevent   *m_bev       = nullptr;
    event         *m_retryEv   = nullptr;
    int            m_stopPipe[2] = {-1,-1};

    std::thread    m_thread;
    FrameReader    m_reader;

    std::atomic<bool> m_running{false};
};