#pragma once
#include <string>
#include <cstdint>
#include <mutex>

struct AiConfig {
    // Model
    std::string model_path        = "/opt/models/general.xmodel";
    std::string model_type        = "general";
    uint32_t    input_width       = 640;
    uint32_t    input_height      = 480;
    int         ai_model          = 0;
    float       confidence_thresh = 0.6f;

    // Feature toggles
    bool        object_detection  = false;
    bool        face_detection    = false;
    bool        tracking_enabled  = false;
    bool        pose_estimation   = false;
    bool        anomaly_detection = false;

    // Subscriber — VideoWorker PUB → AIWorker SUB
    std::string sub_host          = "127.0.0.1";
    uint16_t    sub_port          = 9001;

    // Publisher — AIWorker PUB → EventHub SUB
    std::string pub_host          = "127.0.0.1";
    uint16_t    pub_port          = 9004;

    // Settings — EventHub PUSH → AIWorker PULL
    std::string settings_host     = "127.0.0.1";
    uint16_t    settings_port     = 9011;

    std::string config_path;
    mutable std::mutex mtx;

    bool load(const std::string &path);
    bool save() const;
    void applyAction(const std::string &action, const std::string &value);
};