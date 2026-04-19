#include "video_config.h"
#include <fstream>
#include <cstdio>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

void VideoConfig::setActiveCamera(int idx)
{
    if (idx < 0 || idx >= (int)cameras.size()) return;
    active_camera = idx;
    const auto &c = cameras[idx];
    source_type   = c.type;
    device        = c.device;
    width         = c.width;
    height        = c.height;
    fps           = c.fps;
    fprintf(stdout, "[VideoConfig] active camera -> %s (%s)\n",
            c.name.c_str(), c.device.c_str());
}

bool VideoConfig::load(const std::string &path)
{
    config_path = path;
    std::ifstream f(path);
    if (!f.is_open()) {
        fprintf(stderr, "[VideoConfig] cannot open %s — defaults\n", path.c_str());
        return false;
    }
    try {
        json j; f >> j;

        // Load camera list
        if (j.contains("cameras")) {
            for (const auto &c : j["cameras"]) {
                CameraEntry e;
                e.id     = c.value("id",     0);
                e.name   = c.value("name",   "Camera");
                e.device = c.value("device", "/dev/video0");
                e.type   = c.value("type",   "usb");
                e.width  = c.value("width",  1280u);
                e.height = c.value("height", 720u);
                e.fps    = c.value("fps",    30u);
                cameras.push_back(e);
            }
        }

        active_camera = j.value("active_camera", 0);

        // Fallback: legacy single-source config
        if (cameras.empty()) {
            CameraEntry e;
            if (j.contains("source")) {
                e.type   = j["source"].value("type",   e.type);
                e.device = j["source"].value("device", e.device);
                e.width  = j["source"].value("width",  e.width);
                e.height = j["source"].value("height", e.height);
                e.fps    = j["source"].value("fps",    e.fps);
            }
            cameras.push_back(e);
        }

        setActiveCamera(active_camera);

        if (j.contains("publisher")) {
            pub_host = j["publisher"].value("host", pub_host);
            pub_port = j["publisher"].value("port", pub_port);
        }
        if (j.contains("settings")) {
            settings_host = j["settings"].value("host", settings_host);
            settings_port = j["settings"].value("port", settings_port);
        }

        brightness      = j.value("brightness",      brightness);
        night_mode      = j.value("night_mode",      night_mode);
        flip_horizontal = j.value("flip_horizontal", flip_horizontal);
        flip_vertical   = j.value("flip_vertical",   flip_vertical);
        record_to_file  = j.value("record_to_file",  record_to_file);
        rtsp_out        = j.value("rtsp_out",        rtsp_out);
        show_overlays   = j.value("show_overlays",   show_overlays);
        jpeg_quality    = j.value("jpeg_quality",    jpeg_quality);

    } catch (const json::exception &e) {
        fprintf(stderr, "[VideoConfig] parse error: %s\n", e.what());
        return false;
    }
    return true;
}

bool VideoConfig::save() const
{
    if (config_path.empty()) return false;
    try {
        json j;
        json cams = json::array();
        for (const auto &c : cameras) {
            cams.push_back({
                {"id",     c.id},
                {"name",   c.name},
                {"device", c.device},
                {"type",   c.type},
                {"width",  c.width},
                {"height", c.height},
                {"fps",    c.fps}
            });
        }
        j["cameras"]        = cams;
        j["active_camera"]  = active_camera;
        j["publisher"]["host"]  = pub_host;
        j["publisher"]["port"]  = pub_port;
        j["settings"]["host"]   = settings_host;
        j["settings"]["port"]   = settings_port;
        j["brightness"]         = brightness;
        j["night_mode"]         = night_mode;
        j["flip_horizontal"]    = flip_horizontal;
        j["flip_vertical"]      = flip_vertical;
        j["record_to_file"]     = record_to_file;
        j["rtsp_out"]           = rtsp_out;
        j["show_overlays"]      = show_overlays;
        j["jpeg_quality"]       = jpeg_quality;

        std::ofstream f(config_path);
        f << j.dump(2);
        fprintf(stdout, "[VideoConfig] saved → %s\n", config_path.c_str());
        return true;
    } catch (const json::exception &e) {
        fprintf(stderr, "[VideoConfig] save error: %s\n", e.what());
        return false;
    }
}

void VideoConfig::applyAction(const std::string &action, const std::string &value)
{
    std::lock_guard<std::mutex> lk(mtx);

    if (action == "switch_camera") {
        setActiveCamera(std::stoi(value));
    }
    else if (action == "video_source") {
        const char* types[] = {"mipi","usb","rtsp","file"};
        int idx = std::stoi(value);
        if (idx >= 0 && idx < 4) source_type = types[idx];
    }
    else if (action == "resolution") {
        const uint32_t w[] = {3840,1920,1280,640,320};
        const uint32_t h[] = {2160,1080, 720,480,240};
        int idx = std::stoi(value);
        if (idx >= 0 && idx < 5) {
            width  = w[idx];
            height = h[idx];
            // Update active camera entry too
            if (active_camera < (int)cameras.size()) {
                cameras[active_camera].width  = w[idx];
                cameras[active_camera].height = h[idx];
            }
        }
    }
    else if (action == "frame_rate") {
        const uint32_t rates[] = {60,30,24,15,10};
        int idx = std::stoi(value);
        if (idx >= 0 && idx < 5) {
            fps = rates[idx];
            if (active_camera < (int)cameras.size())
                cameras[active_camera].fps = rates[idx];
        }
    }
    else if (action == "brightness")      brightness      = std::stoi(value);
    else if (action == "night_mode")      night_mode      = (value == "true");
    else if (action == "flip_horizontal") flip_horizontal = (value == "true");
    else if (action == "flip_vertical")   flip_vertical   = (value == "true");
    else if (action == "record_to_file")  record_to_file  = (value == "true");
    else if (action == "rtsp_out")        rtsp_out        = (value == "true");
    else if (action == "show_overlays")   show_overlays   = (value == "true");
    else if (action == "stream_enable") {
        bool enable = (value == "true");
        streaming_enabled.store(enable);
        fprintf(stdout, "[VideoConfig] streaming %s\n",
                enable ? "ENABLED" : "DISABLED");
        return;   // don't save this — it's runtime only
    }
    else if (action == "switch_camera") {
        setActiveCamera(std::stoi(value));
        // save() called inside setActiveCamera via the else branch fallthrough
    }
    else {
        fprintf(stderr, "[VideoConfig] unknown action: %s\n", action.c_str());
        return;
    }

    fprintf(stdout, "[VideoConfig] applied %s = %s\n",
            action.c_str(), value.c_str());
    save();
}