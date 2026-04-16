#include <cstdio>
#include <csignal>
#include <string>
#include "video_config.h"
#include "frame_reader.h"
#include "video_publisher.h"

static volatile bool g_running = true;
static void sigHandler(int) { g_running = false; }

int main(int argc, char *argv[])
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    std::string configPath = "/etc/aeroboard/video.json";
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--config" && i+1 < argc) configPath = argv[++i];
    }

    VideoConfig cfg;
    cfg.load(configPath);

    signal(SIGINT, sigHandler); signal(SIGTERM, sigHandler);

    VideoPublisher pub(cfg.pub_host, cfg.pub_port);

    FrameReader reader(cfg, [&pub](const pfas::ScreenEvent &ev) {
        pub.publish(ev);
    });

    reader.start();
    fprintf(stdout, "[VideoWorker] running\n");

    while (g_running) { struct timespec ts{1,0}; nanosleep(&ts,nullptr); }

    reader.stop();
    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}