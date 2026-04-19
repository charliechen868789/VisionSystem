#include <cstdio>
#include <csignal>
#include <string>
#include <thread>
#include <atomic>
#include <zmq.hpp>
#include "video_config.h"
#include "frame_reader.h"
#include "video_publisher.h"
#include "screen_event.pb.h"

static volatile bool g_running = true;
static void sigHandler(int) { g_running = false; }

// ── Settings receiver thread ──────────────────────────────────────────────────
static void settingsLoop(VideoConfig &cfg, std::atomic<bool> &running)
{
    zmq::context_t ctx(1);
    zmq::socket_t  pull(ctx, zmq::socket_type::pull);
    pull.set(zmq::sockopt::rcvtimeo, 500);

    std::string ep = "tcp://" + cfg.settings_host
                   + ":" + std::to_string(cfg.settings_port);
    try {
        pull.connect(ep);
        fprintf(stdout, "[VideoWorker] settings PULL → %s\n", ep.c_str());
    } catch (const zmq::error_t &e) {
        fprintf(stderr, "[VideoWorker] settings connect failed: %s\n", e.what());
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

int main(int argc, char *argv[])
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    std::string configPath = "/etc/aeroboard/video.json";
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--config" && i+1 < argc) configPath = argv[++i];
        else if (a == "--help") {
            fprintf(stdout, "Usage: %s [--config <path>]\n", argv[0]);
            return 0;
        }
    }

    VideoConfig cfg;
    cfg.load(configPath);

    signal(SIGINT,  sigHandler);
    signal(SIGTERM, sigHandler);

    // Settings receiver thread
    std::atomic<bool> running{true};
    std::thread settingsThread(settingsLoop, std::ref(cfg), std::ref(running));

    // Frame publisher
    VideoPublisher pub(cfg.pub_host, cfg.pub_port);

    // Frame reader — reads cfg by reference so settings updates take effect
    FrameReader reader(cfg, [&pub](const pfas::ScreenEvent &ev) {
        pub.publish(ev);
    });

    reader.start();

    fprintf(stdout,
            "[VideoWorker] running\n"
            "  pub      : tcp://%s:%u\n"
            "  settings : tcp://%s:%u\n",
            cfg.pub_host.c_str(), cfg.pub_port,
            cfg.settings_host.c_str(), cfg.settings_port);

    while (g_running) {
        struct timespec ts{1, 0};
        nanosleep(&ts, nullptr);
    }

    fprintf(stdout, "\n[VideoWorker] shutting down\n");
    running = false;
    reader.stop();
    settingsThread.join();

    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}