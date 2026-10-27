#include "services/DeviceServices.h"

#include <QDateTime>
#include <QMutexLocker>
#include <QPointer>
#include <QUrl>
#include <QVariantMap>

#include <algorithm>
#include <cmath>

// ── Media ───────────────────────────────────────────────────────────────────

MediaService::MediaService(QObject* parent) : QObject(parent) {
  QPointer<MediaService> self(this);
  m_controller = platform::MediaController::create([self](const platform::MediaState& st) {
    if (!self) {
      return;
    }
    QMetaObject::invokeMethod(
        self.data(), [self, st] { if (self) self->apply(st); }, Qt::QueuedConnection);
  });
  // While playing, the position moves on its own: tick the UI twice a second.
  m_tick.setInterval(500);
  connect(&m_tick, &QTimer::timeout, this, &MediaService::positionChanged);
}

MediaService::~MediaService() {
  m_controller.reset(); // joins the worker before members go away
}

void MediaService::apply(platform::MediaState st) {
  const bool newArt = st.artVersion != m_state.artVersion;
  if (newArt) {
    QImage img;
    if (!st.art.empty()) {
      img.loadFromData(st.art.data(), static_cast<int>(st.art.size()));
    }
    {
      QMutexLocker lock(&m_artMutex);
      m_art = img;
    }
    m_artUrl = img.isNull() ? QString() : QStringLiteral("image://media/%1").arg(st.artVersion);
  }
  st.art.clear(); // keep only the decoded copy
  m_state = std::move(st);
  m_stateReceivedMs = QDateTime::currentMSecsSinceEpoch();
  if (m_state.playing && m_state.duration > 0) {
    m_tick.start();
  } else {
    m_tick.stop();
  }
  emit changed();
  emit positionChanged();
}

QString MediaService::appIcon() const {
  if (m_state.appId.empty()) {
    return {};
  }
  return QStringLiteral("image://icons/app/%1")
      .arg(QString::fromLatin1(QUrl::toPercentEncoding(QString::fromStdWString(m_state.appId))));
}

double MediaService::position() const {
  double p = m_state.position;
  if (m_state.playing) {
    p += (QDateTime::currentMSecsSinceEpoch() - m_stateReceivedMs) / 1000.0;
  }
  if (m_state.duration > 0) {
    p = std::min(p, m_state.duration);
  }
  return std::max(p, 0.0);
}

QImage MediaService::art() const {
  QMutexLocker lock(&m_artMutex);
  return m_art;
}

void MediaService::playPause() {
  if (m_controller) m_controller->playPause();
}
void MediaService::next() {
  if (m_controller) m_controller->next();
}
void MediaService::previous() {
  if (m_controller) m_controller->previous();
}
void MediaService::seek(double seconds) {
  if (m_controller) m_controller->seek(seconds);
}

// ── Bluetooth ───────────────────────────────────────────────────────────────

namespace {
  QString bluetoothGlyph(const QString& kind) {
    if (kind == "headset") return "bluetooth-device-headset";
    if (kind == "audio") return "bluetooth-device-headphones";
    if (kind == "mouse") return "bluetooth-device-mouse";
    if (kind == "keyboard") return "keyboard";
    if (kind == "gamepad") return "bluetooth-device-gamepad";
    if (kind == "phone") return "bluetooth-device-phone";
    if (kind == "watch") return "bluetooth-device-watch";
    if (kind == "computer") return "device-desktop";
    return "bluetooth";
  }

  struct BluetoothSnapshot {
    platform::RadioState radio;
    std::vector<platform::BluetoothDevice> devices;
  };
} // namespace

BluetoothService::BluetoothService(QObject* parent) : QObject(parent) {
  m_timer.setInterval(5000);
  connect(&m_timer, &QTimer::timeout, this, &BluetoothService::refresh);
  m_timer.start();
  refresh();
}

