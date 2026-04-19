#pragma once
#include "screen_event.pb.h"
#include "cloud_poster.h"
#include "event_logger.h"
#include "settings_forwarder.h"
#include "hub_config.h"
#include <zmq.hpp>
#include <string>
#include <unordered_map>
#include <chrono>
#include <cstdint>

class EventDispatcher
{
public:
    EventDispatcher(CloudPoster  &poster,
                    EventLogger  &logger,
                    const std::unordered_map<std::string, uint32_t> &gpioMap,
                    const HubConfig &cfg);
    ~EventDispatcher();

    void dispatch(const pfas::ScreenEvent &ev);

private:
    void onDwellAlert   (const pfas::ScreenEvent &ev);
    void onTrackUpdate  (const pfas::ScreenEvent &ev);
    void onPageChanged  (const pfas::ScreenEvent &ev);
    void onControlAction(const pfas::ScreenEvent &ev);
    void onCalibUpdated (const pfas::ScreenEvent &ev);
    void onSystemStatus (const pfas::ScreenEvent &ev);
    void onVideoFrame   (const pfas::ScreenEvent &ev);
    void onSensorData   (const pfas::ScreenEvent &ev);
    void onSystemInfo   (const pfas::ScreenEvent &ev);
    void onAiResult     (const pfas::ScreenEvent &ev);

    void forwardToGui     (const pfas::ScreenEvent &ev);  // :9005
    void forwardVideoToGui(const pfas::ScreenEvent &ev);  // :9006
    void forwardAiToGui   (const pfas::ScreenEvent &ev);  // :9007

    std::string      protoToJson(const pfas::ScreenEvent &ev);
    static uint64_t  nowMs();

    CloudPoster       &m_poster;
    EventLogger       &m_logger;
    SettingsForwarder  m_settings;

    const std::unordered_map<std::string, uint32_t> &m_gpioMap;

    zmq::context_t m_replyCtx;
    zmq::socket_t  m_replySock;        // :9005
    zmq::socket_t  m_videoReplySock;   // :9006
    zmq::socket_t  m_aiReplySock;      // :9007

    pfas::SystemInfoPayload m_lastSystemInfo;
    bool                    m_hasSystemInfo = false;
    uint64_t                m_rxCount       = 0;
};