#pragma once
#include <string>
#include <cstdint>
#include <mutex>

struct VideoConfig {
    // Source
    std::string source_type     = "mipi";
    std::string device          = "/dev/video0";
    uint32_t    width           = 1920;
    uint32_t    height          = 1080;
    uint32_t    fps             = 30;

    // Image
    int         brightness      = 75;
    bool        night_mode      = false;
    bool        flip_horizontal = false;
    bool        flip_vertical   = false;

    // Output
    bool        record_to_file  = false;
    bool        rtsp_out        = false;
    bool        show_overlays   = true;
    int         jpeg_quality    = 80;

    // Publisher — VideoWorker PUB → EventHub SUB
    std::string pub_host        = "127.0.0.1";
    uint16_t    pub_port        = 9001;

    // Settings — EventHub PUSH → VideoWorker PULL
    std::string settings_host   = "127.0.0.1";
    uint16_t    settings_port   = 9010;

    // Config file path for save()
    std::string config_path;

    mutable std::mutex mtx;

    bool load(const std::string &path);
    bool save() const;
    void applyAction(const std::string &action, const std::string &value);
};