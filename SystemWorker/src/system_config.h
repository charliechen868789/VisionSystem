#pragma once
#include <string>
#include <cstdint>

struct SystemConfig {
    uint32_t    poll_interval_ms  = 5000;
    std::string pub_host          = "127.0.0.1";
    uint16_t    pub_port          = 9000;
    float       cpu_warn          = 85.0f;
    float       mem_warn          = 90.0f;
    float       temp_warn         = 75.0f;

    bool load(const std::string &path);
};