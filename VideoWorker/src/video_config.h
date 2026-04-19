#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <mutex>
#include <atomic>

struct CameraEntry {
    int         id     = 0;
    std::string name   = "Camera";
    std::string device = "/dev/video0";
    std::string type   = "usb";
    uint32_t    width  = 1280;
    uint32_t    height = 720;
    uint32_t    fps    = 30;
};

struct VideoConfig {
    std::vector<CameraEntry> cameras;
    int         active_camera   = 0;

    // Active camera shortcuts (updated when active_camera changes)
    std::string source_type     = "usb";
    std::string device          = "/dev/video0";
    uint32_t    width           = 1280;
    uint32_t    height          = 720;
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

    // Publisher
    std::string pub_host        = "127.0.0.1";
    uint16_t    pub_port        = 9001;

    // Settings
    std::string settings_host   = "127.0.0.1";
    uint16_t    settings_port   = 9010;

    // Runtime flags
    std::atomic<bool> streaming_enabled{false};

    std::string config_path;
    mutable std::mutex mtx;

    bool load(const std::string &path);
    bool save() const;
    void applyAction(const std::string &action, const std::string &value);
    void setActiveCamera(int id);   // updates device/width/height/fps shortcuts
};