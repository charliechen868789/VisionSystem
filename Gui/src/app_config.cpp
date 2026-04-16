#include "app_config.h"
#include <fstream>
#include <cstdio>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

bool GuiConfig::load(const std::string &path)
{
    std::ifstream f(path);
    if (!f.is_open()) {
        fprintf(stderr, "[GuiConfig] cannot open %s — using defaults\n", path.c_str());
        return false;
    }
    try {
        json j;
        f >> j;

        if (j.contains("publisher")) {
            pub_host = j["publisher"].value("host", pub_host);
            pub_port = j["publisher"].value("port", pub_port);
        }
        if (j.contains("defaults")) {
            const auto &d = j["defaults"];
            gpio0       = d.value("gpio0",       gpio0);
            gpio1       = d.value("gpio1",       gpio1);
            pwm_enable  = d.value("pwm_enable",  pwm_enable);
            spi_bus     = d.value("spi_bus",     spi_bus);
            brightness  = d.value("brightness",  brightness);
            resolution  = d.value("resolution",  resolution);
            video_source= d.value("video_source",video_source);
            auto_start  = d.value("auto_start",  auto_start);
            debug_logging=d.value("debug_logging",debug_logging);
            watchdog    = d.value("watchdog",    watchdog);
            low_power   = d.value("low_power",   low_power);
        }
        if (j.contains("ui")) {
            fullscreen       = j["ui"].value("fullscreen",       fullscreen);
            theme            = j["ui"].value("theme",            theme);
            firmware_version = j["ui"].value("firmware_version", firmware_version);
        }
    } catch (const json::exception &e) {
        fprintf(stderr, "[GuiConfig] parse error: %s\n", e.what());
        return false;
    }
    return true;
}

void GuiConfig::dump() const
{
    fprintf(stdout,
            "[GuiConfig]\n"
            "  publisher : tcp://%s:%u\n"
            "  gpio0=%d gpio1=%d pwm=%d spi=%d\n"
            "  brightness=%d resolution=%d vsource=%d\n"
            "  fullscreen=%d theme=%s fw=%s\n",
            pub_host.c_str(), pub_port,
            gpio0, gpio1, pwm_enable, spi_bus,
            brightness, resolution, video_source,
            fullscreen, theme.c_str(), firmware_version.c_str());
}