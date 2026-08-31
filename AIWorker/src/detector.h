#pragma once
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>

#ifdef HAVE_TENSORRT
#include <NvInfer.h>
#include <cuda_runtime_api.h>
#endif

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
    ONNX,         // EfficientDet / YOLOv5 .onnx
    TENSORRT      // Pre-built .engine, GPU inference via TensorRT's C++ API
};

#ifdef HAVE_TENSORRT

// Routes TensorRT's own diagnostics through the project's stderr logging
// convention instead of TensorRT's default stdout dump.
class TrtLogger : public nvinfer1::ILogger {
public:
    void log(Severity severity, const char *msg) noexcept override;
};

// One raw YOLO detection-head output (e.g. the 13x13 or 26x26 scale of a
// yolov4-tiny engine) — channel-first [1, numAnchors*(5+numClasses), H, W],
// undecoded (raw conv output, no sigmoid/exp applied yet).
struct TrtOutputBinding {
    int     bindingIndex = -1;
    void   *dev          = nullptr;
    size_t  bytes        = 0;
    int     channels = 0, height = 0, width = 0;
};

// Owns every TensorRT/CUDA resource Detector's engine backend allocates.
// Kept as its own move-only type so Detector's compiler-generated move
// constructor/assignment (needed for the reload-on-model-switch path in
// AIWorker/main.cpp) transfers GPU ownership correctly instead of double-
// freeing — Detector itself declares no special member functions.
struct TrtState {
    nvinfer1::IRuntime            *runtime  = nullptr;
    nvinfer1::ICudaEngine         *engine   = nullptr;
    nvinfer1::IExecutionContext   *context  = nullptr;
    void                           *inputDev   = nullptr;
    size_t                          inputBytes = 0;
    std::vector<TrtOutputBinding>  outputs;

    TrtState() = default;
    ~TrtState() { reset(); }
    TrtState(TrtState &&other) noexcept { *this = std::move(other); }
    TrtState &operator=(TrtState &&other) noexcept;
    TrtState(const TrtState &) = delete;
    TrtState &operator=(const TrtState &) = delete;

    void reset();
};

#endif // HAVE_TENSORRT

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

#ifdef HAVE_TENSORRT
    bool                          loadTensorRT(const std::string &enginePath);
    std::vector<DetectionResult>  detectTensorRT(cv::Mat &frame, int w, int h);

    TrtLogger m_trtLogger;
    TrtState  m_trt;
#endif
};
