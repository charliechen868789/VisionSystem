#include <cstdio>
#include <csignal>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <chrono>
#include <zmq.hpp>
#include "system_config.h"
#include "screen_event.pb.h"

static volatile bool g_running = true;
static void sigHandler(int) { g_running = false; }

static uint64_t nowMs() {
    using namespace std::chrono;
    return (uint64_t)duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count();
}

static float readCpuPercent()
{
    // Read /proc/stat, compute delta idle/total
    static long prev_idle = 0, prev_total = 0;
    std::ifstream f("/proc/stat");
    std::string label;
    long u,n,s,i,w,hi,si,st;
    f >> label >> u >> n >> s >> i >> w >> hi >> si >> st;
    long idle  = i + w;
    long total = u + n + s + i + w + hi + si + st;
    float cpu  = 0;
    if (total - prev_total > 0)
        cpu = 100.0f * (1.0f - (float)(idle - prev_idle) / (float)(total - prev_total));
    prev_idle = idle; prev_total = total;
    return cpu;
}

static float readMemPercent()
{
    std::ifstream f("/proc/meminfo");
    long total = 0, available = 0;
    std::string key; long val; std::string unit;
    while (f >> key >> val >> unit) {
        if (key == "MemTotal:")     total     = val;
        if (key == "MemAvailable:") available = val;
    }
    return total > 0 ? 100.0f * (1.0f - (float)available / (float)total) : 0;
}

static float readTemp()
{
    std::ifstream f("/sys/class/thermal/thermal_zone0/temp");
    float raw = 0; f >> raw;
    return raw / 1000.0f;
}

int main(int argc, char *argv[])
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    std::string configPath = "/media/JetsonNan/Peple_Flow/config/system.json";
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--config" && i+1 < argc) configPath = argv[++i];
    }

    SystemConfig cfg;
    cfg.load(configPath);
    signal(SIGINT, sigHandler); signal(SIGTERM, sigHandler);

    zmq::context_t ctx(1);
    zmq::socket_t  sock(ctx, zmq::socket_type::pub);
    std::string ep = "tcp://" + cfg.pub_host + ":" + std::to_string(cfg.pub_port);
    sock.bind(ep);
    fprintf(stdout, "[SystemWorker] running — publishing to %s\n", ep.c_str());

    while (g_running) {
        float cpu  = readCpuPercent();
        float mem  = readMemPercent();
        float temp = readTemp();

        if (cpu  > cfg.cpu_warn)  fprintf(stderr, "[SystemWorker] CPU  warning: %.1f%%\n", cpu);
        if (mem  > cfg.mem_warn)  fprintf(stderr, "[SystemWorker] MEM  warning: %.1f%%\n", mem);
        if (temp > cfg.temp_warn) fprintf(stderr, "[SystemWorker] TEMP warning: %.1f°C\n", temp);

        pfas::ScreenEvent ev;
        ev.set_timestamp_ms(nowMs());
        ev.set_event_type(pfas::SYSTEM_INFO);
        auto *si = ev.mutable_system_info();
        si->set_cpu_percent(cpu);
        si->set_mem_percent(mem);
        si->set_temp_celsius(temp);

        std::string bytes;
        if (ev.SerializeToString(&bytes)) {
            zmq::message_t msg(bytes.data(), bytes.size());
            sock.send(msg, zmq::send_flags::dontwait);
        }

        fprintf(stdout, "[SystemWorker] cpu=%.1f%% mem=%.1f%% temp=%.1f°C\n",
                cpu, mem, temp);
        std::this_thread::sleep_for(std::chrono::milliseconds(cfg.poll_interval_ms));
    }

    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}