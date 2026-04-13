#pragma once
#include <string>
#include <cstdio>
#include "screen_event.pb.h"

class EventLogger
{
public:
    explicit EventLogger(const std::string &path);
    ~EventLogger();

    void logRaw        (const pfas::ScreenEvent &ev);
    void logDwellAlert (const pfas::DwellAlertPayload &da);
    void logTrackUpdate(const pfas::TrackUpdatePayload &tu);

private:
    FILE *m_fp = nullptr;
    void write(const char *line);
};