#pragma once
#include <string>
#include <cstdint>
#include <mutex>
#include <atomic>      // ADD


struct AiConfig {
    // Model
    std::string model_path        = "/opt/models/yolov4-tiny.weights";
    std::string model_config      = "/opt/models/yolov4-tiny.cfg";
    std::string names_path        = "/opt/models/coco.names";
    std::string model_type        = "yolov4-tiny";
    uint32_t    input_width       = 640;
    uint32_t    input_height      = 480;
    int         ai_model          = 0;
    float       confidence_thresh = 0.6f;
    int         infer_every_n_frames = 3;

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
    std::atomic<bool>       streaming_enabled{true};  // ADD
    bool load(const std::string &path);
    bool save() const;
    void applyAction(const std::string &action, const std::string &value);
};