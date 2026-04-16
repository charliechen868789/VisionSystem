#include "event_dispatcher.h"
#include <google/protobuf/util/json_util.h>
#include <cstdio>

EventDispatcher::EventDispatcher(CloudPoster &poster,
                                 EventLogger &logger,
                                 const std::unordered_map<std::string, uint32_t> &gpioMap)
    : m_poster(poster), m_logger(logger), m_gpioMap(gpioMap) {}

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
    case pfas::VIDEO_FRAME:    onVideoFrame(ev);    break;
    case pfas::SENSOR_DATA:    onSensorData(ev);    break;
    case pfas::SYSTEM_INFO:    onSystemInfo(ev);    break;
    case pfas::AI_RESULT:      onAiResult(ev);      break;
    default:
        fprintf(stderr, "[Dispatcher] unknown event %d\n", ev.event_type());
    }
}

void EventDispatcher::onControlAction(const pfas::ScreenEvent &ev)
{
    const auto &ca = ev.control_action();
    bool value = (ca.value() == "true");

    fprintf(stdout, "[CONTROL_ACTION] action=%s  value=%s\n",
            ca.action().c_str(), ca.value().c_str());

    // Log which GPIO number this action maps to (no sysfs write — GPIO handled elsewhere)
    auto it = m_gpioMap.find(ca.action());
    if (it != m_gpioMap.end()) {
        fprintf(stdout, "[CONTROL_ACTION] mapped to GPIO%u -> %s\n",
                it->second, value ? "HIGH" : "LOW");
    }

    m_logger.logControlAction(ca, value);
}

void EventDispatcher::onDwellAlert(const pfas::ScreenEvent &ev)
{
    const auto &da = ev.dwell_alert();
    fprintf(stdout, "[DWELL_ALERT] ts=%llu track=#%u zone=%s world=(%.2f,%.2f)\n",
            (unsigned long long)ev.timestamp_ms(),
            da.track_id(), da.zone().c_str(),
            da.location().x(), da.location().y());
    m_poster.post("DWELL_ALERT", protoToJson(ev));
    m_logger.logDwellAlert(da);
}

void EventDispatcher::onTrackUpdate(const pfas::ScreenEvent &ev)
{
    const auto &tu = ev.track_update();
    fprintf(stdout, "[TRACK_UPDATE] frame=%u persons=%d\n",
            tu.frame_number(), tu.persons_size());
    m_logger.logTrackUpdate(tu);
}

void EventDispatcher::onPageChanged(const pfas::ScreenEvent &ev)
{
    fprintf(stdout, "[PAGE_CHANGED] -> %s\n", ev.page_changed().c_str());
}

void EventDispatcher::onCalibUpdated(const pfas::ScreenEvent &ev)
{
    fprintf(stdout, "[CALIB_UPDATED] %d points\n",
            ev.calib_updated().point_count());
    m_poster.post("CALIB_UPDATED", protoToJson(ev));
}

void EventDispatcher::onSystemStatus(const pfas::ScreenEvent &ev)
{
    fprintf(stdout, "[SYSTEM_STATUS] %s\n",
            ev.system_status().message().c_str());
}

void EventDispatcher::onVideoFrame(const pfas::ScreenEvent &ev)
{
    const auto &vf = ev.video_frame();
    fprintf(stdout, "[VIDEO_FRAME] seq=%u  %ux%u  %zu bytes\n",
            vf.frame_seq(), vf.width(), vf.height(),
            vf.jpeg_data().size());
}

void EventDispatcher::onSensorData(const pfas::ScreenEvent &ev)
{
    const auto &s = ev.sensor_data();
    fprintf(stdout, "[SENSOR] id=%s  %.3f %s  @ %s\n",
            s.sensor_id().c_str(), s.value(),
            s.unit().c_str(), s.iso_time().c_str());
    m_logger.logSensorData(s);
}

void EventDispatcher::onSystemInfo(const pfas::ScreenEvent &ev)
{
    const auto &si = ev.system_info();
    fprintf(stdout, "[SYSTEM_INFO] cpu=%.1f%%  mem=%.1f%%  temp=%.1f°C  up=%s\n",
            si.cpu_percent(), si.mem_percent(),
            si.temp_celsius(), si.uptime().c_str());
    m_logger.logSystemInfo(si);
}

void EventDispatcher::onAiResult(const pfas::ScreenEvent &ev)
{
    const auto &ai = ev.ai_result();
    fprintf(stdout, "[AI_RESULT] model=%s  label=%s  conf=%.3f  frame=%u\n",
            ai.model().c_str(), ai.label().c_str(),
            ai.confidence(), ai.frame_seq());
    m_logger.logAiResult(ai);
}

std::string EventDispatcher::protoToJson(const pfas::ScreenEvent &ev)
{
    std::string json;
    google::protobuf::util::JsonPrintOptions opts;
    opts.add_whitespace                = false;
    opts.always_print_primitive_fields = true;
    google::protobuf::util::MessageToJsonString(ev, &json, opts);
    return json;
}