#include "services/WallpaperService.h"

#include "core/Config.h"
#include "platform/Platform.h"

#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QUrl>
#include <QVariantMap>
#include <QDateTime>

#include <algorithm>

WallpaperService::WallpaperService(Config* config, QObject* parent) : QObject(parent), m_config(config) {
  connect(m_config, &Config::changed, this, [this] {
    refresh();
    applyAutomation();
  });
  m_poll.setInterval(1000);
  connect(&m_poll, &QTimer::timeout, this, &WallpaperService::pollCurrent);
  m_poll.start();
  connect(&m_rotate, &QTimer::timeout, this, [this] {
    const QVariantMap automation = m_config->wallpaper().value("automation").toMap();
    if (automation.value("order").toString() == "alphabetical" && !m_images.isEmpty()) {
      int idx = 0;
      for (int i = 0; i < m_images.size(); ++i) {
        if (m_images[i].toMap().value("path").toString() == m_current) {
          idx = (i + 1) % static_cast<int>(m_images.size());
        }
      }
      set(m_images[idx].toMap().value("path").toString());
    } else {
      random();
    }
  });
  pollCurrent();
  refresh();
  applyAutomation();
}

QString WallpaperService::currentUrl() const {
  return m_current.isEmpty() ? QString() : QUrl::fromLocalFile(m_current).toString();
}

QString WallpaperService::directory() const {
  QString dir = m_config->wallpaper().value("directory").toString().trimmed();
  if (dir.isEmpty()) {
    // Same default as Noctalia: the Pictures folder (its "Wallpapers" subfolder if there is one).
    const QString pictures = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    const QString wallpapers = QDir(pictures).filePath("Wallpapers");
    dir = QFileInfo::exists(wallpapers) ? wallpapers : pictures;
  }
  dir.replace("%USERPROFILE%", QDir::homePath(), Qt::CaseInsensitive);
  if (dir.startsWith("~")) {
    dir = QDir::homePath() + dir.mid(1);
  }
  return QDir::cleanPath(dir);
}

void WallpaperService::pollCurrent() {
  const QString path = QString::fromStdWString(platform::currentWallpaper());
  const QFileInfo file(path);
  const QString signature = path + '|' + QString::number(file.lastModified().toMSecsSinceEpoch()) + '|' + QString::number(file.size());
  if (path != m_current || signature != m_currentSignature) {
    m_current = path;
    m_currentSignature = signature;
    emit currentChanged();
  }
}

void WallpaperService::refresh() {
  const QString dir = directory();
  const bool recursive = m_config->wallpaper().value("recursive", true).toBool();
  const QString key = dir + (recursive ? "|r" : "|n");
  if (key == m_scannedKey && !m_images.isEmpty()) {
    return;
  }
  m_scannedKey = key;
  m_loading = true;
  emit imagesChanged();
  m_worker.run(
      [dir, recursive] {
        QStringList files;
        QDirIterator it(dir, {"*.jpg", "*.jpeg", "*.png", "*.bmp", "*.webp", "*.jfif", "*.gif"}, QDir::Files,
                        recursive ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags);
        while (it.hasNext() && files.size() < 2000) {
          files.append(it.next());
        }
        std::sort(files.begin(), files.end(), [](const QString& a, const QString& b) {
          return QFileInfo(a).fileName().compare(QFileInfo(b).fileName(), Qt::CaseInsensitive) < 0;
        });
        return files;
      },
      this,
      [this](QStringList files) {
        QVariantList list;
        for (const QString& f : files) {
          list.append(QVariantMap{{"path", QDir::toNativeSeparators(f)},
                                  {"url", QUrl::fromLocalFile(f).toString()},
                                  {"name", QFileInfo(f).completeBaseName()}});
        }
        m_images = list;
        m_loading = false;
        emit imagesChanged();
      });
}

void WallpaperService::set(const QString& path) {
  const QString mode = m_config->wallpaper().value("fill_mode", "crop").toString();
  if (platform::setWallpaper(QDir::toNativeSeparators(path).toStdWString(), mode.toStdWString())) {
    m_current = QDir::toNativeSeparators(path);
    const QFileInfo file(m_current);
    m_currentSignature = m_current + '|' + QString::number(file.lastModified().toMSecsSinceEpoch()) + '|' + QString::number(file.size());
    emit currentChanged();
  }
}

void WallpaperService::random() {
  if (m_images.isEmpty()) {
    return;
  }
  QString pick = m_current;
  for (int tries = 0; tries < 5 && (pick == m_current); ++tries) {
    pick = m_images[QRandomGenerator::global()->bounded(static_cast<int>(m_images.size()))].toMap().value("path").toString();
  }
  set(pick);
}

void WallpaperService::openFolder() const { QDesktopServices::openUrl(QUrl::fromLocalFile(directory())); }

void WallpaperService::applyAutomation() {
  const QVariantMap automation = m_config->wallpaper().value("automation").toMap();
  if (automation.value("enabled", false).toBool()) {
    const int seconds = std::max(30, automation.value("interval_seconds", 1800).toInt());
    if (!m_rotate.isActive() || m_rotate.interval() != seconds * 1000) {
      m_rotate.start(seconds * 1000);
    }
  } else {
    m_rotate.stop();
  }
}
