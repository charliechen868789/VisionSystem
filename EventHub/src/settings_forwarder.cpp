#include "settings_forwarder.h"
#include <cstdio>

const std::unordered_map<std::string, std::string>
SettingsForwarder::k_actionWorker = {
    { "video_source",      "video" },
    { "resolution",        "video" },
    { "frame_rate",        "video" },
    { "brightness",        "video" },
    { "night_mode",        "video" },
    { "flip_horizontal",   "video" },
    { "flip_vertical",     "video" },
    { "record_to_file",    "video" },
    { "rtsp_out",          "video" },
    { "show_overlays",     "video" },
    { "stream_enable",     "video" },
    { "switch_camera",     "video" },
    { "ai_model",          "ai"    },
    { "ai_confidence",     "ai"    },
    { "object_detection",  "ai"    },
    { "face_detection",    "ai"    },
    { "tracking_enabled",  "ai"    },
    { "pose_estimation",   "ai"    },
    { "anomaly_detection", "ai"    },
};

SettingsForwarder::SettingsForwarder(const HubConfig &cfg) : m_ctx(1)
{
    for (const auto &[name, w] : cfg.workers) {
        if (!w.enabled || w.settings_port == 0) continue;

        auto *sock = new zmq::socket_t(m_ctx, zmq::socket_type::push);
        std::string ep = "tcp://" + cfg.sub_host
                       + ":" + std::to_string(w.settings_port);
        try {
            sock->bind(ep);
            m_socks[name] = sock;
            fprintf(stdout, "[SettingsForwarder] PUSH for '%s' bound to %s\n",
                    name.c_str(), ep.c_str());
        } catch (const zmq::error_t &e) {
            fprintf(stderr, "[SettingsForwarder] bind %s failed: %s\n",
                    ep.c_str(), e.what());
            delete sock;
        }
    }
}

SettingsForwarder::~SettingsForwarder()
{
    for (auto &[name, sock] : m_socks) {
        sock->close();
        delete sock;
    }
    m_ctx.close();
}

void SettingsForwarder::forward(const pfas::ControlActionPayload &ca)
{
    auto it = k_actionWorker.find(ca.action());
    if (it == k_actionWorker.end()) return;

    auto sockIt = m_socks.find(it->second);
    if (sockIt == m_socks.end()) return;

    pfas::ScreenEvent ev;
    ev.set_event_type(pfas::CONTROL_ACTION);
    *ev.mutable_control_action() = ca;

    std::string bytes;
    if (!ev.SerializeToString(&bytes)) return;

    zmq::message_t msg(bytes.data(), bytes.size());
    try {
        sockIt->second->send(msg, zmq::send_flags::dontwait);
        fprintf(stdout, "[SettingsForwarder] → %s: %s=%s\n",
                it->second.c_str(),
                ca.action().c_str(), ca.value().c_str());
    } catch (const zmq::error_t &e) {
        fprintf(stderr, "[SettingsForwarder] send failed: %s\n", e.what());
    }
}