void BluetoothService::refresh() {
  m_worker.run(
      [] {
        BluetoothSnapshot s;
        s.radio = platform::queryRadio(platform::RadioKind::Bluetooth);
        if (s.radio.present) {
          s.devices = platform::listBluetoothDevices();
        }
        return s;
      },
      this,
      [this](BluetoothSnapshot s) {
        QVariantList list;
        for (const auto& d : s.devices) {
          const QString kind = QString::fromStdWString(d.kind);
          list.append(QVariantMap{{"id", QString::fromStdWString(d.id)},
                                  {"name", QString::fromStdWString(d.name)},
                                  {"connected", d.connected},
                                  {"kind", kind},
                                  {"glyph", bluetoothGlyph(kind)}});
        }
        std::stable_sort(list.begin(), list.end(), [](const QVariant& a, const QVariant& b) {
          return a.toMap().value("connected").toBool() && !b.toMap().value("connected").toBool();
        });
        if (s.radio.present == m_available && s.radio.on == m_enabled && list == m_devices && !m_busy) {
          return;
        }
        m_available = s.radio.present;
        m_enabled = s.radio.on;
        m_devices = list;
        m_busy = false;
        emit changed();
      });
}

void BluetoothService::setEnabled(bool on) {
  m_busy = true;
  m_enabled = on; // optimistic
  emit changed();
  m_worker.run([on] { return platform::setRadio(platform::RadioKind::Bluetooth, on); }, this,
               [this](bool) { refresh(); });
}

// ── Wi-Fi ───────────────────────────────────────────────────────────────────

WifiService::WifiService(QObject* parent) : QObject(parent) {
  m_watchTimer.setInterval(3000);
  connect(&m_watchTimer, &QTimer::timeout, this, &WifiService::refresh);
  m_slowTimer.setInterval(20000);
  connect(&m_slowTimer, &QTimer::timeout, this, &WifiService::refresh);
  m_slowTimer.start();
  refresh();
}

void WifiService::setWatching(bool on) {
  if (on == m_watchTimer.isActive()) {
    return;
  }
  if (on) {
    scan();
    refresh();
    m_watchTimer.start();
  } else {
    m_watchTimer.stop();
  }
  emit changed();
}

void WifiService::refresh() {
  struct Snap {
    platform::WifiState wifi;
    platform::RadioState radio;
  };
  m_worker.run(
      [] {
        Snap s;
        s.wifi = platform::queryWifi();
        s.radio = platform::queryRadio(platform::RadioKind::WiFi);
        return s;
      },
      this,
      [this](Snap s) {
        m_enabled = s.radio.present ? s.radio.on : s.wifi.available;
        applyState(std::move(s.wifi));
      });
}

void WifiService::applyState(platform::WifiState st) {
  QVariantList list;
  for (const auto& n : st.networks) {
    list.append(QVariantMap{{"ssid", QString::fromStdWString(n.ssid)},
                            {"signal", n.signal},
                            {"secured", n.secured},
                            {"connected", n.connected},
                            {"saved", n.saved}});
  }
  if (!m_connecting.isEmpty() && st.connected && QString::fromStdWString(st.ssid) == m_connecting) {
    m_connecting.clear();
  }
  m_state = std::move(st);
  m_networks = list;
  emit changed();
}

void WifiService::scan() {
  m_worker.post([] { platform::scanWifi(); });
  QTimer::singleShot(2500, this, &WifiService::refresh);
}

void WifiService::connectTo(const QString& ssid, const QString& password) {
  m_connecting = ssid;
  m_lastError.clear();
  emit changed();
  const std::wstring s = ssid.toStdWString();
  const std::wstring p = password.toStdWString();
  m_worker.run([s, p] { return platform::connectWifi(s, p); }, this, [this, ssid](bool ok) {
    if (!ok) {
      m_lastError = QStringLiteral("Não foi possível conectar a %1").arg(ssid);
      m_connecting.clear();
      emit changed();
      return;
    }
    // Give Windows time to associate, then check; clear "connecting" after a while either way.
    QTimer::singleShot(3000, this, &WifiService::refresh);
    QTimer::singleShot(12000, this, [this, ssid] {
      if (m_connecting == ssid) {
        m_connecting.clear();
        if (!m_state.connected || QString::fromStdWString(m_state.ssid) != ssid) {
          m_lastError = QStringLiteral("Não foi possível conectar a %1 (senha errada?)").arg(ssid);
        }
        emit changed();
      }
    });
  });
}

