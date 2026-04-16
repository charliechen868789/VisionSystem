#include "ai_config.h"
#include <fstream>
#include <cstdio>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

bool AiConfig::load(const std::string &path)
{
    std::ifstream f(path);
    if (!f.is_open()) {
        fprintf(stderr, "[AiConfig] cannot open %s\n", path.c_str());
        return false;
    }
    try {
        json j; f >> j;
        if (j.contains("model")) {
            model_path   = j["model"].value("path",         model_path);
            model_type   = j["model"].value("type",         model_type);
            input_width  = j["model"].value("input_width",  input_width);
            input_height = j["model"].value("input_height", input_height);
        }
        if (j.contains("subscriber")) {
            sub_host = j["subscriber"].value("host", sub_host);
            sub_port = j["subscriber"].value("port", sub_port);
        }
        if (j.contains("publisher")) {
            pub_host = j["publisher"].value("host", pub_host);
            pub_port = j["publisher"].value("port", pub_port);
        }
        confidence_thresh = j.value("confidence_threshold", confidence_thresh);
    } catch (const json::exception &e) {
        fprintf(stderr, "[AiConfig] parse error: %s\n", e.what());
        return false;
    }
    return true;
}