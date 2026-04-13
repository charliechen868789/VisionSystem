#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <string>

#include "hub_client.h"
#include "cloud_poster.h"
#include "event_logger.h"
#include "event_dispatcher.h"

static volatile bool g_running = true;

static void sigHandler(int) { g_running = false; }

static void usage(const char *prog)
{
    fprintf(stderr,
        "Usage: %s [options]\n"
        "  --host     <host>   Qt publisher host     (default: 127.0.0.1)\n"
        "  --port     <port>   Qt publisher port     (default: 9000)\n"
        "  --endpoint <url>    Cloud POST endpoint   (default: http://localhost:8080/api/events)\n"
        "  --api-key  <key>    Authorization Bearer key\n"
        "  --log      <path>   Log file path         (default: /var/log/eventhub.log)\n"
        "  --no-post           Disable cloud POST\n",
        prog);
}

int main(int argc, char *argv[])
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    // -- CLI parsing ----------------------------------------------------------
    std::string host     = "127.0.0.1";
    uint16_t    port     = 9000;
    std::string endpoint = "http://localhost:8080/api/events";
    std::string apiKey;
    std::string logPath  = "/var/log/eventhub.log";
    bool        noPost   = false;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if      (a == "--host"     && i+1<argc) host     = argv[++i];
        else if (a == "--port"     && i+1<argc) port     = (uint16_t)atoi(argv[++i]);
        else if (a == "--endpoint" && i+1<argc) endpoint = argv[++i];
        else if (a == "--api-key"  && i+1<argc) apiKey   = argv[++i];
        else if (a == "--log"      && i+1<argc) logPath  = argv[++i];
        else if (a == "--no-post")              noPost   = true;
        else if (a == "--help")                 { usage(argv[0]); return 0; }
    }

    signal(SIGINT,  sigHandler);
    signal(SIGTERM, sigHandler);

    // -- Wire up components ---------------------------------------------------
    CloudPoster::Config cfg;
    cfg.endpoint   = endpoint;
    cfg.apiKey     = apiKey;
    cfg.maxRetries = 3;
    cfg.timeoutSec = 10;
    if (noPost) cfg.endpoint = "";   // poster will skip if endpoint empty

    CloudPoster    poster(cfg);
    EventLogger    logger(logPath);
    EventDispatcher dispatcher(poster, logger);

    HubClient client(host, port,
        [&dispatcher](const pfas::ScreenEvent &ev)
        {
            dispatcher.dispatch(ev);
        });

    // -- Run ------------------------------------------------------------------
    fprintf(stdout,
        "EventHub starting\n"
        "  ? subscriber : tcp://%s:%u\n"
        "  ? cloud POST  : %s\n"
        "  ? log file    : %s\n",
        host.c_str(), port,
        noPost ? "(disabled)" : endpoint.c_str(),
        logPath.c_str());

    client.start();

    // Block main thread until SIGINT/SIGTERM
    while (g_running) {
        struct timespec ts{1, 0};
        nanosleep(&ts, nullptr);
    }

    fprintf(stdout, "\nEventHub shutting down…\n");
    client.stop();
    poster.stop();

    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}