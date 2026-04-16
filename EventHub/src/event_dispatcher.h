#pragma once
#include "screen_event.pb.h"
#include "cloud_poster.h"
#include "event_logger.h"
#include <string>
#include <unordered_map>

class EventDispatcher
{
public:
    EventDispatcher(CloudPoster &poster,
                    EventLogger &logger,
                    const std::unordered_map<std::string, uint32_t> &gpioMap);

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

    const std::unordered_map<std::string, uint32_t> &m_gpioMap;
};