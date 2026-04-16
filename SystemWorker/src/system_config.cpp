#include "system_config.h"
#include <fstream>
#include <cstdio>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

bool SystemConfig::load(const std::string &path)
{
    std::ifstream f(path);
    if (!f.is_open()) {
        fprintf(stderr, "[SystemConfig] cannot open %s\n", path.c_str());
        return false;
    }
    try {
        json j; f >> j;
        poll_interval_ms = j.value("poll_interval_ms", poll_interval_ms);
        if (j.contains("publisher")) {
            pub_host = j["publisher"].value("host", pub_host);
            pub_port = j["publisher"].value("port", pub_port);
        }
        if (j.contains("thresholds")) {
            cpu_warn  = j["thresholds"].value("cpu_warn_percent",  cpu_warn);
            mem_warn  = j["thresholds"].value("mem_warn_percent",  mem_warn);
            temp_warn = j["thresholds"].value("temp_warn_celsius", temp_warn);
        }
    } catch (const json::exception &e) {
        fprintf(stderr, "[SystemConfig] parse error: %s\n", e.what());
        return false;
    }
    return true;
}