void WifiService::disconnect() {
  m_worker.run([] { return platform::disconnectWifi(); }, this, [this](bool) {
    QTimer::singleShot(1000, this, &WifiService::refresh);
  });
}

void WifiService::setEnabled(bool on) {
  m_enabled = on;
  emit changed();
  m_worker.run([on] { return platform::setRadio(platform::RadioKind::WiFi, on); }, this,
               [this](bool) { QTimer::singleShot(1500, this, &WifiService::refresh); });
}

// ── Brightness ──────────────────────────────────────────────────────────────

BrightnessService::BrightnessService(QObject* parent) : QObject(parent) {
  // DDC/CI writes take ~50 ms each: coalesce slider drags into at most ~8 writes per second.
  m_writeTimer.setInterval(120);
  m_writeTimer.setSingleShot(true);
  connect(&m_writeTimer, &QTimer::timeout, this, &BrightnessService::flushWrites);
  refresh();
}

bool BrightnessService::available() const {
  return std::any_of(m_displays.begin(), m_displays.end(),
                     [](const QVariant& d) { return d.toMap().value("supported").toBool(); });
}

double BrightnessService::value() const {
  for (const QVariant& d : m_displays) {
    const QVariantMap m = d.toMap();
    if (m.value("supported").toBool()) {
      return m.value("brightness").toDouble();
    }
  }
  return 0;
}

void BrightnessService::refresh() {
  m_loading = true;
  emit changed();
  m_worker.run([] { return platform::listDisplays(); }, this, [this](std::vector<platform::DisplayInfo> list) {
    QVariantList out;
    for (const auto& d : list) {
      out.append(QVariantMap{{"id", QString::fromStdWString(d.id)},
                             {"name", QString::fromStdWString(d.name)},
                             {"internal", d.internal},
                             {"supported", d.brightnessSupported},
                             {"brightness", d.brightness < 0 ? 0.0 : d.brightness / 100.0},
                             {"width", d.width},
                             {"height", d.height},
                             {"refresh", d.refreshRate}});
    }
    m_displays = out;
    m_loading = false;
    emit changed();
  });
}

void BrightnessService::setDisplay(const QString& id, double v) {
  v = std::clamp(v, 0.0, 1.0);
  for (QVariant& d : m_displays) {
    QVariantMap m = d.toMap();
    if (m.value("id").toString() == id) {
      m["brightness"] = v;
      d = m;
    }
  }
  emit changed();
  queueWrite(id, static_cast<int>(std::lround(v * 100)));
}

void BrightnessService::setValue(double v) {
  for (const QVariant& d : std::as_const(m_displays)) {
    const QVariantMap m = d.toMap();
    if (m.value("supported").toBool()) {
      setDisplay(m.value("id").toString(), v);
    }
  }
}

void BrightnessService::change(double delta) {
  setValue(value() + delta);
  emit changedByUser();
}

void BrightnessService::queueWrite(const QString& id, int percent) {
  m_pending.insert(id, percent);
  if (!m_writeTimer.isActive() && !m_writing) {
    m_writeTimer.start();
  }
}

void BrightnessService::flushWrites() {
  if (m_pending.isEmpty()) {
    return;
  }
  const QHash<QString, int> batch = m_pending;
  m_pending.clear();
  m_writing = true;
  m_worker.run(
      [batch] {
        for (auto it = batch.constBegin(); it != batch.constEnd(); ++it) {
          platform::setDisplayBrightness(it.key().toStdWString(), it.value());
        }
        return true;
      },
      this,
      [this](bool) {
        m_writing = false;
        if (!m_pending.isEmpty()) {
          m_writeTimer.start();
        }
      });
}
