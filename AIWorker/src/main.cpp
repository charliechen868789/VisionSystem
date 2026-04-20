#include <cstdio>
#include <csignal>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <zmq.hpp>
#include "ai_config.h"
#include "detector.h"
#include "screen_event.pb.h"

static volatile bool g_running = true;
static void sigHandler(int) { g_running = false; }

static uint64_t nowMs() {
    using namespace std::chrono;
    return (uint64_t)duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count();
}

static void settingsLoop(AiConfig &cfg, std::atomic<bool> &running)
{
    zmq::context_t ctx(1);
    zmq::socket_t  pull(ctx, zmq::socket_type::pull);
    pull.set(zmq::sockopt::rcvtimeo, 500);

    std::string ep = "tcp://" + cfg.settings_host
                   + ":" + std::to_string(cfg.settings_port);
    try {
        pull.connect(ep);
        fprintf(stdout, "[AIWorker] settings PULL → %s\n", ep.c_str());
    } catch (const zmq::error_t &e) {
        fprintf(stderr, "[AIWorker] settings connect failed: %s\n", e.what());
        return;
    }

    while (running) {
        zmq::message_t msg;
        if (!pull.recv(msg)) continue;
        pfas::ScreenEvent ev;
        if (!ev.ParseFromArray(msg.data(), (int)msg.size())) continue;
        if (ev.event_type() != pfas::CONTROL_ACTION) continue;
        const auto &ca = ev.control_action();
        cfg.applyAction(ca.action(), ca.value());
    }
    pull.close(); ctx.close();
}

int main(int argc, char *argv[])
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    std::string configPath = "/etc/aeroboard/ai.json";
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--config" && i+1 < argc) configPath = argv[++i];
        else if (a == "--help") {
            fprintf(stdout, "Usage: %s [--config <path>]\n", argv[0]);
            return 0;
        }
    }

    AiConfig cfg;
    cfg.load(configPath);

    signal(SIGINT,  sigHandler);
    signal(SIGTERM, sigHandler);

    // Load detector
    Detector detector;
    bool detectorLoaded = detector.load(
        cfg.model_path,
        cfg.model_config,   // .cfg file for darknet, empty for ONNX
        cfg.names_path,
        cfg.confidence_thresh);

    if (!detectorLoaded)
        fprintf(stderr, "[AIWorker] detector not loaded — running in passthrough\n");

    // Settings thread
    std::atomic<bool> running{true};
    std::thread settingsThread(settingsLoop, std::ref(cfg), std::ref(running));

    // ZMQ
    zmq::context_t ctx(1);

    zmq::socket_t sub(ctx, zmq::socket_type::sub);
    sub.set(zmq::sockopt::subscribe, "");
    sub.set(zmq::sockopt::rcvtimeo, 500);
    sub.connect("tcp://" + cfg.sub_host + ":" + std::to_string(cfg.sub_port));

    zmq::socket_t pub(ctx, zmq::socket_type::pub);
    pub.bind("tcp://" + cfg.pub_host + ":" + std::to_string(cfg.pub_port));

    fprintf(stdout,
            "[AIWorker] running\n"
            "  model    : %s\n"
            "  sub      : tcp://%s:%u\n"
            "  pub      : tcp://%s:%u\n",
            cfg.model_path.c_str(),
            cfg.sub_host.c_str(), cfg.sub_port,
            cfg.pub_host.c_str(), cfg.pub_port);

    // Frame skip counter — run inference every N frames to maintain FPS
    int frameSkip    = 0;
    int inferEveryN  = cfg.infer_every_n_frames;  // e.g. 3 = infer every 3rd frame

    while (g_running) {
        zmq::message_t msg;
        if (!sub.recv(msg)) continue;

        pfas::ScreenEvent inEv;
        if (!inEv.ParseFromArray(msg.data(), (int)msg.size())) continue;
        if (inEv.event_type() != pfas::VIDEO_FRAME) continue;

        const auto &vf = inEv.video_frame();

        // Check if any AI feature enabled
        bool anyEnabled;
        bool streamEnabled;
        {
            std::lock_guard<std::mutex> lk(cfg.mtx);
            anyEnabled    = cfg.object_detection  ||
                            cfg.face_detection    ||
                            cfg.tracking_enabled  ||
                            cfg.pose_estimation   ||
                            cfg.anomaly_detection;
            streamEnabled = cfg.streaming_enabled.load();
        }

        // Skip if streaming disabled or no features on
        if (!streamEnabled || !anyEnabled || !detectorLoaded) continue;

        // Frame skip for performance
        if (++frameSkip < inferEveryN) continue;
        frameSkip = 0;

        // Run inference
        const std::string &jpegStr = vf.jpeg_data();
        std::vector<uint8_t> jpegVec(jpegStr.begin(), jpegStr.end());

        auto detections = detector.detect(jpegVec,
                                          cfg.input_width,
                                          cfg.input_height);

        if (detections.empty()) continue;

        // Build AI_RESULT event with all detections
        pfas::ScreenEvent outEv;
        outEv.set_timestamp_ms(nowMs());
        outEv.set_event_type(pfas::AI_RESULT);

        auto *ai = outEv.mutable_ai_result();
        {
            std::lock_guard<std::mutex> lk(cfg.mtx);
            ai->set_model(cfg.model_type);
        }
        ai->set_frame_seq(vf.frame_seq());

        // Best detection for compat
        ai->set_label(detections[0].label);
        ai->set_confidence(detections[0].confidence);

        // All detections as bounding boxes
        for (const auto &d : detections) {
            auto *det = ai->add_detections();
            det->set_label(d.label);
            det->set_confidence(d.confidence);
            det->set_x(d.x);
            det->set_y(d.y);
            det->set_width(d.w);
            det->set_height(d.h);
            det->set_class_id(d.class_id);
        }

        // Publish
        std::string bytes;
        if (!outEv.SerializeToString(&bytes)) continue;

        zmq::message_t out(bytes.data(), bytes.size());
        try {
            pub.send(out, zmq::send_flags::dontwait);
            fprintf(stdout, "[AIWorker] %zu detections on frame %u\n",
                    detections.size(), vf.frame_seq());
        } catch (const zmq::error_t &e) {
            fprintf(stderr, "[AIWorker] pub failed: %s\n", e.what());
        }
    }

    running = false;
    settingsThread.join();
    sub.close(); pub.close(); ctx.close();
    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}