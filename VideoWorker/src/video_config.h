#pragma once
#include <string>
#include <cstdint>

struct VideoConfig {
    std::string source_type   = "mipi";
    std::string device        = "/dev/video0";
    uint32_t    width         = 1920;
    uint32_t    height        = 1080;
    uint32_t    fps           = 30;
    std::string pub_host      = "127.0.0.1";
    uint16_t    pub_port      = 9001;
    int         jpeg_quality  = 80;

    bool load(const std::string &path);
};