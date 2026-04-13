#pragma once
#include "screen_event.pb.h"
#include "cloud_poster.h"
#include "event_logger.h"
#include <string>

// Routes every ScreenEvent to the right handler
class EventDispatcher
{
public:
    EventDispatcher(CloudPoster &poster, EventLogger &logger);

    void dispatch(const pfas::ScreenEvent &ev);

private:
    void onDwellAlert   (const pfas::ScreenEvent &ev);
    void onTrackUpdate  (const pfas::ScreenEvent &ev);
    void onPageChanged  (const pfas::ScreenEvent &ev);
    void onControlAction(const pfas::ScreenEvent &ev);
    void onCalibUpdated (const pfas::ScreenEvent &ev);
    void onSystemStatus (const pfas::ScreenEvent &ev);

    std::string protoToJson(const pfas::ScreenEvent &ev);

    CloudPoster  &m_poster;
    EventLogger  &m_logger;
    uint64_t      m_rxCount = 0;
};