#pragma once
#include <QString>

struct GuiDetection {
    QString label;
    float   confidence;
    float   x, y, w, h;  // normalized 0-1
};