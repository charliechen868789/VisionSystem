#include "mini_test.h"

#include "system_config.h"
#include "video_config.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <unistd.h>

namespace {

// Writes `contents` to a fresh temp file and returns its path.
std::string writeTempFile(const std::string &contents) {
    static int counter = 0;
    auto path = std::filesystem::temp_directory_path() /
                ("visionsystem_test_" + std::to_string(::getpid()) + "_" +
                 std::to_string(counter++) + ".json");
    std::ofstream f(path);
    f << contents;
    f.close();
    return path.string();
}

} // namespace

TEST_CASE(SystemConfig_LoadMissingFile_ReturnsFalseAndKeepsDefaults) {
    SystemConfig cfg;
    CHECK(!cfg.load("/nonexistent/path/system.json"));
    CHECK_EQ(cfg.poll_interval_ms, 5000u);
    CHECK_EQ(cfg.pub_port, 9000);
}

TEST_CASE(SystemConfig_LoadValidJson_OverridesDefaults) {
    auto path = writeTempFile(R"({
        "poll_interval_ms": 1234,
        "publisher": { "host": "10.0.0.5", "port": 9099 },
        "thresholds": { "cpu_warn_percent": 50.0, "mem_warn_percent": 60.0, "temp_warn_celsius": 70.0 }
    })");

    SystemConfig cfg;
    CHECK(cfg.load(path));
    CHECK_EQ(cfg.poll_interval_ms, 1234u);
    CHECK_EQ(cfg.pub_host, std::string("10.0.0.5"));
    CHECK_EQ(cfg.pub_port, 9099);
    CHECK(cfg.cpu_warn == 50.0f);
    CHECK(cfg.mem_warn == 60.0f);
    CHECK(cfg.temp_warn == 70.0f);

    std::filesystem::remove(path);
}

TEST_CASE(SystemConfig_LoadMalformedJson_ReturnsFalse) {
    auto path = writeTempFile("{ not valid json ");
    SystemConfig cfg;
    CHECK(!cfg.load(path));
    std::filesystem::remove(path);
}

TEST_CASE(VideoConfig_LoadLegacySource_CreatesSingleCamera) {
    auto path = writeTempFile(R"({
        "source": { "type": "mipi", "device": "/dev/video1", "width": 1920, "height": 1080, "fps": 24 }
    })");

    VideoConfig cfg;
    CHECK(cfg.load(path));
    CHECK_EQ(cfg.cameras.size(), (size_t)1);
    CHECK_EQ(cfg.source_type, std::string("mipi"));
    CHECK_EQ(cfg.device, std::string("/dev/video1"));
    CHECK_EQ(cfg.width, 1920u);
    CHECK_EQ(cfg.height, 1080u);
    CHECK_EQ(cfg.fps, 24u);

    std::filesystem::remove(path);
}

TEST_CASE(VideoConfig_LoadCameraList_SelectsActiveCamera) {
    auto path = writeTempFile(R"({
        "cameras": [
            { "id": 0, "name": "Front", "device": "/dev/video0", "type": "usb", "width": 1280, "height": 720, "fps": 30 },
            { "id": 1, "name": "Rear",  "device": "/dev/video2", "type": "usb", "width": 640,  "height": 480, "fps": 15 }
        ],
        "active_camera": 1
    })");

    VideoConfig cfg;
    CHECK(cfg.load(path));
    CHECK_EQ(cfg.cameras.size(), (size_t)2);
    CHECK_EQ(cfg.active_camera, 1);
    CHECK_EQ(cfg.device, std::string("/dev/video2"));
    CHECK_EQ(cfg.width, 640u);
    CHECK_EQ(cfg.height, 480u);
    CHECK_EQ(cfg.fps, 15u);

    std::filesystem::remove(path);
}

TEST_CASE(VideoConfig_ApplyAction_ResolutionUpdatesActiveCamera) {
    auto path = writeTempFile(R"({
        "cameras": [ { "id": 0, "device": "/dev/video0", "type": "usb", "width": 1280, "height": 720, "fps": 30 } ],
        "active_camera": 0
    })");

    VideoConfig cfg;
    CHECK(cfg.load(path));

    // index 2 in the resolution table == 1280x720
    cfg.applyAction("resolution", "2");
    CHECK_EQ(cfg.width, 1280u);
    CHECK_EQ(cfg.height, 720u);
    CHECK_EQ(cfg.cameras[0].width, 1280u);
    CHECK_EQ(cfg.cameras[0].height, 720u);

    // index 3 in the resolution table == 640x480
    cfg.applyAction("resolution", "3");
    CHECK_EQ(cfg.width, 640u);
    CHECK_EQ(cfg.height, 480u);

    std::filesystem::remove(path);
}

TEST_CASE(VideoConfig_ApplyAction_UnknownActionIsIgnored) {
    auto path = writeTempFile(R"({ "cameras": [ { "id": 0 } ] })");

    VideoConfig cfg;
    CHECK(cfg.load(path));
    auto widthBefore = cfg.width;
    auto heightBefore = cfg.height;

    cfg.applyAction("not_a_real_action", "42");
    CHECK_EQ(cfg.width, widthBefore);
    CHECK_EQ(cfg.height, heightBefore);

    std::filesystem::remove(path);
}

int main() {
    return mini_test::run_all();
}
