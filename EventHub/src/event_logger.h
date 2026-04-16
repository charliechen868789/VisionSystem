#pragma once
#include "screen_event.pb.h"
#include <cstdio>
#include <string>

class EventLogger
{
public:
    explicit EventLogger(const std::string &path);
    ~EventLogger();

    void logRaw          (const pfas::ScreenEvent          &ev);
    void logDwellAlert   (const pfas::DwellAlertPayload    &da);
    void logTrackUpdate  (const pfas::TrackUpdatePayload   &tu);
    void logControlAction(const pfas::ControlActionPayload &ca, bool gpioState);
    void logSensorData   (const pfas::SensorPayload        &s);
    void logSystemInfo   (const pfas::SystemInfoPayload    &si);
    void logAiResult     (const pfas::AiResultPayload      &ai);

private:
    void  write(const char *line);
    FILE *m_fp = nullptr;
};