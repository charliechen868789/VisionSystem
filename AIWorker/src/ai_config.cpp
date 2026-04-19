#include "ai_config.h"
#include <fstream>
#include <cstdio>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

static const char *k_models[] = {
    "/opt/models/general.xmodel",
    "/opt/models/fast_ai.xmodel",
    "/opt/models/high_accuracy.xmodel",
    "/opt/models/face_optimized.xmodel",
    "/opt/models/edge_lite.xmodel",
    "/opt/models/custom_yolo.xmodel",
    "/opt/models/pose.xmodel"
};
static const char *k_modelTypes[] = {
    "general","fast","high_accuracy",
    "face_optimized","edge_lite","custom_yolo","pose"
};
static constexpr int k_modelCount = 7;

bool AiConfig::load(const std::string &path)
{
    config_path = path;
    std::ifstream f(path);
    if (!f.is_open()) {
        fprintf(stderr, "[AiConfig] cannot open %s — using defaults\n",
                path.c_str());
        return false;
    }
    try {
        json j; f >> j;

        if (j.contains("model")) {
            model_path   = j["model"].value("path",         model_path);
            model_type   = j["model"].value("type",         model_type);
            input_width  = j["model"].value("input_width",  input_width);
            input_height = j["model"].value("input_height", input_height);
            ai_model     = j["model"].value("index",        ai_model);
        }
        if (j.contains("subscriber")) {
            sub_host = j["subscriber"].value("host", sub_host);
            sub_port = j["subscriber"].value("port", sub_port);
        }
        if (j.contains("publisher")) {
            pub_host = j["publisher"].value("host", pub_host);
            pub_port = j["publisher"].value("port", pub_port);
        }
        if (j.contains("settings")) {
            settings_host = j["settings"].value("host", settings_host);
            settings_port = j["settings"].value("port", settings_port);
        }
        confidence_thresh = j.value("confidence_threshold", confidence_thresh);
        object_detection  = j.value("object_detection",     object_detection);
        face_detection    = j.value("face_detection",       face_detection);
        tracking_enabled  = j.value("tracking_enabled",     tracking_enabled);
        pose_estimation   = j.value("pose_estimation",      pose_estimation);
        anomaly_detection = j.value("anomaly_detection",    anomaly_detection);

    } catch (const json::exception &e) {
        fprintf(stderr, "[AiConfig] parse error: %s\n", e.what());
        return false;
    }

    fprintf(stdout,
            "[AiConfig] loaded\n"
            "  model    : %s (%s)\n"
            "  sub      : tcp://%s:%u\n"
            "  pub      : tcp://%s:%u\n"
            "  settings : tcp://%s:%u\n"
            "  conf     : %.2f\n",
            model_path.c_str(), model_type.c_str(),
            sub_host.c_str(), sub_port,
            pub_host.c_str(), pub_port,
            settings_host.c_str(), settings_port,
            confidence_thresh);
    return true;
}

bool AiConfig::save() const
{
    if (config_path.empty()) return false;
    try {
        json j;
        j["model"]["path"]         = model_path;
        j["model"]["type"]         = model_type;
        j["model"]["input_width"]  = input_width;
        j["model"]["input_height"] = input_height;
        j["model"]["index"]        = ai_model;
        j["subscriber"]["host"]    = sub_host;
        j["subscriber"]["port"]    = sub_port;
        j["publisher"]["host"]     = pub_host;
        j["publisher"]["port"]     = pub_port;
        j["settings"]["host"]      = settings_host;
        j["settings"]["port"]      = settings_port;
        j["confidence_threshold"]  = confidence_thresh;
        j["object_detection"]      = object_detection;
        j["face_detection"]        = face_detection;
        j["tracking_enabled"]      = tracking_enabled;
        j["pose_estimation"]       = pose_estimation;
        j["anomaly_detection"]     = anomaly_detection;

        std::ofstream f(config_path);
        f << j.dump(2);
        fprintf(stdout, "[AiConfig] saved → %s\n", config_path.c_str());
        return true;
    } catch (const json::exception &e) {
        fprintf(stderr, "[AiConfig] save error: %s\n", e.what());
        return false;
    }
}

void AiConfig::applyAction(const std::string &action, const std::string &value)
{
    std::lock_guard<std::mutex> lk(mtx);

    if (action == "ai_model") {
        int idx = std::stoi(value);
        if (idx >= 0 && idx < k_modelCount) {
            ai_model   = idx;
            model_path = k_models[idx];
            model_type = k_modelTypes[idx];
        }
    }
    else if (action == "ai_confidence")    confidence_thresh = std::stof(value);
    else if (action == "object_detection") object_detection  = (value == "true");
    else if (action == "face_detection")   face_detection    = (value == "true");
    else if (action == "tracking_enabled") tracking_enabled  = (value == "true");
    else if (action == "pose_estimation")  pose_estimation   = (value == "true");
    else if (action == "anomaly_detection")anomaly_detection = (value == "true");
    else {
        fprintf(stderr, "[AiConfig] unknown action: %s\n", action.c_str());
        return;
    }

    fprintf(stdout, "[AiConfig] applied %s = %s\n",
            action.c_str(), value.c_str());
    save();
}