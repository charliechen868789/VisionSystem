#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct SensorEntry {
    std::string id;
    std::string type;
    std::string device;
    uint32_t    interval_ms = 2000;
};

struct SensorConfig {
    std::vector<SensorEntry> sensors;
    std::string pub_host = "127.0.0.1";
    uint16_t    pub_port = 9002;

    bool load(const std::string &path);
};