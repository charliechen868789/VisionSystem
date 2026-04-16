#include "frame_reader.h"
#include <cstdio>
#include <cstring>
#include <chrono>
#include <ctime>

static uint64_t nowMs() {
    using namespace std::chrono;
    return (uint64_t)duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count();
}

FrameReader::FrameReader(const VideoConfig &cfg, FrameCallback cb)
    : m_cfg(cfg), m_cb(std::move(cb)) {}

FrameReader::~FrameReader() { stop(); }

void FrameReader::start()
{
    m_running = true;
    m_thread  = std::thread(&FrameReader::loop, this);
}

void FrameReader::stop()
{
    m_running = false;
    if (m_thread.joinable()) m_thread.join();
}

void FrameReader::loop()
{
    fprintf(stdout, "[FrameReader] opening %s (%ux%u @ %u fps)\n",
            m_cfg.device.c_str(), m_cfg.width, m_cfg.height, m_cfg.fps);

    const uint32_t interval_ms = 1000 / (m_cfg.fps > 0 ? m_cfg.fps : 30);

    while (m_running) {
        // --- Capture frame from V4L2 device (stub: replace with real V4L2 ioctl) ---
        // In production: open() → VIDIOC_REQBUFS → mmap → VIDIOC_STREAMON → VIDIOC_DQBUF
        // Here we emit a stub event so the rest of the pipeline works
        pfas::ScreenEvent ev;
        ev.set_timestamp_ms(nowMs());
        ev.set_event_type(pfas::VIDEO_FRAME);

        auto *vf = ev.mutable_video_frame();
        vf->set_width(m_cfg.width);
        vf->set_height(m_cfg.height);
        vf->set_frame_seq(++m_seq);
        // vf->set_jpeg_data(jpeg_bytes);  ← set from real capture buffer

        m_cb(ev);

        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
    }
    fprintf(stdout, "[FrameReader] stopped\n");
}