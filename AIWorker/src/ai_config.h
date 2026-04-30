#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <mutex>
#include <atomic>

struct ModelEntry {
    int         index        = 0;
    std::string name         = "Model";
    std::string path;
    std::string config;
    std::string names;
    std::string type         = "yolov4-tiny";
    std::string framework    = "darknet";
    uint32_t    input_width  = 416;
    uint32_t    input_height = 416;
};

struct AiConfig {
    std::vector<ModelEntry> models;
    int         active_model     = 0;

    // Active model shortcuts — updated by setActiveModel()
    std::string model_path       = "/opt/models/yolov4-tiny.weights";
    std::string model_config     = "/opt/models/yolov4-tiny.cfg";
    std::string names_path       = "/opt/models/coco.names";
    std::string model_type       = "yolov4-tiny";
    std::string model_framework  = "darknet";   // ADD
    uint32_t    input_width      = 416;
    uint32_t    input_height     = 416;

    float       confidence_thresh    = 0.5f;
    int         infer_every_n_frames = 3;

    bool        object_detection  = false;
    bool        face_detection    = false;
    bool        tracking_enabled  = false;
    bool        pose_estimation   = false;
    bool        anomaly_detection = false;

    std::string sub_host      = "127.0.0.1";
    uint16_t    sub_port      = 9001;
    std::string pub_host      = "127.0.0.1";
    uint16_t    pub_port      = 9004;
    std::string settings_host = "127.0.0.1";
    uint16_t    settings_port = 9011;

    std::string        config_path;
    mutable std::mutex mtx;
    std::atomic<bool>  streaming_enabled{false};
    std::atomic<bool>  model_changed{false};    // ADD

    bool load(const std::string &path);
    bool save() const;
    void applyAction(const std::string &action, const std::string &value);
    void setActiveModel(int idx);
};