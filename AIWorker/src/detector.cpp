#include "detector.h"
#include <fstream>
#include <cstdio>

bool Detector::load(const std::string &modelPath,
                    const std::string &configPath,
                    const std::string &namesPath,
                    float confThresh,
                    float nmsThresh)
{
    m_confThresh = confThresh;
    m_nmsThresh  = nmsThresh;

    // Load class names
    std::ifstream f(namesPath);
    if (!f.is_open()) {
        fprintf(stderr, "[Detector] cannot open names: %s\n", namesPath.c_str());
        return false;
    }
    std::string line;
    while (std::getline(f, line))
        if (!line.empty()) m_classes.push_back(line);

    fprintf(stdout, "[Detector] loaded %zu classes from %s\n",
            m_classes.size(), namesPath.c_str());

    // Load model
    try {
        if (configPath.empty()) {
            // ONNX model
            m_net = cv::dnn::readNetFromONNX(modelPath);
        } else {
            // Darknet YOLO
            m_net = cv::dnn::readNetFromDarknet(configPath, modelPath);
        }
    } catch (const cv::Exception &e) {
        fprintf(stderr, "[Detector] model load failed: %s\n", e.what());
        return false;
    }

    // Use CUDA if available, otherwise CPU
    if (cv::cuda::getCudaEnabledDeviceCount() > 0) {
        m_net.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
        m_net.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
        fprintf(stdout, "[Detector] using CUDA backend\n");
    } else {
        m_net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        m_net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        fprintf(stdout, "[Detector] using CPU backend\n");
    }

    m_loaded = true;
    fprintf(stdout, "[Detector] model loaded: %s\n", modelPath.c_str());
    return true;
}

std::vector<std::string> Detector::getOutputLayerNames()
{
    std::vector<std::string> names;
    auto outLayers = m_net.getUnconnectedOutLayers();
    auto layerNames = m_net.getLayerNames();
    for (int i : outLayers)
        names.push_back(layerNames[i - 1]);
    return names;
}

std::vector<DetectionResult> Detector::detect(
    const std::vector<uint8_t> &jpegData,
    int inputW, int inputH)
{
    if (!m_loaded || jpegData.empty()) return {};

    // Decode JPEG
    cv::Mat frame = cv::imdecode(
        cv::Mat(1, (int)jpegData.size(), CV_8UC1,
                const_cast<uint8_t*>(jpegData.data())),
        cv::IMREAD_COLOR);

    if (frame.empty()) {
        fprintf(stderr, "[Detector] JPEG decode failed\n");
        return {};
    }

    int frameW = frame.cols;
    int frameH = frame.rows;

    // Create blob
    cv::Mat blob;
    cv::dnn::blobFromImage(frame, blob, 1.0/255.0,
                           cv::Size(inputW, inputH),
                           cv::Scalar(0,0,0), true, false);
    m_net.setInput(blob);

    // Forward pass
    std::vector<cv::Mat> outs;
    m_net.forward(outs, getOutputLayerNames());

    // Parse detections
    std::vector<int>   classIds;
    std::vector<float> confidences;
    std::vector<cv::Rect> boxes;

    for (const auto &out : outs) {
        for (int i = 0; i < out.rows; ++i) {
            const float *data = out.ptr<float>(i);
            float objConf = data[4];
            if (objConf < m_confThresh) continue;

            // Find best class
            cv::Mat scores(1, (int)m_classes.size(), CV_32F,
                           const_cast<float*>(data + 5));
            cv::Point classLoc;
            double maxVal;
            cv::minMaxLoc(scores, nullptr, &maxVal, nullptr, &classLoc);

            float conf = (float)maxVal * objConf;
            if (conf < m_confThresh) continue;

            // Box in pixel coords
            int cx = (int)(data[0] * frameW);
            int cy = (int)(data[1] * frameH);
            int w  = (int)(data[2] * frameW);
            int h  = (int)(data[3] * frameH);

            classIds.push_back(classLoc.x);
            confidences.push_back(conf);
            boxes.push_back(cv::Rect(cx - w/2, cy - h/2, w, h));
        }
    }

    // NMS
    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes, confidences, m_confThresh,
                      m_nmsThresh, indices);

    std::vector<DetectionResult> results;
    for (int idx : indices) {
        DetectionResult r;
        r.class_id   = classIds[idx];
        r.label      = (r.class_id < (int)m_classes.size())
                       ? m_classes[r.class_id] : "unknown";
        r.confidence = confidences[idx];

        // Normalize to 0-1
        const auto &b = boxes[idx];
        r.x = std::max(0.0f, (float)b.x / frameW);
        r.y = std::max(0.0f, (float)b.y / frameH);
        r.w = std::min(1.0f, (float)b.width  / frameW);
        r.h = std::min(1.0f, (float)b.height / frameH);

        results.push_back(r);
        fprintf(stdout, "[Detector] %s %.2f  box=(%.2f,%.2f,%.2f,%.2f)\n",
                r.label.c_str(), r.confidence, r.x, r.y, r.w, r.h);
    }

    return results;
}