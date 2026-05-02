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
#include "../../common/zmq_compat.h"

static volatile bool g_running = true;
static void sigHandler(int) { g_running = false; }

static uint64_t nowMs()
{
    using namespace std::chrono;
    return (uint64_t)duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count();
}

static void settingsLoop(AiConfig &cfg, std::atomic<bool> &running)
{
    zmq::context_t ctx(1);
    zmq::socket_t  pull(ctx, zmq::socket_type::pull);
#if CPPZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 7, 0)
    pull.set(zmq::sockopt::rcvtimeo, 500);
#else
    int timeout = 500;
    pull.setsockopt(ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
#endif
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
    pull.close();
    ctx.close();
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

    // ── Lambda to load detector from current cfg ──────────────────────────────
    auto loadDetector = [&](Detector &det) -> bool {
        std::string mpath, mcfg, mnames, mframework;
        float conf;
        {
            std::lock_guard<std::mutex> lk(cfg.mtx);
            mpath      = cfg.model_path;
            mcfg       = cfg.model_config;
            mnames     = cfg.names_path;
            mframework = cfg.model_framework;
            conf       = cfg.confidence_thresh;
        }
        fprintf(stdout, "[AIWorker] loading model: %s [%s]\n",
                mpath.c_str(), mframework.c_str());
        return det.load(mpath, mcfg, mnames, mframework, conf);
    };

    // Load initial detector
    Detector detector;
    bool detectorLoaded = loadDetector(detector);
    if (!detectorLoaded)
        fprintf(stderr, "[AIWorker] detector not loaded — passthrough mode\n");

    // Settings thread
    std::atomic<bool> running{true};
    std::thread settingsThread(settingsLoop, std::ref(cfg), std::ref(running));

    // ZMQ sockets
    zmq::context_t ctx(1);

    zmq::socket_t sub(ctx, zmq::socket_type::sub);
    zmq_set_subscribe(sub, "");
    zmq_set_rcvtimeo(sub, 500);
    sub.connect("tcp://" + cfg.sub_host + ":" + std::to_string(cfg.sub_port));

    zmq::socket_t pub(ctx, zmq::socket_type::pub);
    pub.bind("tcp://" + cfg.pub_host + ":" + std::to_string(cfg.pub_port));

    fprintf(stdout,
            "[AIWorker] running\n"
            "  model     : %s\n"
            "  framework : %s\n"
            "  sub       : tcp://%s:%u\n"
            "  pub       : tcp://%s:%u\n"
            "  skip      : every %d frames\n",
            cfg.model_path.c_str(),
            cfg.model_framework.c_str(),
            cfg.sub_host.c_str(), cfg.sub_port,
            cfg.pub_host.c_str(), cfg.pub_port,
            cfg.infer_every_n_frames);

    int frameSkip   = 0;
    int inferEveryN = cfg.infer_every_n_frames > 0
                    ? cfg.infer_every_n_frames : 1;

    while (g_running) {

        // ── Reload detector if model switched ─────────────────────────────────
        if (cfg.model_changed.load()) {
            cfg.model_changed.store(false);
            fprintf(stdout, "[AIWorker] model changed — reloading detector\n");
            Detector newDet;
            if (loadDetector(newDet)) {
                detector       = std::move(newDet);
                detectorLoaded = true;
                frameSkip      = 0;
                inferEveryN    = cfg.infer_every_n_frames > 0
                               ? cfg.infer_every_n_frames : 1;
                fprintf(stdout, "[AIWorker] detector reloaded OK\n");
            } else {
                fprintf(stderr, "[AIWorker] detector reload FAILED\n");
                detectorLoaded = false;
            }
        }

        // ── Receive frame ─────────────────────────────────────────────────────
        zmq::message_t msg;
        if (!sub.recv(msg)) continue;

        pfas::ScreenEvent inEv;
        if (!inEv.ParseFromArray(msg.data(), (int)msg.size())) continue;
        if (inEv.event_type() != pfas::VIDEO_FRAME) continue;

        const auto &vf = inEv.video_frame();

        // ── Gate checks ───────────────────────────────────────────────────────
        bool anyEnabled, streamEnabled;
        {
            std::lock_guard<std::mutex> lk(cfg.mtx);
            anyEnabled    = cfg.object_detection  ||
                            cfg.face_detection    ||
                            cfg.tracking_enabled  ||
                            cfg.pose_estimation   ||
                            cfg.anomaly_detection;
        }
        streamEnabled = cfg.streaming_enabled.load();

        if (!streamEnabled || !anyEnabled || !detectorLoaded) continue;

        // ── Frame skip ────────────────────────────────────────────────────────
        if (++frameSkip < inferEveryN) continue;
        frameSkip = 0;

        // ── Inference ─────────────────────────────────────────────────────────
        const std::string &jpegStr = vf.jpeg_data();
        std::vector<uint8_t> jpegVec(jpegStr.begin(), jpegStr.end());

        uint32_t iw, ih;
        {
            std::lock_guard<std::mutex> lk(cfg.mtx);
            iw = cfg.input_width;
            ih = cfg.input_height;
        }

        auto detections = detector.detect(jpegVec, (int)iw, (int)ih);
        if (detections.empty()) continue;

        // ── Build AI_RESULT ───────────────────────────────────────────────────
        pfas::ScreenEvent outEv;
        outEv.set_timestamp_ms(nowMs());
        outEv.set_event_type(pfas::AI_RESULT);

        auto *ai = outEv.mutable_ai_result();
        {
            std::lock_guard<std::mutex> lk(cfg.mtx);
            ai->set_model(cfg.model_type);
        }
        ai->set_frame_seq(vf.frame_seq());
        ai->set_label(detections[0].label);
        ai->set_confidence(detections[0].confidence);

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

        // ── Publish ───────────────────────────────────────────────────────────
        std::string bytes;
        if (!outEv.SerializeToString(&bytes)) continue;

        zmq::message_t out(bytes.data(), bytes.size());
        try {
            pub.send(out, zmq::send_flags::dontwait);
            fprintf(stdout, "[AIWorker] %zu detections frame=%u [%s]\n",
                    detections.size(), vf.frame_seq(),
                    ai->model().c_str());
        } catch (const zmq::error_t &e) {
            fprintf(stderr, "[AIWorker] pub failed: %s\n", e.what());
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