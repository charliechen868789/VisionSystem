#include "video_config.h"
#include <fstream>
#include <cstdio>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

bool VideoConfig::load(const std::string &path)
{
    std::ifstream f(path);
    if (!f.is_open()) {
        fprintf(stderr, "[VideoConfig] cannot open %s — using defaults\n", path.c_str());
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
        jpeg_quality = j.value("jpeg_quality", jpeg_quality);
    } catch (const json::exception &e) {
        fprintf(stderr, "[VideoConfig] parse error: %s\n", e.what());
        return false;
    }
    return true;
}