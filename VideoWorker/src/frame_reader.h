#pragma once
#include "video_config.h"
#include "screen_event.pb.h"
#include <functional>
#include <atomic>
#include <thread>

using FrameCallback = std::function<void(const pfas::ScreenEvent &)>;

class FrameReader
{
public:
    explicit FrameReader(const VideoConfig &cfg, FrameCallback cb);
    ~FrameReader();

    void start();
    void stop();

private:
    void loop();

    const VideoConfig &m_cfg;
    FrameCallback      m_cb;
    std::thread        m_thread;
    std::atomic<bool>  m_running{false};
    uint32_t           m_seq = 0;
};