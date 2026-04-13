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
    snprintf(buf, sizeof(buf),
             "[RAW] ts=%llu type=%d bytes=<proto>",
             (unsigned long long)ev.timestamp_ms(), ev.event_type());
    write(buf);
}

void EventLogger::logDwellAlert(const pfas::DwellAlertPayload &da)
{
    char buf[256];
    snprintf(buf, sizeof(buf),
             "[DWELL_ALERT] track=#%u zone=%s world=(%.2f,%.2f) time=%s",
             da.track_id(), da.zone().c_str(),
             da.location().x(), da.location().y(),
             da.iso_time().c_str());
    write(buf);
}

void EventLogger::logTrackUpdate(const pfas::TrackUpdatePayload &tu)
{
    char buf[128];
    snprintf(buf, sizeof(buf),
             "[TRACK_UPDATE] frame=%u persons=%d",
             tu.frame_number(), tu.persons_size());
    write(buf);
}