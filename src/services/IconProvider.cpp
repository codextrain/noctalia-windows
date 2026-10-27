#include "services/IconProvider.h"

#include "platform/Platform.h"

#include <QMutexLocker>
#include <QUrl>

IconProvider::IconProvider() : QQuickImageProvider(QQuickImageProvider::Image) {}

QImage IconProvider::requestImage(const QString& id, QSize* size, const QSize& requestedSize) {
  // Ask Windows for a large icon and let Qt scale it down: crisp on high-DPI screens.
  const int px = std::max({requestedSize.width(), requestedSize.height(), 64});
  const QString key = QStringLiteral("%1@%2").arg(id).arg(px);

  {
    QMutexLocker lock(&m_mutex);
    const auto it = m_cache.constFind(key);
    if (it != m_cache.constEnd()) {
      if (size != nullptr) {
        *size = it->size();
      }
      return *it;
    }
  }

  const int slash = id.indexOf('/');
  const QString kind = id.left(slash);
  const QString value = QUrl::fromPercentEncoding(id.mid(slash + 1).toUtf8());

  platform::initializeThread();
  platform::Image img;
  if (kind == QLatin1String("app")) {
    img = platform::appIcon(value.toStdWString(), px);
  } else if (kind == QLatin1String("file")) {
    img = platform::fileIcon(value.toStdWString(), px);
  }

  QImage out;
  if (img.valid()) {
    out = QImage(img.bgra.data(), img.width, img.height, img.width * 4, QImage::Format_ARGB32_Premultiplied).copy();
  }

  {
    QMutexLocker lock(&m_mutex);
    m_cache.insert(key, out);
  }
  if (size != nullptr) {
    *size = out.size();
  }
  return out;
}
