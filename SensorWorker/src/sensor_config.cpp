#include "sensor_config.h"
#include <fstream>
#include <cstdio>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

bool SensorConfig::load(const std::string &path)
{
    std::ifstream f(path);
    if (!f.is_open()) {
        fprintf(stderr, "[SensorConfig] cannot open %s\n", path.c_str());
        return false;
    }
    try {
        json j; f >> j;
        if (j.contains("sensors")) {
            for (const auto &s : j["sensors"]) {
                SensorEntry e;
                e.id          = s.value("id",          "");
                e.type        = s.value("type",        "");
                e.device      = s.value("device",      "");
                e.interval_ms = s.value("interval_ms", 2000u);
                sensors.push_back(e);
            }
        }
        if (j.contains("publisher")) {
            pub_host = j["publisher"].value("host", pub_host);
            pub_port = j["publisher"].value("port", pub_port);
        }
    } catch (const json::exception &e) {
        fprintf(stderr, "[SensorConfig] parse error: %s\n", e.what());
        return false;
    }
    return true;
}