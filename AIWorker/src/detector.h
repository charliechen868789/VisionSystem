#pragma once
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>

struct DetectionResult {
    std::string label;
    float       confidence;
    float       x, y, w, h;  // normalized 0-1
    int         class_id;
};

class Detector
{
public:
    Detector() = default;
    ~Detector() = default;

    bool load(const std::string &modelPath,
              const std::string &configPath,
              const std::string &namesPath,
              float              confThresh = 0.5f,
              float              nmsThresh  = 0.4f);

    bool isLoaded() const { return m_loaded; }

    std::vector<DetectionResult> detect(const std::vector<uint8_t> &jpegData,
                                        int inputW = 416,
                                        int inputH = 416);

private:
    cv::dnn::Net         m_net;
    std::vector<std::string> m_classes;
    float                m_confThresh = 0.5f;
    float                m_nmsThresh  = 0.4f;
    bool                 m_loaded     = false;

    std::vector<std::string> getOutputLayerNames();
};