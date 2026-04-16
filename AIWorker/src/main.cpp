#include <cstdio>
#include <csignal>
#include <string>
#include <zmq.hpp>
#include "ai_config.h"
#include "screen_event.pb.h"

static volatile bool g_running = true;
static void sigHandler(int) { g_running = false; }

static uint64_t nowMs() {
    using namespace std::chrono;
    return (uint64_t)duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count();
}

int main(int argc, char *argv[])
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    std::string configPath = "/etc/aeroboard/ai.json";
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--config" && i+1 < argc) configPath = argv[++i];
    }

    AiConfig cfg;
    cfg.load(configPath);
    signal(SIGINT, sigHandler); signal(SIGTERM, sigHandler);

    zmq::context_t ctx(1);

    // Subscribe to VideoWorker frames
    zmq::socket_t sub(ctx, zmq::socket_type::sub);
    sub.connect("tcp://" + cfg.sub_host + ":" + std::to_string(cfg.sub_port));
    sub.set(zmq::sockopt::subscribe, "");

    // Publish AI results back to EventHub
    zmq::socket_t pub(ctx, zmq::socket_type::pub);
    pub.bind("tcp://" + cfg.pub_host + ":" + std::to_string(cfg.pub_port));

    fprintf(stdout, "[AIWorker] model=%s  sub=%s:%u  pub=%s:%u\n",
            cfg.model_path.c_str(),
            cfg.sub_host.c_str(), cfg.sub_port,
            cfg.pub_host.c_str(), cfg.pub_port);

    // TODO: load Vitis AI / DPU model here
    // vart::Runner *runner = ...

    while (g_running) {
        zmq::message_t msg;
        auto res = sub.recv(msg, zmq::recv_flags::dontwait);
        if (!res) {
            struct timespec ts{0, 10000000}; nanosleep(&ts, nullptr);
            continue;
        }

        pfas::ScreenEvent inEv;
        if (!inEv.ParseFromArray(msg.data(), (int)msg.size())) continue;
        if (inEv.event_type() != pfas::VIDEO_FRAME) continue;

        const auto &vf = inEv.video_frame();

        // TODO: run inference on vf.jpeg_data()
        // For now emit stub result
        float confidence = 0.92f;
        if (confidence < cfg.confidence_thresh) continue;

        pfas::ScreenEvent outEv;
        outEv.set_timestamp_ms(nowMs());
        outEv.set_event_type(pfas::AI_RESULT);
        auto *ai = outEv.mutable_ai_result();
        ai->set_model(cfg.model_type);
        ai->set_label("face");
        ai->set_confidence(confidence);
        ai->set_frame_seq(vf.frame_seq());

        std::string bytes;
        if (outEv.SerializeToString(&bytes)) {
            zmq::message_t out(bytes.data(), bytes.size());
            pub.send(out, zmq::send_flags::dontwait);
            fprintf(stdout, "[AIWorker] result: %s conf=%.2f frame=%u\n",
                    ai->label().c_str(), confidence, vf.frame_seq());
        }
    }

    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}