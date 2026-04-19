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
    explicit FrameReader(VideoConfig &cfg, FrameCallback cb);
    ~FrameReader();

    void start();
    void stop();

    // Called when active_camera changes — restarts capture
    void restartCapture();

private:
    void loop();
    int  openDevice(const std::string &device,
                    uint32_t width, uint32_t height);
    void closeDevice(int fd);

    VideoConfig       &m_cfg;   // non-const — reads active camera
    FrameCallback      m_cb;
    std::thread        m_thread;
    std::atomic<bool>  m_running{false};
    std::atomic<bool>  m_restart{false};
    uint32_t           m_seq = 0;
};