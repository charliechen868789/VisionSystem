#include "frame_reader.h"
#include <cstdio>
#include <cstring>
#include <chrono>

// ─── V4L2 includes ────────────────────────────────────────────────────────────
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

static uint64_t nowMs()
{
    using namespace std::chrono;
    return (uint64_t)duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count();
}

// ─── Simple JPEG stub (replaces real capture until V4L2 is wired) ─────────────
// Returns a minimal valid 1x1 white JPEG so the pipeline is testable end-to-end
static std::vector<uint8_t> stubJpeg()
{
    // Minimal valid JPEG: 1×1 white pixel
    static const uint8_t kJpeg[] = {
        0xFF,0xD8,0xFF,0xE0,0x00,0x10,0x4A,0x46,0x49,0x46,0x00,0x01,
        0x01,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0xFF,0xDB,0x00,0x43,
        0x00,0x08,0x06,0x06,0x07,0x06,0x05,0x08,0x07,0x07,0x07,0x09,
        0x09,0x08,0x0A,0x0C,0x14,0x0D,0x0C,0x0B,0x0B,0x0C,0x19,0x12,
        0x13,0x0F,0x14,0x1D,0x1A,0x1F,0x1E,0x1D,0x1A,0x1C,0x1C,0x20,
        0x24,0x2E,0x27,0x20,0x22,0x2C,0x23,0x1C,0x1C,0x28,0x37,0x29,
        0x2C,0x30,0x31,0x34,0x34,0x34,0x1F,0x27,0x39,0x3D,0x38,0x32,
        0x3C,0x2E,0x33,0x34,0x32,0xFF,0xC0,0x00,0x0B,0x08,0x00,0x01,
        0x00,0x01,0x01,0x01,0x11,0x00,0xFF,0xC4,0x00,0x1F,0x00,0x00,
        0x01,0x05,0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
        0x09,0x0A,0x0B,0xFF,0xC4,0x00,0xB5,0x10,0x00,0x02,0x01,0x03,
        0x03,0x02,0x04,0x03,0x05,0x05,0x04,0x04,0x00,0x00,0x01,0x7D,
        0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,
        0x13,0x51,0x61,0x07,0x22,0x71,0x14,0x32,0x81,0x91,0xA1,0x08,
        0x23,0x42,0xB1,0xC1,0x15,0x52,0xD1,0xF0,0x24,0x33,0x62,0x72,
        0x82,0xFF,0xDA,0x00,0x08,0x01,0x01,0x00,0x00,0x3F,0x00,0xFB,
        0xD2,0x8A,0x28,0x03,0xFF,0xD9
    };
    return std::vector<uint8_t>(kJpeg, kJpeg + sizeof(kJpeg));
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

    // ── Try to open V4L2 device ───────────────────────────────────────────────
    int fd = open(m_cfg.device.c_str(), O_RDWR | O_NONBLOCK);
    bool useV4L2 = (fd >= 0);

    if (!useV4L2) {
        fprintf(stderr, "[FrameReader] cannot open %s — running stub\n",
                m_cfg.device.c_str());
    } else {
        // Set format
        v4l2_format fmt{};
        fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.width       = m_cfg.width;
        fmt.fmt.pix.height      = m_cfg.height;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
        fmt.fmt.pix.field       = V4L2_FIELD_NONE;
        if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
            fprintf(stderr, "[FrameReader] VIDIOC_S_FMT failed — stub\n");
            close(fd);
            useV4L2 = false;
        }
    }

    // ── mmap buffer setup ────────────────────────────────────────────────────
    struct MmapBuf { void *start; size_t length; };
    std::vector<MmapBuf> buffers;

    if (useV4L2) {
        v4l2_requestbuffers req{};
        req.count  = 4;
        req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;
        ioctl(fd, VIDIOC_REQBUFS, &req);

        buffers.resize(req.count);
        for (uint32_t i = 0; i < req.count; ++i) {
            v4l2_buffer buf{};
            buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index  = i;
            ioctl(fd, VIDIOC_QUERYBUF, &buf);
            buffers[i].length = buf.length;
            buffers[i].start  = mmap(nullptr, buf.length,
                                     PROT_READ | PROT_WRITE,
                                     MAP_SHARED, fd, buf.m.offset);
            ioctl(fd, VIDIOC_QBUF, &buf);
        }

        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ioctl(fd, VIDIOC_STREAMON, &type);
        fprintf(stdout, "[FrameReader] V4L2 streaming started\n");
    }

    const uint32_t interval_ms = 1000 / (m_cfg.fps > 0 ? m_cfg.fps : 30);

    while (m_running) {
        std::vector<uint8_t> jpegData;

        if (useV4L2) {
            // Dequeue buffer
            v4l2_buffer buf{};
            buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;

            if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(interval_ms));
                continue;
            }

            // Copy MJPEG frame
            const uint8_t *src =
                static_cast<const uint8_t*>(buffers[buf.index].start);
            jpegData.assign(src, src + buf.bytesused);

            // Requeue buffer
            ioctl(fd, VIDIOC_QBUF, &buf);
        } else {
            // Stub: emit minimal JPEG
            jpegData = stubJpeg();
            std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
        }

        // Apply flips if needed (stub — real impl uses libjpeg-turbo or OpenCV)
        // if (m_cfg.flip_horizontal) { ... }
        // if (m_cfg.flip_vertical)   { ... }

        // Build and emit ScreenEvent
        pfas::ScreenEvent ev;
        ev.set_timestamp_ms(nowMs());
        ev.set_event_type(pfas::VIDEO_FRAME);

        auto *vf = ev.mutable_video_frame();
        vf->set_width(m_cfg.width);
        vf->set_height(m_cfg.height);
        vf->set_frame_seq(++m_seq);
        vf->set_jpeg_data(jpegData.data(), jpegData.size());

        m_cb(ev);

        fprintf(stdout, "[FrameReader] frame seq=%u  %zu bytes\n",
                m_seq, jpegData.size());
    }

    // ── Cleanup V4L2 ─────────────────────────────────────────────────────────
    if (useV4L2) {
        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ioctl(fd, VIDIOC_STREAMOFF, &type);
        for (auto &b : buffers)
            munmap(b.start, b.length);
        close(fd);
    }

    fprintf(stdout, "[FrameReader] stopped\n");
}