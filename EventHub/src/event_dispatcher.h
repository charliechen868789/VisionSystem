#pragma once
#include "screen_event.pb.h"
#include "cloud_poster.h"
#include "event_logger.h"
#include <string>
#include <unordered_map>
#include <zmq.hpp>
#include <chrono>

class EventDispatcher
{
public:
    EventDispatcher(CloudPoster &poster,
                    EventLogger &logger,
                    const std::unordered_map<std::string, uint32_t> &gpioMap,
                    const std::string &guiReplyHost,
                    uint16_t           guiReplyPort);

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

    std::string protoToJson(const pfas::ScreenEvent &ev);

    CloudPoster  &m_poster;
    EventLogger  &m_logger;
    uint64_t      m_rxCount = 0;
    // Add members:
    zmq::context_t           m_replyCtx;
    zmq::socket_t            m_replySock;
    pfas::SystemInfoPayload  m_lastSystemInfo;   // cache latest
    bool                     m_hasSystemInfo = false;
    const std::unordered_map<std::string, uint32_t> &m_gpioMap;

    void forwardSystemInfo(const pfas::ScreenEvent &ev);
    long long nowMs();
};