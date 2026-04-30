#include "detector.h"
#include <fstream>
#include <cstdio>

static Framework parseFramework(const std::string &s)
{
    if (s == "tensorflow") return Framework::TENSORFLOW;
    if (s == "caffe")      return Framework::CAFFE;
    if (s == "onnx")       return Framework::ONNX;
    return Framework::DARKNET;
}

bool Detector::load(const std::string &modelPath,
                    const std::string &configPath,
                    const std::string &namesPath,
                    const std::string &framework,
                    float confThresh, float nmsThresh)
{
    m_confThresh = confThresh;
    m_nmsThresh  = nmsThresh;
    m_framework  = parseFramework(framework);
    m_loaded     = false;
    m_classes.clear();

    // Load class names
    if (!namesPath.empty()) {
        std::ifstream f(namesPath);
        if (f.is_open()) {
            std::string line;
            while (std::getline(f, line))
                if (!line.empty()) m_classes.push_back(line);
            fprintf(stdout, "[Detector] loaded %zu classes from %s\n",
                    m_classes.size(), namesPath.c_str());
        } else {
            fprintf(stderr, "[Detector] cannot open names: %s\n",
                    namesPath.c_str());
        }
    }

    // Load model based on framework
    try {
        switch (m_framework) {
        case Framework::DARKNET:
            if (configPath.empty()) {
                fprintf(stderr, "[Detector] darknet requires .cfg file\n");
                return false;
            }
            m_net = cv::dnn::readNetFromDarknet(configPath, modelPath);
            break;

        case Framework::TENSORFLOW:
            if (configPath.empty()) {
                fprintf(stderr, "[Detector] tensorflow requires .pbtxt file\n");
                return false;
            }
            m_net = cv::dnn::readNetFromTensorflow(modelPath, configPath);
            break;

        case Framework::CAFFE:
            if (configPath.empty()) {
                fprintf(stderr, "[Detector] caffe requires .prototxt file\n");
                return false;
            }
            m_net = cv::dnn::readNetFromCaffe(configPath, modelPath);
            break;

        case Framework::ONNX:
            m_net = cv::dnn::readNetFromONNX(modelPath);
            break;
        }
    } catch (const cv::Exception &e) {
        fprintf(stderr, "[Detector] model load failed: %s\n", e.what());
        return false;
    }

    if (m_net.empty()) {
        fprintf(stderr, "[Detector] loaded net is empty\n");
        return false;
    }

    // Backend selection
    if (cv::cuda::getCudaEnabledDeviceCount() > 0) {
    #if CV_VERSION_MAJOR > 4 || (CV_VERSION_MAJOR == 4 && CV_VERSION_MINOR >= 2)
        m_net.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
        m_net.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
    #else
        // OpenCV < 4.2 — CUDA backend not available, fall back to CPU
        m_net.setPreferableBackend(cv::dnn::DNN_BACKEND_DEFAULT);
        m_net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
    #endif
        fprintf(stdout, "[Detector] backend: CUDA\n");
    } else {
        m_net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        m_net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        fprintf(stdout, "[Detector] backend: CPU\n");
    }

    m_loaded = true;
    fprintf(stdout, "[Detector] loaded [%s] %s\n",
            framework.c_str(), modelPath.c_str());
    return true;
}

std::vector<DetectionResult> Detector::detect(
    const std::vector<uint8_t> &jpegData, int inputW, int inputH)
{
    if (!m_loaded || jpegData.empty()) return {};

    cv::Mat frame = cv::imdecode(
        cv::Mat(1, (int)jpegData.size(), CV_8UC1,
                const_cast<uint8_t*>(jpegData.data())),
        cv::IMREAD_COLOR);

    if (frame.empty()) {
        fprintf(stderr, "[Detector] JPEG decode failed\n");
        return {};
    }

    switch (m_framework) {
    case Framework::DARKNET:   return detectYOLO     (frame, inputW, inputH);
    case Framework::TENSORFLOW:return detectSSD      (frame, inputW, inputH);
    case Framework::CAFFE:
        // Distinguish face caffe from mobilenet caffe by class count
        if (m_classes.size() <= 2)
            return detectFaceCaffe(frame, inputW, inputH);
        else
            return detectSSDCaffe (frame, inputW, inputH);
    case Framework::ONNX:      return detectONNX     (frame, inputW, inputH);
    }
    return {};
}

// ── YOLO (Darknet) ────────────────────────────────────────────────────────────

std::vector<std::string> Detector::getYOLOOutputLayers()
{
    std::vector<std::string> names;
    for (int i : m_net.getUnconnectedOutLayers())
        names.push_back(m_net.getLayerNames()[i - 1]);
    return names;
}

