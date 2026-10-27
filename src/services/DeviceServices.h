#pragma once

#include "core/Worker.h"
#include "platform/Platform.h"

#include <QImage>
#include <QMutex>
#include <QObject>
#include <QQuickImageProvider>
#include <QTimer>
#include <QVariantList>
#include <memory>

// ── Media ("now playing") ───────────────────────────────────────────────────
// QML: Media.available, title, artist, album, playing, position, duration, artUrl,
//      Media.playPause(), next(), previous(), seek(seconds)
class MediaService : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool available READ available NOTIFY changed)
  Q_PROPERTY(QString title READ title NOTIFY changed)
  Q_PROPERTY(QString artist READ artist NOTIFY changed)
  Q_PROPERTY(QString album READ album NOTIFY changed)
  Q_PROPERTY(QString appId READ appId NOTIFY changed)
  Q_PROPERTY(QString appIcon READ appIcon NOTIFY changed)
  Q_PROPERTY(bool playing READ playing NOTIFY changed)
  Q_PROPERTY(bool canNext READ canNext NOTIFY changed)
  Q_PROPERTY(bool canPrevious READ canPrevious NOTIFY changed)
  Q_PROPERTY(bool canSeek READ canSeek NOTIFY changed)
  Q_PROPERTY(double position READ position NOTIFY positionChanged)
  Q_PROPERTY(double duration READ duration NOTIFY changed)
  Q_PROPERTY(QString artUrl READ artUrl NOTIFY changed)

public:
  explicit MediaService(QObject* parent = nullptr);
  ~MediaService() override;

  bool available() const { return m_state.available && !m_state.title.empty(); }
  QString title() const { return QString::fromStdWString(m_state.title); }
  QString artist() const { return QString::fromStdWString(m_state.artist); }
  QString album() const { return QString::fromStdWString(m_state.album); }
  QString appId() const { return QString::fromStdWString(m_state.appId); }
  QString appIcon() const;
  bool playing() const { return m_state.playing; }
  bool canNext() const { return m_state.canNext; }
  bool canPrevious() const { return m_state.canPrevious; }
  bool canSeek() const { return m_state.canSeek; }
  double position() const;
  double duration() const { return m_state.duration; }
  QString artUrl() const { return m_artUrl; }

  Q_INVOKABLE void playPause();
  Q_INVOKABLE void next();
  Q_INVOKABLE void previous();
  Q_INVOKABLE void seek(double seconds);

  // Cover art for the "image://media/..." provider (thread-safe).
  QImage art() const;

signals:
  void changed();
  void positionChanged();

private:
  void apply(platform::MediaState st);

  std::unique_ptr<platform::MediaController> m_controller;
  platform::MediaState m_state;
  qint64 m_stateReceivedMs = 0;
  QString m_artUrl;
  mutable QMutex m_artMutex;
  QImage m_art;
  QTimer m_tick;
};

class MediaArtProvider : public QQuickImageProvider {
public:
  explicit MediaArtProvider(MediaService* media) : QQuickImageProvider(Image), m_media(media) {}
  QImage requestImage(const QString&, QSize* size, const QSize&) override {
    const QImage img = m_media->art();
    if (size != nullptr) {
      *size = img.size();
    }
    return img;
  }

private:
  MediaService* m_media;
};

// ── Bluetooth ───────────────────────────────────────────────────────────────
// QML: Bluetooth.available, enabled, devices [{id, name, connected, kind, glyph}], setEnabled(on)
class BluetoothService : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool available READ available NOTIFY changed)
  Q_PROPERTY(bool enabled READ enabled NOTIFY changed)
  Q_PROPERTY(bool busy READ busy NOTIFY changed)
  Q_PROPERTY(QVariantList devices READ devices NOTIFY changed)

public:
  explicit BluetoothService(QObject* parent = nullptr);
  bool available() const { return m_available; }
  bool enabled() const { return m_enabled; }
  bool busy() const { return m_busy; }
  QVariantList devices() const { return m_devices; }
  Q_INVOKABLE void refresh();
  Q_INVOKABLE void setEnabled(bool on);

