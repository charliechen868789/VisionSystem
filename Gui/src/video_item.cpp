#include "video_item.h"
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QMetaObject>

VideoItem::VideoItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setRenderTarget(QQuickPaintedItem::Image);
    setAntialiasing(false);
}

void VideoItem::setFrame(const QByteArray &jpeg, uint32_t frameSeq)
{
    if (jpeg.isEmpty()) return;

    QImage img;
    if (!img.loadFromData(reinterpret_cast<const uchar*>(jpeg.constData()),
                          jpeg.size(), "JPEG")) {
        qWarning() << "[VideoItem] JPEG decode failed size=" << jpeg.size();
        return;
    }

    {
        QMutexLocker lk(&m_mutex);
        m_frame    = img;
        m_frameSeq = frameSeq;
    }

    if (!m_active) {
        m_active = true;
        // emit must be on UI thread
        QMetaObject::invokeMethod(this, "activeChanged", Qt::QueuedConnection);
    }

    // Schedule repaint on UI thread — thread-safe
    QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
}

void VideoItem::setAiResult(const QString &label,
                             double confidence,
                             uint32_t frameSeq)
{
    {
        QMutexLocker lk(&m_mutex);
        m_aiLabel    = label;
        m_aiConf     = confidence;
        m_aiFrameSeq = frameSeq;
    }
    QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
}

void VideoItem::setDetections(const QList<GuiDetection> &dets, uint32_t frameSeq)
{
    QMutexLocker lk(&m_mutex);
    m_detections = dets;
    m_aiFrameSeq = frameSeq;
    QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
}

void VideoItem::paint(QPainter *painter)
{
    QMutexLocker lk(&m_mutex);
    if (m_frame.isNull()) { /* placeholder */ return; }

    QRect target = boundingRect().toRect();
    if (target.isEmpty()) return;

    QImage scaled = m_frame.scaled(target.size(),
                                   Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation);
    int xOff = (target.width()  - scaled.width())  / 2;
    int yOff = (target.height() - scaled.height()) / 2;
    painter->drawImage(xOff, yOff, scaled);

    // ── Draw detection boxes ──────────────────────────────────────────────────
    if (!m_detections.isEmpty()) {
        QFont labelFont("monospace", 10, QFont::Bold);
        painter->setFont(labelFont);
        QFontMetrics fm(labelFont);

        // Color palette per class
        static const QColor kColors[] = {
            {255, 80,  80},   // red
            {80,  255, 80},   // green
            {80,  80,  255},  // blue
            {255, 255, 80},   // yellow
            {255, 80,  255},  // magenta
            {80,  255, 255},  // cyan
            {255, 160, 80},   // orange
            {160, 80,  255},  // purple
        };
        constexpr int kColorCount = 8;

        for (const auto &d : m_detections) {
            // Map normalized coords to screen coords
            int bx = xOff + (int)(d.x * scaled.width());
            int by = yOff + (int)(d.y * scaled.height());
            int bw = (int)(d.w * scaled.width());
            int bh = (int)(d.h * scaled.height());

            // Pick color based on label hash
            int colorIdx = qHash(d.label) % kColorCount;
            QColor boxColor = kColors[colorIdx];

            // Draw bounding box
            painter->setPen(QPen(boxColor, 2));
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(bx, by, bw, bh);

            // Label badge
            QString tag = QString("%1 %2%")
                          .arg(d.label)
                          .arg((int)(d.confidence * 100));

            QRect textRect = fm.boundingRect(tag);
            textRect.adjust(-4, -2, 4, 2);
            textRect.moveTopLeft(QPoint(bx, by - textRect.height() - 2));

            // Keep badge inside frame
            if (textRect.top() < yOff)
                textRect.moveTop(by + 2);

            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(boxColor.red(),
                                     boxColor.green(),
                                     boxColor.blue(), 200));
            painter->drawRoundedRect(textRect, 3, 3);

            painter->setPen(Qt::black);
            painter->drawText(textRect, Qt::AlignCenter, tag);
        }
    }

    // Frame counter
    painter->setPen(QColor(255, 255, 255, 60));
    painter->setFont(QFont("monospace", 8));
    painter->drawText(xOff + 8,
                      yOff + scaled.height() - 8,
                      QString("seq:%1  det:%2")
                          .arg(m_frameSeq)
                          .arg(m_detections.size()));
}