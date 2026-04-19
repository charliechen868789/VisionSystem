#include "frame_reader.h"
#include <cstdio>
#include <cstring>
#include <chrono>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

static uint64_t nowMs() {
    using namespace std::chrono;
    return (uint64_t)duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count();
}

FrameReader::FrameReader(VideoConfig &cfg, FrameCallback cb)
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

void FrameReader::restartCapture()
{
    m_restart = true;
}

void FrameReader::loop()
{
    while (m_running) {
        m_restart = false;

        // Read current camera config under lock
        std::string device;
        uint32_t width, height, fps;
        {
            std::lock_guard<std::mutex> lk(m_cfg.mtx);
            device = m_cfg.device;
            width  = m_cfg.width;
            height = m_cfg.height;
            fps    = m_cfg.fps;
        }

        fprintf(stdout, "[FrameReader] opening %s (%ux%u @ %u fps)\n",
                device.c_str(), width, height, fps);

        int fd = open(device.c_str(), O_RDWR | O_NONBLOCK);
        bool useV4L2 = (fd >= 0);

        struct MmapBuf { void *start; size_t length; };
        std::vector<MmapBuf> buffers;

        if (useV4L2) {
            v4l2_format fmt{};
            fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            fmt.fmt.pix.width       = width;
            fmt.fmt.pix.height      = height;
            fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
            fmt.fmt.pix.field       = V4L2_FIELD_NONE;
            if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
                fprintf(stderr, "[FrameReader] VIDIOC_S_FMT failed\n");
                close(fd); useV4L2 = false;
            }
        }

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
                                         PROT_READ|PROT_WRITE,
                                         MAP_SHARED, fd, buf.m.offset);
                ioctl(fd, VIDIOC_QBUF, &buf);
            }

            v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            ioctl(fd, VIDIOC_STREAMON, &type);
            fprintf(stdout, "[FrameReader] streaming started on %s\n",
                    device.c_str());
        } else {
            fprintf(stderr, "[FrameReader] cannot open %s\n", device.c_str());
        }

        const uint32_t interval_ms = 1000 / (fps > 0 ? fps : 30);

        // Capture loop — exits on stop or camera switch
        while (m_running && !m_restart) {
            std::vector<uint8_t> jpegData;

            if (useV4L2) {
                v4l2_buffer buf{};
                buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(interval_ms));
                    continue;
                }
                const uint8_t *src =
                    static_cast<const uint8_t*>(buffers[buf.index].start);
                jpegData.assign(src, src + buf.bytesused);
                ioctl(fd, VIDIOC_QBUF, &buf);
            } else {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(interval_ms));
                continue;
            }

            // Check streaming enabled
            {
                std::lock_guard<std::mutex> lk(m_cfg.mtx);
                // Future: check if GUI page active
            }

            pfas::ScreenEvent ev;
            ev.set_timestamp_ms(nowMs());
            ev.set_event_type(pfas::VIDEO_FRAME);

            auto *vf = ev.mutable_video_frame();
            vf->set_width(width);
            vf->set_height(height);
            vf->set_frame_seq(++m_seq);
            vf->set_jpeg_data(jpegData.data(), jpegData.size());

            // Tag which camera
            vf->set_camera_id(m_cfg.active_camera);

            // Skip frame if GUI not watching
            if (!m_cfg.streaming_enabled.load()) {
                fprintf(stdout, "[FrameReader] stream disabled — dropping frame %u\n",
                        m_seq);
                continue;
            }

            m_cb(ev);
            fprintf(stdout, "[FrameReader] frame seq=%u  %zu bytes\n",
                    m_seq, jpegData.size());
                    }

        // Cleanup
        if (useV4L2) {
            v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            ioctl(fd, VIDIOC_STREAMOFF, &type);
            for (auto &b : buffers) munmap(b.start, b.length);
            close(fd);
            fprintf(stdout, "[FrameReader] closed %s\n", device.c_str());
        }
    }

    fprintf(stdout, "[FrameReader] stopped\n");
}