#include <cstdio>
#include <csignal>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <zmq.hpp>
#include "ai_config.h"
#include "screen_event.pb.h"

static volatile bool g_running = true;
static void sigHandler(int) { g_running = false; }

static uint64_t nowMs()
{
    using namespace std::chrono;
    return (uint64_t)duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count();
}

// ── Settings receiver thread ──────────────────────────────────────────────────
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
        auto res = pull.recv(msg);
        if (!res) continue;

        pfas::ScreenEvent ev;
        if (!ev.ParseFromArray(msg.data(), (int)msg.size())) continue;
        if (ev.event_type() != pfas::CONTROL_ACTION) continue;

        const auto &ca = ev.control_action();
        cfg.applyAction(ca.action(), ca.value());
    }

    pull.close();
    ctx.close();
}

// ── Check if any AI feature is enabled ───────────────────────────────────────
static bool anyEnabled(const AiConfig &cfg)
{
    return cfg.object_detection  ||
           cfg.face_detection    ||
           cfg.tracking_enabled  ||
           cfg.pose_estimation   ||
           cfg.anomaly_detection;
}

// ── Stub inference result ─────────────────────────────────────────────────────
// Replace this with real Vitis AI / DPU / TensorRT call
struct InferResult {
    std::string label;
    float       confidence;
    bool        valid;
};

static InferResult runInference(const AiConfig &cfg,
                                const std::string &/*jpegData*/)
{
    // TODO: load model from cfg.model_path, run on jpegData
    // Return stub for now:
    InferResult r;
    r.label      = "person";
    r.confidence = 0.92f;
    r.valid      = true;
    return r;
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

    // Settings receiver thread
    std::atomic<bool> running{true};
    std::thread settingsThread(settingsLoop, std::ref(cfg), std::ref(running));

    // ZMQ sockets
    zmq::context_t ctx(1);

    // SUB — receive VIDEO_FRAME from VideoWorker
    zmq::socket_t sub(ctx, zmq::socket_type::sub);
    sub.set(zmq::sockopt::subscribe, "");
    sub.set(zmq::sockopt::rcvtimeo, 500);
    sub.connect("tcp://" + cfg.sub_host + ":" + std::to_string(cfg.sub_port));

    // PUB — send AI_RESULT to EventHub
    zmq::socket_t pub(ctx, zmq::socket_type::pub);
    pub.bind("tcp://" + cfg.pub_host + ":" + std::to_string(cfg.pub_port));

    fprintf(stdout,
            "[AIWorker] running\n"
            "  model    : %s (%s)\n"
            "  sub      : tcp://%s:%u  (VideoWorker frames)\n"
            "  pub      : tcp://%s:%u  (AI results → EventHub)\n"
            "  settings : tcp://%s:%u\n",
            cfg.model_path.c_str(), cfg.model_type.c_str(),
            cfg.sub_host.c_str(), cfg.sub_port,
            cfg.pub_host.c_str(), cfg.pub_port,
            cfg.settings_host.c_str(), cfg.settings_port);

    while (g_running) {
        // Receive frame from VideoWorker
        zmq::message_t msg;
        auto res = sub.recv(msg);
        if (!res) continue;   // timeout — check g_running

        pfas::ScreenEvent inEv;
        if (!inEv.ParseFromArray(msg.data(), (int)msg.size())) {
            fprintf(stderr, "[AIWorker] proto parse failed\n");
            continue;
        }
        if (inEv.event_type() != pfas::VIDEO_FRAME) continue;

        const auto &vf = inEv.video_frame();

        // Skip inference if all features disabled
        {
            std::lock_guard<std::mutex> lk(cfg.mtx);
            if (!anyEnabled(cfg)) continue;
        }

        // Run inference
        InferResult result = runInference(cfg, vf.jpeg_data());
        if (!result.valid) continue;

        // Check confidence threshold
        {
            std::lock_guard<std::mutex> lk(cfg.mtx);
            if (result.confidence < cfg.confidence_thresh) continue;
        }

        // Build and publish AI_RESULT
        pfas::ScreenEvent outEv;
        outEv.set_timestamp_ms(nowMs());
        outEv.set_event_type(pfas::AI_RESULT);

        auto *ai = outEv.mutable_ai_result();
        {
            std::lock_guard<std::mutex> lk(cfg.mtx);
            ai->set_model(cfg.model_type);
        }
        ai->set_label(result.label);
        ai->set_confidence(result.confidence);
        ai->set_frame_seq(vf.frame_seq());

        std::string bytes;
        if (!outEv.SerializeToString(&bytes)) continue;

        zmq::message_t out(bytes.data(), bytes.size());
        try {
            pub.send(out, zmq::send_flags::dontwait);
            fprintf(stdout, "[AIWorker] result: label=%s conf=%.2f frame=%u\n",
                    ai->label().c_str(), result.confidence, vf.frame_seq());
        } catch (const zmq::error_t &e) {
            fprintf(stderr, "[AIWorker] pub send failed: %s\n", e.what());
        }
    }

    fprintf(stdout, "\n[AIWorker] shutting down\n");
    running = false;
    settingsThread.join();
    sub.close();
    pub.close();
    ctx.close();
    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}