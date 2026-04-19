#include "video_item.h"
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QMetaObject>

VideoItem::VideoItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setRenderTarget(QQuickPaintedItem::FramebufferObject);
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

void VideoItem::paint(QPainter *painter)
{
    QMutexLocker lk(&m_mutex);

    if (m_frame.isNull()) {
        painter->fillRect(boundingRect().toRect(), QColor("#0d1530"));
        painter->setPen(QColor("#1e3050"));
        painter->setFont(QFont("monospace", 12));
        painter->drawText(boundingRect().toRect(),
                          Qt::AlignCenter, "No Signal");
        return;
    }

    // Draw scaled JPEG frame
    QRect target = boundingRect().toRect();
    if (target.isEmpty()) return;

    QImage scaled = m_frame.scaled(target.size(),
                                   Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation);

    int xOff = (target.width()  - scaled.width())  / 2;
    int yOff = (target.height() - scaled.height()) / 2;
    painter->drawImage(xOff, yOff, scaled);

    // AI overlay
    bool aiRecent = (m_frameSeq > 0) &&
                    (m_frameSeq <= m_aiFrameSeq + 5);

    if (aiRecent && !m_aiLabel.isEmpty() && m_aiConf > 0.0) {
        QString tag = QString("%1  %2%")
                      .arg(m_aiLabel)
                      .arg((int)(m_aiConf * 100));

        QFont font("monospace", 11, QFont::Bold);
        painter->setFont(font);
        QFontMetrics fm(font);
        QRect textRect = fm.boundingRect(tag);
        textRect.adjust(-8, -4, 8, 4);
        textRect.moveTopLeft(QPoint(xOff + 12, yOff + 12));

        QColor badgeColor = m_aiConf > 0.8
                          ? QColor(40, 200, 100, 200)
                          : QColor(220, 140, 40, 200);

        painter->setPen(Qt::NoPen);
        painter->setBrush(badgeColor);
        painter->drawRoundedRect(textRect, 4, 4);
        painter->setPen(Qt::white);
        painter->drawText(textRect, Qt::AlignCenter, tag);
    }

    // Frame counter
    painter->setPen(QColor(255, 255, 255, 80));
    painter->setFont(QFont("monospace", 8));
    painter->drawText(xOff + 8, yOff + scaled.height() - 8,
                      QString("seq:%1").arg(m_frameSeq));
}