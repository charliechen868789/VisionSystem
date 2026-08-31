#include "detector.h"
#include <fstream>
#include <cstdio>
#include <cmath>

static Framework parseFramework(const std::string &s)
{
    if (s == "tensorflow") return Framework::TENSORFLOW;
    if (s == "caffe")      return Framework::CAFFE;
    if (s == "onnx")       return Framework::ONNX;
    if (s == "tensorrt")   return Framework::TENSORRT;
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

    if (m_framework == Framework::TENSORRT) {
#ifdef HAVE_TENSORRT
        if (!loadTensorRT(modelPath)) return false;
        m_loaded = true;
        fprintf(stdout, "[Detector] loaded [tensorrt] %s\n", modelPath.c_str());
        return true;
#else
        fprintf(stderr,
                "[Detector] framework=tensorrt requested but this build has no "
                "TensorRT support (HAVE_TENSORRT not defined at compile time)\n");
        return false;
#endif
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
#ifdef HAVE_TENSORRT
    case Framework::TENSORRT: return detectTensorRT (frame, inputW, inputH);
#endif
    default: return {};
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

// ── TensorRT (pre-built .engine, GPU inference) ──────────────────────────────
#ifdef HAVE_TENSORRT

void TrtLogger::log(Severity severity, const char *msg) noexcept
{
    if (severity <= Severity::kWARNING)
        fprintf(stderr, "[TensorRT] %s\n", msg);
}

void TrtState::reset()
{
    if (inputDev) { cudaFree(inputDev); inputDev = nullptr; }
    for (auto &o : outputs)
        if (o.dev) cudaFree(o.dev);
    outputs.clear();
    delete context; context = nullptr;
    delete engine;  engine  = nullptr;
    delete runtime; runtime = nullptr;
    inputBytes = 0;
}

TrtState &TrtState::operator=(TrtState &&other) noexcept
{
    if (this == &other) return *this;
    reset();
    runtime    = other.runtime;    other.runtime    = nullptr;
    engine     = other.engine;     other.engine     = nullptr;
    context    = other.context;    other.context    = nullptr;
    inputDev   = other.inputDev;   other.inputDev   = nullptr;
    inputBytes = other.inputBytes; other.inputBytes = 0;
    outputs    = std::move(other.outputs);
    return *this;
}

// YOLOv4-tiny's fixed architecture: 2 detection heads, 3 anchors each,
// matching the [yolo] layers' mask/anchors in yolov4-tiny.cfg. Keyed by
// output grid size (yolov4-tiny is always square: 13x13 and 26x26 for a
// 416x416 input). This is specific to yolov4-tiny — a different YOLO
// variant's engine needs its own anchor table here.
struct YoloAnchorSet { int gridSize; float anchors[3][2]; };
static const YoloAnchorSet kYoloV4TinyAnchors[] = {
    {13, {{81,82}, {135,169}, {344,319}}},   // stride 32 — large objects
    {26, {{23,27}, {37,58},   {81,82}}},     // stride 16 — small objects
};

static inline float sigmoidf(float x) { return 1.0f / (1.0f + std::exp(-x)); }

bool Detector::loadTensorRT(const std::string &enginePath)
{
    m_trt.reset();

    std::ifstream f(enginePath, std::ios::binary);
    if (!f.is_open()) {
        fprintf(stderr, "[Detector] cannot open engine file: %s\n", enginePath.c_str());
        return false;
    }
    std::vector<char> engineData((std::istreambuf_iterator<char>(f)),
                                  std::istreambuf_iterator<char>());
    if (engineData.empty()) {
        fprintf(stderr, "[Detector] engine file is empty: %s\n", enginePath.c_str());
        return false;
    }

    m_trt.runtime = nvinfer1::createInferRuntime(m_trtLogger);
    if (!m_trt.runtime) {
        fprintf(stderr, "[Detector] createInferRuntime failed\n");
        return false;
    }

    m_trt.engine = m_trt.runtime->deserializeCudaEngine(engineData.data(), engineData.size());
    if (!m_trt.engine) {
        fprintf(stderr, "[Detector] deserializeCudaEngine failed for %s\n", enginePath.c_str());
        m_trt.reset();
        return false;
    }

    m_trt.context = m_trt.engine->createExecutionContext();
    if (!m_trt.context) {
        fprintf(stderr, "[Detector] createExecutionContext failed\n");
        m_trt.reset();
        return false;
    }

    auto volume = [](const nvinfer1::Dims &d) {
        int64_t v = 1;
        for (int i = 0; i < d.nbDims; ++i) v *= (d.d[i] > 0 ? d.d[i] : 1);
        return v;
    };

    // One input + one-or-more raw YOLO detection-head outputs (a yolov4-tiny
    // engine converted via yolo_to_onnx.py has 2: [1,255,13,13] and
    // [1,255,26,26] — undecoded conv outputs, not flattened boxes).
    int inputIdx = -1;
    int nbBindings = m_trt.engine->getNbBindings();
    for (int i = 0; i < nbBindings; ++i)
        if (m_trt.engine->bindingIsInput(i)) { inputIdx = i; break; }

    if (inputIdx < 0) {
        fprintf(stderr, "[Detector] no input binding found in engine\n");
        m_trt.reset();
        return false;
    }

    nvinfer1::Dims inDims = m_trt.engine->getBindingDimensions(inputIdx);
    m_trt.inputBytes = (size_t)volume(inDims) * sizeof(float);
    if (cudaMalloc(&m_trt.inputDev, m_trt.inputBytes) != cudaSuccess) {
        fprintf(stderr, "[Detector] cudaMalloc failed for input (%zu bytes)\n", m_trt.inputBytes);
        m_trt.reset();
        return false;
    }

    for (int i = 0; i < nbBindings; ++i) {
        if (i == inputIdx) continue;
        nvinfer1::Dims d = m_trt.engine->getBindingDimensions(i);
        if (d.nbDims != 4) {
            fprintf(stderr, "[Detector] unexpected output rank %d on binding %d\n", d.nbDims, i);
            m_trt.reset();
            return false;
        }
        TrtOutputBinding ob;
        ob.bindingIndex = i;
        ob.channels = d.d[1];
        ob.height   = d.d[2];
        ob.width    = d.d[3];
        ob.bytes    = (size_t)volume(d) * sizeof(float);
        if (cudaMalloc(&ob.dev, ob.bytes) != cudaSuccess) {
            fprintf(stderr, "[Detector] cudaMalloc failed for output binding %d (%zu bytes)\n",
                    i, ob.bytes);
            m_trt.reset();
            return false;
        }
        m_trt.outputs.push_back(ob);
    }

    if (m_trt.outputs.empty()) {
        fprintf(stderr, "[Detector] engine has no output bindings\n");
        m_trt.reset();
        return false;
    }

    fprintf(stdout, "[Detector] TensorRT engine loaded: %s (%zu output head%s)\n",
            enginePath.c_str(), m_trt.outputs.size(), m_trt.outputs.size() == 1 ? "" : "s");
    return true;
}

std::vector<DetectionResult> Detector::detectTensorRT(cv::Mat &frame, int w, int h)
{
    // Same preprocessing as detectYOLO(): 0-1 normalize, BGR->RGB, HWC->CHW.
    cv::Mat blob;
    cv::dnn::blobFromImage(frame, blob, 1.0/255.0, cv::Size(w, h),
                           cv::Scalar(0,0,0), true, false);

    if ((size_t)blob.total() * sizeof(float) != m_trt.inputBytes) {
        fprintf(stderr,
                "[Detector] TensorRT input size mismatch: blob=%zu bytes, engine expects %zu\n",
                (size_t)blob.total() * sizeof(float), m_trt.inputBytes);
        return {};
    }

    cudaError_t err = cudaMemcpy(m_trt.inputDev, blob.ptr<float>(), m_trt.inputBytes,
                                  cudaMemcpyHostToDevice);
    if (err != cudaSuccess) {
        fprintf(stderr, "[Detector] cudaMemcpy H2D failed: %s\n", cudaGetErrorString(err));
        return {};
    }

    std::vector<void *> bindings(m_trt.engine->getNbBindings(), nullptr);
    for (auto &o : m_trt.outputs) bindings[o.bindingIndex] = o.dev;
    for (size_t i = 0; i < bindings.size(); ++i)
        if (!bindings[i]) bindings[i] = m_trt.inputDev;   // the one remaining slot is the input

    if (!m_trt.context->executeV2(bindings.data())) {
        fprintf(stderr, "[Detector] TensorRT executeV2 failed\n");
        return {};
    }

    std::vector<int>      classIds;
    std::vector<float>    confs;
    std::vector<cv::Rect> boxes;

    for (auto &ob : m_trt.outputs) {
        std::vector<float> data(ob.bytes / sizeof(float));
        err = cudaMemcpy(data.data(), ob.dev, ob.bytes, cudaMemcpyDeviceToHost);
        if (err != cudaSuccess) {
            fprintf(stderr, "[Detector] cudaMemcpy D2H failed: %s\n", cudaGetErrorString(err));
            continue;
        }

        const YoloAnchorSet *anchorSet = nullptr;
        for (auto &a : kYoloV4TinyAnchors)
            if (a.gridSize == ob.height) { anchorSet = &a; break; }
        if (!anchorSet) {
            fprintf(stderr,
                    "[Detector] no anchor table for %dx%d output — expected a yolov4-tiny "
                    "engine (13x13 / 26x26 heads)\n", ob.height, ob.width);
            continue;
        }

        const int numAnchors = 3;
        const int numClasses = ob.channels / numAnchors - 5;
        const int stride     = w / ob.width;   // network input size / grid size
        const int gh = ob.height, gw = ob.width;

        // Channel-first layout: data[(a*(5+numClasses)+attr)*gh*gw + gy*gw + gx]
        for (int a = 0; a < numAnchors; ++a) {
            int base = a * (5 + numClasses) * gh * gw;
            for (int gy = 0; gy < gh; ++gy) {
                for (int gx = 0; gx < gw; ++gx) {
                    int cell = gy * gw + gx;
                    float tobj = data[base + 4 * gh * gw + cell];
                    float obj  = sigmoidf(tobj);
                    if (obj < m_confThresh) continue;

                    int   bestClass = 0;
                    float bestScore = 0;
                    for (int c = 0; c < numClasses; ++c) {
                        float sc = sigmoidf(data[base + (5 + c) * gh * gw + cell]);
                        if (sc > bestScore) { bestScore = sc; bestClass = c; }
                    }
                    float conf = obj * bestScore;
                    if (conf < m_confThresh) continue;

                    float tx = data[base + 0 * gh * gw + cell];
                    float ty = data[base + 1 * gh * gw + cell];
                    float tw = data[base + 2 * gh * gw + cell];
                    float th = data[base + 3 * gh * gw + cell];

                    float bx = (sigmoidf(tx) + gx) * stride;
                    float by = (sigmoidf(ty) + gy) * stride;
                    float bw = std::exp(tw) * anchorSet->anchors[a][0];
                    float bh = std::exp(th) * anchorSet->anchors[a][1];

                    classIds.push_back(bestClass);
                    confs.push_back(conf);
                    boxes.push_back({(int)(bx - bw / 2), (int)(by - bh / 2),
                                      (int)bw, (int)bh});
                }
            }
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
        // Boxes were decoded in network-input pixel space (w x h); normalize
        // by that, not the source frame size — the frame was already resized
        // to the network's input dimensions for this pass.
        r.x = std::max(0.0f, (float)b.x / w);
        r.y = std::max(0.0f, (float)b.y / h);
        r.w = std::min(1.0f, (float)b.width  / w);
        r.h = std::min(1.0f, (float)b.height / h);
        res.push_back(r);
        fprintf(stdout, "[Detector/TensorRT] %s %.2f\n",
                r.label.c_str(), r.confidence);
    }
    return res;
}

#endif // HAVE_TENSORRT