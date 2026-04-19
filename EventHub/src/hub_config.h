#pragma once
#include <string>
#include <unordered_map>
#include <cstdint>
#include <vector>

struct WorkerEntry {
    bool        enabled = false;
    std::string config;
    std::string binary;
    uint16_t    pub_port      = 0;   // ADD — worker PUB port
    uint16_t    settings_port = 0;   // ADD — EventHub PUSH settings here
};

struct HubConfig {
    std::string sub_host = "127.0.0.1";
    uint16_t    gui_sub_port         = 9000;
    uint16_t    gui_reply_port       = 9005;   // system info
    uint16_t    gui_video_reply_port = 9006;   // video frames
    uint16_t    gui_ai_reply_port    = 9007;   // AI results

    std::string cloud_endpoint = "http://localhost:8080/api/events";
    std::string cloud_api_key;
    int         cloud_retries  = 3;
    int         cloud_timeout  = 10;
    bool        cloud_enabled  = true;

    std::string log_path = "/var/log/eventhub.log";

    // Kept for action-name -> GPIO number logging only (no sysfs writes)
    std::unordered_map<std::string, uint32_t> gpio_map;

    std::unordered_map<std::string, WorkerEntry> workers;
    std::vector<std::string> workerEndpoints() const;
    bool load(const std::string &path);
    void dump() const;
};