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

static float readGpuPercent()
{
    // Tegra reports GPU load as a permille value (0-1000), matching what
    // tegrastats itself reads from this file — divide by 10 for percent.
    std::ifstream f("/sys/devices/57000000.gpu/load");
    if (!f.is_open()) return 0.0f;
    float raw = 0; f >> raw;
    return raw / 10.0f;
}

// Reads the first wlanN interface's signal level (dBm) from
// /proc/net/wireless and maps it to a 0-100% quality figure using the
// same linear scale NetworkManager/wpa_supplicant use (-100dBm=0%,
// -50dBm=100%). Returns false (percentOut left at 0) if no wlan interface
// is currently associated — that's the normal state on this board, which
// runs over Ethernet.
static bool readWifiSignal(float &percentOut)
{
    percentOut = 0.0f;
    std::ifstream f("/proc/net/wireless");
    std::string line;
    std::getline(f, line);   // header line 1
    std::getline(f, line);   // header line 2

    while (std::getline(f, line)) {
        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;

        std::string iface = line.substr(0, colon);
        size_t start = iface.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        iface = iface.substr(start);
        if (iface.rfind("wlan", 0) != 0) continue;

        std::istringstream iss(line.substr(colon + 1));
        std::string status, link, level;
        iss >> status >> link >> level;
        if (level.empty()) continue;
        while (!level.empty() && level.back() == '.') level.pop_back();

        float dBm;
        try { dBm = std::stof(level); } catch (...) { continue; }
        if (dBm >= 0.0f) continue;   // not a real reading — not associated

        if      (dBm <= -100.0f) percentOut = 0.0f;
        else if (dBm >= -50.0f)  percentOut = 100.0f;
        else                     percentOut = 2.0f * (dBm + 100.0f);
        return true;
    }
    return false;
}

int main(int argc, char *argv[])
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    std::string configPath = "/etc/aeroboard/system.json";
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
        float gpu  = readGpuPercent();
        float wifiSignal    = 0.0f;
        bool  wifiConnected = readWifiSignal(wifiSignal);

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
        si->set_gpu_percent(gpu);
        si->set_wifi_signal_percent(wifiSignal);
        si->set_wifi_connected(wifiConnected);

        std::string bytes;
        if (ev.SerializeToString(&bytes)) {
            zmq::message_t msg(bytes.data(), bytes.size());
            sock.send(msg, zmq::send_flags::dontwait);
        }

        fprintf(stdout,
                "[SystemWorker] cpu=%.1f%% mem=%.1f%% gpu=%.1f%% temp=%.1f°C wifi=%.0f%%%s\n",
                cpu, mem, gpu, temp, wifiSignal, wifiConnected ? "" : " (disconnected)");
        std::this_thread::sleep_for(std::chrono::milliseconds(cfg.poll_interval_ms));
    }

    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}