#pragma once

#include <QHash>
#include <QMutex>
#include <QQuickImageProvider>

// Serves app / file icons to QML:
//   Image { source: "image://icons/app/<percent-encoded app id>"; asynchronous: true }
//   Image { source: "image://icons/file/<percent-encoded path>" }
// Icons are extracted once (on Qt's image loader threads) and cached.
class IconProvider : public QQuickImageProvider {
public:
  IconProvider();
  QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;

private:
  QMutex m_mutex;
  QHash<QString, QImage> m_cache;
};
