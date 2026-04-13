#include "event_dispatcher.h"
#include <google/protobuf/util/json_util.h>
#include <cstdio>
#include <ctime>

EventDispatcher::EventDispatcher(CloudPoster &poster, EventLogger &logger)
    : m_poster(poster), m_logger(logger) {}

void EventDispatcher::dispatch(const pfas::ScreenEvent &ev)
{
    ++m_rxCount;
    m_logger.logRaw(ev);

    switch (ev.event_type()) {
        case pfas::DWELL_ALERT:    onDwellAlert(ev);    break;
        case pfas::TRACK_UPDATE:   onTrackUpdate(ev);   break;
        case pfas::PAGE_CHANGED:   onPageChanged(ev);   break;
        case pfas::CONTROL_ACTION: onControlAction(ev); break;
        case pfas::CALIB_UPDATED:  onCalibUpdated(ev);  break;
        case pfas::SYSTEM_STATUS:  onSystemStatus(ev);  break;
        default:
            fprintf(stderr, "[Dispatcher] unknown event type %d\n", ev.event_type());
    }
}

// -- handlers -----------------------------------------------------------------

void EventDispatcher::onDwellAlert(const pfas::ScreenEvent &ev)
{
    const auto &da = ev.dwell_alert();
    fprintf(stdout,
        "[DWELL_ALERT] ts=%llu  track=#%u  zone=%s  "
        "world=(%.2fm,%.2fm)  time=%s\n",
        (unsigned long long)ev.timestamp_ms(),
        da.track_id(), da.zone().c_str(),
        da.location().x(), da.location().y(),
        da.iso_time().c_str());

    // POST to cloud as JSON (REST APIs usually prefer JSON over proto)
    m_poster.post("DWELL_ALERT", protoToJson(ev));
    m_logger.logDwellAlert(da);
}

void EventDispatcher::onTrackUpdate(const pfas::ScreenEvent &ev)
{
    const auto &tu = ev.track_update();
    fprintf(stdout, "[TRACK_UPDATE] frame=%u  persons=%d\n",
            tu.frame_number(), tu.persons_size());
    // Track updates are high-frequency — log only, don't POST to cloud
    m_logger.logTrackUpdate(tu);
}

void EventDispatcher::onPageChanged(const pfas::ScreenEvent &ev)
{
    fprintf(stdout, "[PAGE_CHANGED] ? %s\n", ev.page_changed().c_str());
}

void EventDispatcher::onControlAction(const pfas::ScreenEvent &ev)
{
    const auto &ca = ev.control_action();
    fprintf(stdout, "[CONTROL] %s = %s\n",
            ca.action().c_str(), ca.value().c_str());
}

void EventDispatcher::onCalibUpdated(const pfas::ScreenEvent &ev)
{
    fprintf(stdout, "[CALIB_UPDATED] %d points\n",
            ev.calib_updated().point_count());
    m_poster.post("CALIB_UPDATED", protoToJson(ev));
}

void EventDispatcher::onSystemStatus(const pfas::ScreenEvent &ev)
{
    fprintf(stdout, "[STATUS] %s\n",
            ev.system_status().message().c_str());
}

// -- protobuf ? JSON for REST POST --------------------------------------------

std::string EventDispatcher::protoToJson(const pfas::ScreenEvent &ev)
{
    std::string json;
    google::protobuf::util::JsonPrintOptions opts;
    opts.add_whitespace              = false;
    opts.always_print_primitive_fields = true;
    google::protobuf::util::MessageToJsonString(ev, &json, opts);
    return json;
}