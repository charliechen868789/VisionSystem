#include "event_logger.h"
#include <ctime>
#include <cstring>

EventLogger::EventLogger(const std::string &path)
{
    m_fp = fopen(path.c_str(), "a");
    if (!m_fp) fprintf(stderr, "[EventLogger] cannot open %s\n", path.c_str());
}

EventLogger::~EventLogger() { if (m_fp) fclose(m_fp); }

void EventLogger::write(const char *line)
{
    if (!m_fp) return;
    fputs(line, m_fp);
    fputc('\n', m_fp);
    fflush(m_fp);
}

void EventLogger::logRaw(const pfas::ScreenEvent &ev)
{
    char buf[128];
    snprintf(buf, sizeof(buf), "[RAW] ts=%llu type=%d",
             (unsigned long long)ev.timestamp_ms(), ev.event_type());
    write(buf);
}

void EventLogger::logDwellAlert(const pfas::DwellAlertPayload &da)
{
    char buf[256];
    snprintf(buf, sizeof(buf),
             "[DWELL_ALERT] track=#%u zone=%s world=(%.2f,%.2f) time=%s",
             da.track_id(), da.zone().c_str(),
             da.location().x(), da.location().y(), da.iso_time().c_str());
    write(buf);
}

void EventLogger::logTrackUpdate(const pfas::TrackUpdatePayload &tu)
{
    char buf[128];
    snprintf(buf, sizeof(buf), "[TRACK_UPDATE] frame=%u persons=%d",
             tu.frame_number(), tu.persons_size());
    write(buf);
}

void EventLogger::logControlAction(const pfas::ControlActionPayload &ca, bool gpioState)
{
    char buf[256];
    snprintf(buf, sizeof(buf), "[CONTROL_ACTION] action=%s value=%s gpio=%s",
             ca.action().c_str(), ca.value().c_str(),
             gpioState ? "HIGH" : "LOW");
    write(buf);
}

void EventLogger::logSensorData(const pfas::SensorPayload &s)
{
    char buf[256];
    snprintf(buf, sizeof(buf), "[SENSOR] id=%s value=%.3f unit=%s time=%s",
             s.sensor_id().c_str(), s.value(),
             s.unit().c_str(), s.iso_time().c_str());
    write(buf);
}

void EventLogger::logSystemInfo(const pfas::SystemInfoPayload &si)
{
    char buf[256];
    snprintf(buf, sizeof(buf),
             "[SYSTEM_INFO] cpu=%.1f%% mem=%.1f%% temp=%.1fC up=%s",
             si.cpu_percent(), si.mem_percent(),
             si.temp_celsius(), si.uptime().c_str());
    write(buf);
}

void EventLogger::logAiResult(const pfas::AiResultPayload &ai)
{
    char buf[256];
    snprintf(buf, sizeof(buf),
             "[AI_RESULT] model=%s label=%s conf=%.3f frame=%u",
             ai.model().c_str(), ai.label().c_str(),
             ai.confidence(), ai.frame_seq());
    write(buf);
}