signals:
  void changed();

private:
  Worker m_worker{"bluetooth"};
  QTimer m_timer;
  bool m_available = false;
  bool m_enabled = false;
  bool m_busy = false;
  QVariantList m_devices;
};

// ── Wi-Fi ───────────────────────────────────────────────────────────────────
// QML: Wifi.available, enabled, connected, ssid, signal, networks [{ssid, signal, secured,
//      connected, saved}], scan(), connectTo(ssid, password), disconnect(), setEnabled(on)
class WifiService : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool available READ available NOTIFY changed)
  Q_PROPERTY(bool enabled READ enabled NOTIFY changed)
  Q_PROPERTY(bool connected READ connected NOTIFY changed)
  Q_PROPERTY(QString ssid READ ssid NOTIFY changed)
  Q_PROPERTY(int signal READ signalStrength NOTIFY changed)
  Q_PROPERTY(QVariantList networks READ networks NOTIFY changed)
  Q_PROPERTY(QString connecting READ connecting NOTIFY changed)
  Q_PROPERTY(QString lastError READ lastError NOTIFY changed)
  // true while the network tab is visible: refreshes every few seconds
  Q_PROPERTY(bool watching READ watching WRITE setWatching NOTIFY changed)

public:
  explicit WifiService(QObject* parent = nullptr);
  bool available() const { return m_state.available; }
  bool enabled() const { return m_enabled; }
  bool connected() const { return m_state.connected; }
  QString ssid() const { return QString::fromStdWString(m_state.ssid); }
  int signalStrength() const { return m_state.signal; }
  QVariantList networks() const { return m_networks; }
  QString connecting() const { return m_connecting; }
  QString lastError() const { return m_lastError; }
  bool watching() const { return m_watchTimer.isActive(); }
  void setWatching(bool on);

  Q_INVOKABLE void refresh();
  Q_INVOKABLE void scan();
  Q_INVOKABLE void connectTo(const QString& ssid, const QString& password);
  Q_INVOKABLE void disconnect();
  Q_INVOKABLE void setEnabled(bool on);

signals:
  void changed();

private:
  void applyState(platform::WifiState st);

  Worker m_worker{"wifi"};
  QTimer m_watchTimer;
  QTimer m_slowTimer;
  platform::WifiState m_state;
  bool m_enabled = true;
  QVariantList m_networks;
  QString m_connecting;
  QString m_lastError;
};

// ── Displays & brightness ───────────────────────────────────────────────────
// QML: Brightness.available, value (0..1, first display that supports it), setValue(v),
//      Brightness.displays [{id, name, internal, supported, brightness, width, height, refresh}],
//      Brightness.setDisplay(id, v)
class BrightnessService : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool available READ available NOTIFY changed)
  Q_PROPERTY(double value READ value NOTIFY changed)
  Q_PROPERTY(QVariantList displays READ displays NOTIFY changed)
  Q_PROPERTY(bool loading READ loading NOTIFY changed)

public:
  explicit BrightnessService(QObject* parent = nullptr);
  bool available() const;
  double value() const;
  QVariantList displays() const { return m_displays; }
  bool loading() const { return m_loading; }

  Q_INVOKABLE void refresh();
  Q_INVOKABLE void setValue(double v); // all displays that support brightness
  Q_INVOKABLE void setDisplay(const QString& id, double v);
  Q_INVOKABLE void change(double delta);

signals:
  void changed();
  void changedByUser(); // drives the OSD

private:
  void queueWrite(const QString& id, int percent);
  void flushWrites();

  Worker m_worker{"brightness"};
  QVariantList m_displays;
  bool m_loading = false;
  QHash<QString, int> m_pending; // id -> percent waiting to be written
  bool m_writing = false;
  QTimer m_writeTimer;
};