std::vector<DetectionResult> Detector::detectYOLO(cv::Mat &frame,
                                                    int inputW, int inputH)
{
    int fw = frame.cols, fh = frame.rows;

    cv::Mat blob;
    cv::dnn::blobFromImage(frame, blob, 1.0/255.0,
                           cv::Size(inputW, inputH),
                           cv::Scalar(0,0,0), true, false);
    m_net.setInput(blob);

    std::vector<cv::Mat> outs;
    m_net.forward(outs, getYOLOOutputLayers());

    std::vector<int>      classIds;
    std::vector<float>    confs;
    std::vector<cv::Rect> boxes;

    for (const auto &out : outs) {
        for (int i = 0; i < out.rows; ++i) {
            const float *d = out.ptr<float>(i);
            if (d[4] < m_confThresh) continue;

            cv::Mat scores(1, (int)m_classes.size(), CV_32F,
                           const_cast<float*>(d + 5));
            cv::Point cls; double maxVal;
            cv::minMaxLoc(scores, nullptr, &maxVal, nullptr, &cls);

            float conf = (float)maxVal * d[4];
            if (conf < m_confThresh) continue;

            int cx = (int)(d[0]*fw), cy = (int)(d[1]*fh);
            int w  = (int)(d[2]*fw), h  = (int)(d[3]*fh);
            classIds.push_back(cls.x);
            confs.push_back(conf);
            boxes.push_back({cx-w/2, cy-h/2, w, h});
        }
    }

    std::vector<int> idx;
    cv::dnn::NMSBoxes(boxes, confs, m_confThresh, m_nmsThresh, idx);

    std::vector<DetectionResult> res;
    for (int i : idx) {
        const auto &b = boxes[i];
        DetectionResult r;
        r.class_id   = classIds[i];
        r.label      = r.class_id < (int)m_classes.size()
                       ? m_classes[r.class_id] : "unknown";
        r.confidence = confs[i];
        r.x = std::max(0.0f, (float)b.x / fw);
        r.y = std::max(0.0f, (float)b.y / fh);
        r.w = std::min(1.0f, (float)b.width  / fw);
        r.h = std::min(1.0f, (float)b.height / fh);
        res.push_back(r);
        fprintf(stdout, "[Detector/YOLO] %s %.2f\n",
                r.label.c_str(), r.confidence);
    }
    return res;
}

// ── SSD TensorFlow ────────────────────────────────────────────────────────────

std::vector<DetectionResult> Detector::detectSSD(cv::Mat &frame,
                                                   int inputW, int inputH)
{
    int fw = frame.cols, fh = frame.rows;

    cv::Mat blob = cv::dnn::blobFromImage(
        frame, 1.0, cv::Size(inputW, inputH),
        cv::Scalar(127.5, 127.5, 127.5), true, false);
    m_net.setInput(blob);

    cv::Mat det = m_net.forward();
    // det shape: [1, 1, N, 7]
    // [batch, ?, idx, class_id, conf, x1, y1, x2, y2]
    cv::Mat detMat(det.size[2], det.size[3], CV_32F, det.ptr<float>());

    std::vector<DetectionResult> res;
    for (int i = 0; i < detMat.rows; ++i) {
        float conf     = detMat.at<float>(i, 2);
        if (conf < m_confThresh) continue;

        int classId    = (int)detMat.at<float>(i, 1);
        float x1       = detMat.at<float>(i, 3) * fw;
        float y1       = detMat.at<float>(i, 4) * fh;
        float x2       = detMat.at<float>(i, 5) * fw;
        float y2       = detMat.at<float>(i, 6) * fh;

        DetectionResult r;
        r.class_id   = classId;
        r.label      = (classId > 0 && classId <= (int)m_classes.size())
                       ? m_classes[classId - 1] : "unknown";
        r.confidence = conf;
        r.x = std::max(0.0f, x1 / fw);
        r.y = std::max(0.0f, y1 / fh);
        r.w = std::min(1.0f, (x2 - x1) / fw);
        r.h = std::min(1.0f, (y2 - y1) / fh);
        res.push_back(r);
        fprintf(stdout, "[Detector/SSD-TF] %s %.2f\n",
                r.label.c_str(), r.confidence);
    }
    return res;
}

// ── SSD Caffe (MobileNet VOC) ─────────────────────────────────────────────────

std::vector<DetectionResult> Detector::detectSSDCaffe(cv::Mat &frame,
                                                        int inputW, int inputH)
{
    int fw = frame.cols, fh = frame.rows;

    cv::Mat blob = cv::dnn::blobFromImage(
        frame, 0.007843, cv::Size(inputW, inputH),
        cv::Scalar(127.5, 127.5, 127.5));
    m_net.setInput(blob);

    cv::Mat det = m_net.forward();
    cv::Mat detMat(det.size[2], det.size[3], CV_32F, det.ptr<float>());

    std::vector<DetectionResult> res;
    for (int i = 0; i < detMat.rows; ++i) {
        float conf  = detMat.at<float>(i, 2);
        if (conf < m_confThresh) continue;

        int classId = (int)detMat.at<float>(i, 1);
        float x1    = detMat.at<float>(i, 3) * fw;
        float y1    = detMat.at<float>(i, 4) * fh;
        float x2    = detMat.at<float>(i, 5) * fw;
        float y2    = detMat.at<float>(i, 6) * fh;

        DetectionResult r;
        r.class_id   = classId;
        r.label      = (classId < (int)m_classes.size())
                       ? m_classes[classId] : "unknown";
        r.confidence = conf;
        r.x = std::max(0.0f, x1 / fw);
        r.y = std::max(0.0f, y1 / fh);
        r.w = std::min(1.0f, (x2 - x1) / fw);
        r.h = std::min(1.0f, (y2 - y1) / fh);
        res.push_back(r);
        fprintf(stdout, "[Detector/SSD-Caffe] %s %.2f\n",
                r.label.c_str(), r.confidence);
    }
    return res;
}

