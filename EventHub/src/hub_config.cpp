#include "hub_config.h"
#include <fstream>
#include <sstream>
#include <cstdio>

// Minimal JSON parser using nlohmann/json (header-only, no extra dep on Yocto)
// Include path: third_party/nlohmann/json.hpp  OR  system install
#include <nlohmann/json.hpp>
using json = nlohmann::json;

bool HubConfig::load(const std::string &path)
{
    std::ifstream f(path);
    if (!f.is_open()) {
        fprintf(stderr, "[HubConfig] cannot open %s — using defaults\n", path.c_str());
        return false;
    }
    try {
        json j; f >> j;

        if (j.contains("subscriber")) {
            sub_host     = j["subscriber"].value("host", sub_host);
            gui_sub_port = j["subscriber"].value("port", gui_sub_port);
        }

        // reply ports
        gui_reply_port       = j.value("gui_reply_port",       gui_reply_port);
        gui_video_reply_port = j.value("gui_video_reply_port", gui_video_reply_port);
        gui_ai_reply_port    = j.value("gui_ai_reply_port",    gui_ai_reply_port);

        if (j.contains("cloud")) {
            cloud_endpoint = j["cloud"].value("endpoint",    cloud_endpoint);
            cloud_api_key  = j["cloud"].value("api_key",     cloud_api_key);
            cloud_retries  = j["cloud"].value("max_retries", cloud_retries);
            cloud_timeout  = j["cloud"].value("timeout_sec", cloud_timeout);
            cloud_enabled  = j["cloud"].value("enabled",     cloud_enabled);
        }
        if (j.contains("log"))
            log_path = j["log"].value("path", log_path);

        if (j.contains("gpio_map"))
            for (auto &[k, v] : j["gpio_map"].items())
                gpio_map[k] = v.get<uint32_t>();

        if (j.contains("workers")) {
            for (auto &[name, w] : j["workers"].items()) {
                WorkerEntry e;
                e.enabled       = w.value("enabled",       false);
                e.config        = w.value("config",        "");
                e.binary        = w.value("binary",        "");
                e.pub_port      = w.value("pub_port",      (uint16_t)0);
                e.settings_port = w.value("settings_port", (uint16_t)0);
                workers[name]   = e;
            }
        }
    } catch (const json::exception &e) {
        fprintf(stderr, "[HubConfig] parse error: %s\n", e.what());
        return false;
    }
    return true;
}

void HubConfig::dump() const
{
    fprintf(stdout,
            "[HubConfig]\n"
            "  gui sub        : tcp://%s:%u\n"
            "  gui reply sys  : tcp://%s:%u\n"
            "  gui reply vid  : tcp://%s:%u\n"
            "  gui reply ai   : tcp://%s:%u\n"
            "  cloud          : %s  enabled=%d\n"
            "  log            : %s\n",
            sub_host.c_str(), gui_sub_port,
            sub_host.c_str(), gui_reply_port,
            sub_host.c_str(), gui_video_reply_port,
            sub_host.c_str(), gui_ai_reply_port,
            cloud_endpoint.c_str(), cloud_enabled,
            log_path.c_str());

    fprintf(stdout, "  workers:\n");
    for (const auto &[name, w] : workers)
        fprintf(stdout, "    %-8s enabled=%d  pub=%u  settings=%u  %s\n",
                name.c_str(), w.enabled,
                w.pub_port, w.settings_port,
                w.binary.c_str());
}

std::vector<std::string> HubConfig::workerEndpoints() const
{
    std::vector<std::string> eps;

    // GUI → EventHub commands (port 9000)
    eps.push_back("tcp://" + sub_host + ":" + std::to_string(gui_sub_port));

    // Each enabled worker PUB port
    for (const auto &[name, w] : workers) {
        if (w.enabled && w.pub_port > 0) {
            eps.push_back("tcp://" + sub_host + ":" + std::to_string(w.pub_port));
            fprintf(stdout, "[HubConfig] will subscribe to %s on port %u\n",
                    name.c_str(), w.pub_port);
        }
    }
    return eps;
}