#include "video_config.h"
#include <fstream>
#include <cstdio>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

bool VideoConfig::load(const std::string &path)
{
    config_path = path;
    std::ifstream f(path);
    if (!f.is_open()) {
        fprintf(stderr, "[VideoConfig] cannot open %s — using defaults\n",
                path.c_str());
        return false;
    }
    try {
        json j; f >> j;

        if (j.contains("source")) {
            source_type = j["source"].value("type",   source_type);
            device      = j["source"].value("device", device);
            width       = j["source"].value("width",  width);
            height      = j["source"].value("height", height);
            fps         = j["source"].value("fps",    fps);
        }
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
    fprintf(stdout,
            "[VideoConfig] loaded\n"
            "  source   : %s  %s  %ux%u @ %u fps\n"
            "  pub      : tcp://%s:%u\n"
            "  settings : tcp://%s:%u\n",
            source_type.c_str(), device.c_str(), width, height, fps,
            pub_host.c_str(), pub_port,
            settings_host.c_str(), settings_port);
    return true;
}

bool VideoConfig::save() const
{
    if (config_path.empty()) return false;
    try {
        json j;
        j["source"]["type"]        = source_type;
        j["source"]["device"]      = device;
        j["source"]["width"]       = width;
        j["source"]["height"]      = height;
        j["source"]["fps"]         = fps;
        j["publisher"]["host"]     = pub_host;
        j["publisher"]["port"]     = pub_port;
        j["settings"]["host"]      = settings_host;
        j["settings"]["port"]      = settings_port;
        j["brightness"]            = brightness;
        j["night_mode"]            = night_mode;
        j["flip_horizontal"]       = flip_horizontal;
        j["flip_vertical"]         = flip_vertical;
        j["record_to_file"]        = record_to_file;
        j["rtsp_out"]              = rtsp_out;
        j["show_overlays"]         = show_overlays;
        j["jpeg_quality"]          = jpeg_quality;

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

    if      (action == "video_source") {
        // 0=MIPI 1=USB 2=RTSP 3=File
        const char* types[] = {"mipi","usb","rtsp","file"};
        int idx = std::stoi(value);
        if (idx >= 0 && idx < 4) source_type = types[idx];
    }
    else if (action == "resolution") {
        const uint32_t w[] = {3840,1920,1280,640,320};
        const uint32_t h[] = {2160,1080, 720,480,240};
        int idx = std::stoi(value);
        if (idx >= 0 && idx < 5) { width = w[idx]; height = h[idx]; }
    }
    else if (action == "frame_rate") {
        const uint32_t rates[] = {60,30,24,15,10};
        int idx = std::stoi(value);
        if (idx >= 0 && idx < 5) fps = rates[idx];
    }
    else if (action == "brightness")      brightness      = std::stoi(value);
    else if (action == "night_mode")      night_mode      = (value == "true");
    else if (action == "flip_horizontal") flip_horizontal = (value == "true");
    else if (action == "flip_vertical")   flip_vertical   = (value == "true");
    else if (action == "record_to_file")  record_to_file  = (value == "true");
    else if (action == "rtsp_out")        rtsp_out        = (value == "true");
    else if (action == "show_overlays")   show_overlays   = (value == "true");
    else {
        fprintf(stderr, "[VideoConfig] unknown action: %s\n", action.c_str());
        return;
    }

    fprintf(stdout, "[VideoConfig] applied %s = %s\n",
            action.c_str(), value.c_str());
    save();
}