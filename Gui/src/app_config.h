#pragma once
#include <string>
#include <cstdint>

struct GuiConfig {
    // Publisher
    std::string pub_host = "127.0.0.1";
    uint16_t    pub_port = 9000;

    // Hardware defaults
    bool gpio0      = false;
    bool gpio1      = true;
    bool pwm_enable = false;
    bool spi_bus    = true;

    // Video defaults
    int  brightness   = 75;
    int  resolution   = 0;
    int  video_source = 0;

    // Settings defaults
    bool auto_start    = true;
    bool debug_logging = false;
    bool watchdog      = true;
    bool low_power     = false;

    // UI
    bool        fullscreen       = true;
    std::string theme            = "dark";
    std::string firmware_version = "v2.4.1";

    bool load(const std::string &path);
    void dump() const;
};