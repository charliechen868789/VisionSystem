#include <cstdio>
#include <csignal>
#include <string>
#include <thread>
#include <chrono>
#include <fstream>
#include "sensor_config.h"
#include "sensor_publisher.h"

static volatile bool g_running = true;
static void sigHandler(int) { g_running = false; }

static float readSysfsTemp(const std::string &path)
{
    std::ifstream f(path);
    float raw = 0;
    if (f) { f >> raw; }
    return raw / 1000.0f;  // millidegrees → degrees C
}

int main(int argc, char *argv[])
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    std::string configPath = "/media/JetsonNan/Peple_Flow/config/sensor.json";
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--config" && i+1 < argc) configPath = argv[++i];
    }

    SensorConfig cfg;
    cfg.load(configPath);

    signal(SIGINT, sigHandler); signal(SIGTERM, sigHandler);

    SensorPublisher pub(cfg.pub_host, cfg.pub_port);
    fprintf(stdout, "[SensorWorker] running %zu sensors\n", cfg.sensors.size());

    while (g_running) {
        for (const auto &s : cfg.sensors) {
            if (s.type == "temperature") {
                float temp = readSysfsTemp(s.device);
                pub.publishSensor(s.id, temp, "°C", "");
            }
            // extend here for IMU, ADC, etc.
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(
            cfg.sensors.empty() ? 2000 : cfg.sensors[0].interval_ms));
    }

    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}