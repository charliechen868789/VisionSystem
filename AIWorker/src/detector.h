#pragma once
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>

struct DetectionResult {
    std::string label;
    float       confidence;
    float       x, y, w, h;
    int         class_id;
};

enum class Framework {
    DARKNET,      // YOLOv3/v4 .weights + .cfg
    TENSORFLOW,   // SSD MobileNet .pb + .pbtxt
    CAFFE,        // MobileNet SSD / Face .caffemodel + .prototxt
    ONNX          // EfficientDet / YOLOv5 .onnx
};

class Detector
{
public:
    Detector() = default;

    bool load(const std::string &modelPath,
              const std::string &configPath,
              const std::string &namesPath,
              const std::string &framework,
              float              confThresh = 0.5f,
              float              nmsThresh  = 0.4f);

    bool isLoaded() const { return m_loaded; }

    std::vector<DetectionResult> detect(const std::vector<uint8_t> &jpegData,
                                        int inputW = 416,
                                        int inputH = 416);

private:
    std::vector<DetectionResult> detectYOLO     (cv::Mat &frame, int w, int h);
    std::vector<DetectionResult> detectSSD      (cv::Mat &frame, int w, int h);
    std::vector<DetectionResult> detectSSDCaffe (cv::Mat &frame, int w, int h);
    std::vector<DetectionResult> detectFaceCaffe(cv::Mat &frame, int w, int h);
    std::vector<DetectionResult> detectONNX     (cv::Mat &frame, int w, int h);

    std::vector<std::string> getYOLOOutputLayers();

    cv::dnn::Net             m_net;
    std::vector<std::string> m_classes;
    Framework                m_framework   = Framework::DARKNET;
    float                    m_confThresh  = 0.5f;
    float                    m_nmsThresh   = 0.4f;
    bool                     m_loaded      = false;
};