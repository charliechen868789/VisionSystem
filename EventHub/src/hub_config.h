#pragma once
#include <string>
#include <unordered_map>
#include <cstdint>

struct WorkerEntry {
    bool        enabled = false;
    std::string config;
    std::string binary;
};

struct HubConfig {
    std::string sub_host = "127.0.0.1";
    uint16_t    sub_port = 9000;

    std::string cloud_endpoint = "http://localhost:8080/api/events";
    std::string cloud_api_key;
    int         cloud_retries  = 3;
    int         cloud_timeout  = 10;
    bool        cloud_enabled  = true;

    std::string log_path = "/var/log/eventhub.log";

    // Kept for action-name -> GPIO number logging only (no sysfs writes)
    std::unordered_map<std::string, uint32_t> gpio_map;

    std::unordered_map<std::string, WorkerEntry> workers;

    bool load(const std::string &path);
    void dump() const;
};