#pragma once

#include "core/Worker.h"

#include <QObject>
#include <QTimer>
#include <QVariantList>

class Config;

// Desktop wallpaper (Noctalia's [wallpaper]): the current one, the images in your wallpaper
// folder, set / random, and optional automatic rotation.
// QML: Wallpaper.current (path), currentUrl, images [{path, url, name}], set(path), random()
class WallpaperService : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString current READ current NOTIFY currentChanged)
  Q_PROPERTY(QString currentUrl READ currentUrl NOTIFY currentChanged)
  Q_PROPERTY(QString directory READ directory NOTIFY imagesChanged)
  Q_PROPERTY(QVariantList images READ images NOTIFY imagesChanged)
  Q_PROPERTY(bool loading READ loading NOTIFY imagesChanged)

public:
  explicit WallpaperService(Config* config, QObject* parent = nullptr);

  QString current() const { return m_current; }
  QString currentUrl() const;
  QString directory() const;
  QVariantList images() const { return m_images; }
  bool loading() const { return m_loading; }

  Q_INVOKABLE void set(const QString& path);
  Q_INVOKABLE void random();
  Q_INVOKABLE void refresh();
  Q_INVOKABLE void openFolder() const;

signals:
  void currentChanged();
  void imagesChanged();

private:
  void pollCurrent();
  void applyAutomation();

  Config* m_config = nullptr;
  Worker m_worker{"wallpaper"};
  QString m_current;
  QVariantList m_images;
  QString m_scannedKey;
  bool m_loading = false;
  QTimer m_poll;
  QTimer m_rotate;
};
