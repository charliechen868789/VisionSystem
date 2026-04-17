#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <string>
#include "hub_config.h"
#include "cloud_poster.h"
#include "event_logger.h"
#include "event_dispatcher.h"
#include "worker_manager.h"
#include "hub_client.h"
static volatile bool g_running = true;
static void sigHandler(int) { g_running = false; }

int main(int argc, char *argv[])
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    std::string configPath = "/media/JetsonNan/Peple_Flow/config/hub_config.json";
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--config" && i+1 < argc) configPath = argv[++i];
        else if (a == "--help") {
            fprintf(stdout, "Usage: %s [--config <path>]\n", argv[0]);
            return 0;
        }
    }

    HubConfig cfg;
    cfg.load(configPath);
    cfg.dump();

    signal(SIGINT,  sigHandler);
    signal(SIGTERM, sigHandler);

    // Wire components from config
    CloudPoster::Config cc;
    cc.endpoint   = cfg.cloud_enabled ? cfg.cloud_endpoint : "";
    cc.apiKey     = cfg.cloud_api_key;
    cc.maxRetries = cfg.cloud_retries;
    cc.timeoutSec = cfg.cloud_timeout;

    CloudPoster     poster(cc);
    EventLogger     logger(cfg.log_path);
    EventDispatcher dispatcher(poster, logger, cfg.gpio_map,
                           cfg.sub_host, cfg.gui_reply_port);
    WorkerManager   workers(cfg);

    HubClient client(cfg.sub_host, cfg.sub_port,
        [&dispatcher](const pfas::ScreenEvent &ev) {
            dispatcher.dispatch(ev);
        });

    workers.startAll();
    client.start();

    fprintf(stdout, "[EventHub] running — tcp://%s:%u\n",
            cfg.sub_host.c_str(), cfg.sub_port);

    while (g_running) {
        struct timespec ts{1, 0};
        nanosleep(&ts, nullptr);
        workers.checkHealth();
    }

    fprintf(stdout, "\n[EventHub] shutting down\n");
    client.stop();
    poster.stop();
    workers.stopAll();
    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}

