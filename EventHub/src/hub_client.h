#pragma once
#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <thread>
#include <zmq.hpp>
#include "screen_event.pb.h"

class HubClient
{
public:
    using EventCallback = std::function<void(const pfas::ScreenEvent&)>;

    // Takes list of endpoints to subscribe to e.g.
    // {"tcp://127.0.0.1:9000", "tcp://127.0.0.1:9001", "tcp://127.0.0.1:9003"}
    HubClient(std::vector<std::string> endpoints, EventCallback cb);
    ~HubClient();

    void start();
    void stop();

private:
    void runLoop();

    std::vector<std::string> m_endpoints;
    EventCallback            m_cb;
    std::atomic<bool>        m_running{false};
    std::thread              m_thread;
};