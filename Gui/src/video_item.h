#pragma once
#include <QQuickPaintedItem>
#include <QImage>
#include <QMutex>
#include <QString>
#include "gui_types.h"

class VideoItem : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(bool active READ active NOTIFY activeChanged)

public:
    explicit VideoItem(QQuickItem *parent = nullptr);

    bool active() const { return m_active; }

    // Called from Backend — safe to call from any thread
    void setFrame    (const QByteArray &jpeg, uint32_t frameSeq);
    void setAiResult (const QString &label, double confidence, uint32_t frameSeq);
    void setDetections(const QList<GuiDetection> &dets, uint32_t frameSeq);
    void paint(QPainter *painter) override;

signals:
    void activeChanged();

private:
    QMutex   m_mutex;
    QImage   m_frame;
    uint32_t m_frameSeq   = 0;
    uint32_t m_aiFrameSeq = 0;
    QString  m_aiLabel;
    double   m_aiConf     = 0.0;
    bool     m_active     = false;
    QList<GuiDetection> m_detections;
};