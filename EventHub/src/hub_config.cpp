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
        json j;
        f >> j;

        // subscriber
        if (j.contains("subscriber")) {
            sub_host = j["subscriber"].value("host", sub_host);
            sub_port = j["subscriber"].value("port", sub_port);
        }

        // cloud
        if (j.contains("cloud")) {
            const auto &c = j["cloud"];
            cloud_endpoint = c.value("endpoint",    cloud_endpoint);
            cloud_api_key  = c.value("api_key",     cloud_api_key);
            cloud_retries  = c.value("max_retries", cloud_retries);
            cloud_timeout  = c.value("timeout_sec", cloud_timeout);
            cloud_enabled  = c.value("enabled",     cloud_enabled);
        }

        // log
        if (j.contains("log"))
            log_path = j["log"].value("path", log_path);

        // gpio_map
        if (j.contains("gpio_map")) {
            for (auto &[k, v] : j["gpio_map"].items())
                gpio_map[k] = v.get<uint32_t>();
        }

        // workers
        if (j.contains("workers")) {
            for (auto &[name, w] : j["workers"].items()) {
                WorkerEntry e;
                e.enabled = w.value("enabled", false);
                e.config  = w.value("config",  "");
                e.binary  = w.value("binary",  "");
                workers[name] = e;
            }
        }

    } catch (const json::exception &e) {
        fprintf(stderr, "[HubConfig] parse error in %s: %s\n", path.c_str(), e.what());
        return false;
    }

    return true;
}

void HubConfig::dump() const
{
    fprintf(stdout,
            "[HubConfig]\n"
            "  subscriber : tcp://%s:%u\n"
            "  cloud      : %s  enabled=%d\n"
            "  log        : %s\n",
            sub_host.c_str(), sub_port,
            cloud_endpoint.c_str(), cloud_enabled,
            log_path.c_str());

    fprintf(stdout, "  gpio_map   :");
    for (const auto &[k, v] : gpio_map)
        fprintf(stdout, " %s->%u", k.c_str(), v);
    fprintf(stdout, "\n");

    fprintf(stdout, "  workers    :\n");
    for (const auto &[name, w] : workers)
        fprintf(stdout, "    %-8s enabled=%d  %s\n",
                name.c_str(), w.enabled, w.binary.c_str());
}