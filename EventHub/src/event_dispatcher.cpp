#include "event_dispatcher.h"
#include <google/protobuf/util/json_util.h>
#include <cstdio>

EventDispatcher::EventDispatcher(CloudPoster &poster,
                                 EventLogger &logger,
                                 const std::unordered_map<std::string,uint32_t> &gpioMap,
                                 const std::string &guiReplyHost,
                                 uint16_t guiReplyPort)
    : m_poster(poster), m_logger(logger), m_gpioMap(gpioMap)
    , m_replyCtx(1)
    , m_replySock(m_replyCtx, zmq::socket_type::pub)
{
    std::string ep = "tcp://" + guiReplyHost + ":" + std::to_string(guiReplyPort);
    m_replySock.bind(ep);
    fprintf(stdout, "[Dispatcher] GUI reply socket bound to %s\n", ep.c_str());

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

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

long long EventDispatcher::nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

// Update onControlAction to reply on demand:
void EventDispatcher::onControlAction(const pfas::ScreenEvent &ev)
{
    const auto &ca = ev.control_action();
    bool value = (ca.value() == "true");

    fprintf(stdout, "[CONTROL_ACTION] action=%s  value=%s\n",
            ca.action().c_str(), ca.value().c_str());

    if (ca.action() == "get_system_info") {
        if (m_hasSystemInfo) {
            // Reply with cached system info
            pfas::ScreenEvent reply;
            reply.set_timestamp_ms(nowMs());
            reply.set_event_type(pfas::SYSTEM_INFO);
            *reply.mutable_system_info() = m_lastSystemInfo;
            forwardSystemInfo(reply);
            fprintf(stdout, "[Dispatcher] replied system_info to GUI\n");
        } else {
            fprintf(stdout, "[Dispatcher] system_info not yet available\n");
        }
        return;
    }

    auto it = m_gpioMap.find(ca.action());
    if (it != m_gpioMap.end()) {
        fprintf(stdout, "[CONTROL_ACTION] mapped to GPIO%u -> %s\n",
                it->second, value ? "HIGH" : "LOW");
    }
    m_logger.logControlAction(ca, value);
}

// Add helper:
void EventDispatcher::forwardSystemInfo(const pfas::ScreenEvent &ev)
{
    std::string bytes;
    if (!ev.SerializeToString(&bytes)) return;
    zmq::message_t msg(bytes.data(), bytes.size());
    try {
        m_replySock.send(msg, zmq::send_flags::dontwait);
    } catch (const zmq::error_t &e) {
        fprintf(stderr, "[Dispatcher] reply send failed: %s\n", e.what());
    }
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

// Update onSystemInfo to cache the latest reading:
void EventDispatcher::onSystemInfo(const pfas::ScreenEvent &ev)
{
    const auto &si = ev.system_info();
    fprintf(stdout, "[SYSTEM_INFO] cpu=%.1f%%  mem=%.1f%%  temp=%.1f°C\n",
            si.cpu_percent(), si.mem_percent(), si.temp_celsius());
    m_logger.logSystemInfo(si);

    // Cache it
    m_lastSystemInfo  = si;
    m_hasSystemInfo   = true;

    // Always forward to GUI immediately when new data arrives
    forwardSystemInfo(ev);
}