// ── Face Caffe (res10 SSD) ────────────────────────────────────────────────────

std::vector<DetectionResult> Detector::detectFaceCaffe(cv::Mat &frame,
                                                         int inputW, int inputH)
{
    int fw = frame.cols, fh = frame.rows;

    cv::Mat blob = cv::dnn::blobFromImage(
        frame, 1.0, cv::Size(inputW, inputH),
        cv::Scalar(104.0, 177.0, 123.0));
    m_net.setInput(blob);

    cv::Mat det = m_net.forward();
    cv::Mat detMat(det.size[2], det.size[3], CV_32F, det.ptr<float>());

    std::vector<DetectionResult> res;
    for (int i = 0; i < detMat.rows; ++i) {
        float conf = detMat.at<float>(i, 2);
        if (conf < m_confThresh) continue;

        float x1 = detMat.at<float>(i, 3) * fw;
        float y1 = detMat.at<float>(i, 4) * fh;
        float x2 = detMat.at<float>(i, 5) * fw;
        float y2 = detMat.at<float>(i, 6) * fh;

        DetectionResult r;
        r.class_id   = 0;
        r.label      = "face";
        r.confidence = conf;
        r.x = std::max(0.0f, x1 / fw);
        r.y = std::max(0.0f, y1 / fh);
        r.w = std::min(1.0f, (x2 - x1) / fw);
        r.h = std::min(1.0f, (y2 - y1) / fh);
        res.push_back(r);
        fprintf(stdout, "[Detector/Face-Caffe] face %.2f\n", r.confidence);
    }
    return res;
}

// ── ONNX (EfficientDet / YOLOv5 style) ───────────────────────────────────────

std::vector<DetectionResult> Detector::detectONNX(cv::Mat &frame,
                                                    int inputW, int inputH)
{
    int fw = frame.cols, fh = frame.rows;

    cv::Mat blob;
    cv::dnn::blobFromImage(frame, blob, 1.0/255.0,
                           cv::Size(inputW, inputH),
                           cv::Scalar(0,0,0), true, false);
    m_net.setInput(blob);

    std::vector<cv::Mat> outs;
    m_net.forward(outs, m_net.getUnconnectedOutLayersNames());

    // YOLOv5-style ONNX output: [batch, num_boxes, 5+classes]
    std::vector<int>      classIds;
    std::vector<float>    confs;
    std::vector<cv::Rect> boxes;

    if (!outs.empty()) {
        cv::Mat &out = outs[0];
        // Reshape if needed
        if (out.dims == 3)
            out = out.reshape(1, out.size[1]);

        for (int i = 0; i < out.rows; ++i) {
            float *d    = out.ptr<float>(i);
            float objConf = d[4];
            if (objConf < m_confThresh) continue;

            int numClasses = out.cols - 5;
            int bestClass  = 0;
            float bestConf = 0;
            for (int c = 0; c < numClasses; ++c) {
                float sc = d[5 + c] * objConf;
                if (sc > bestConf) { bestConf = sc; bestClass = c; }
            }
            if (bestConf < m_confThresh) continue;

            int cx = (int)(d[0]*fw), cy = (int)(d[1]*fh);
            int w  = (int)(d[2]*fw), h  = (int)(d[3]*fh);
            classIds.push_back(bestClass);
            confs.push_back(bestConf);
            boxes.push_back({cx-w/2, cy-h/2, w, h});
        }
    }

    std::vector<int> idx;
    cv::dnn::NMSBoxes(boxes, confs, m_confThresh, m_nmsThresh, idx);

    std::vector<DetectionResult> res;
    for (int i : idx) {
        const auto &b = boxes[i];
        DetectionResult r;
        r.class_id   = classIds[i];
        r.label      = r.class_id < (int)m_classes.size()
                       ? m_classes[r.class_id] : "unknown";
        r.confidence = confs[i];
        r.x = std::max(0.0f, (float)b.x / fw);
        r.y = std::max(0.0f, (float)b.y / fh);
        r.w = std::min(1.0f, (float)b.width  / fw);
        r.h = std::min(1.0f, (float)b.height / fh);
        res.push_back(r);
        fprintf(stdout, "[Detector/ONNX] %s %.2f\n",
                r.label.c_str(), r.confidence);
    }
    return res;
}