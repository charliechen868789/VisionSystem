#pragma once
#include <string>
#include <cstdint>

struct AiConfig {
    std::string model_path        = "/opt/models/face_detect.xmodel";
    std::string model_type        = "face_detection";
    uint32_t    input_width       = 640;
    uint32_t    input_height      = 480;
    std::string sub_host          = "127.0.0.1";
    uint16_t    sub_port          = 9001;   // subscribes to VideoWorker
    std::string pub_host          = "127.0.0.1";
    uint16_t    pub_port          = 9004;   // publishes AI results
    float       confidence_thresh = 0.6f;

    bool load(const std::string &path);
};