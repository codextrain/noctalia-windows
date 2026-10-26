#include "services/SystemServices.h"

#include "platform/Platform.h"

#include <QDateTime>
#include <QNetworkInformation>
#include <QVariantMap>

// ── Battery ─────────────────────────────────────────────────────────────────

BatteryService::BatteryService(QObject* parent) : QObject(parent) {
  m_timer.setInterval(5000);
  connect(&m_timer, &QTimer::timeout, this, &BatteryService::poll);
  m_timer.start();
  poll();
}

void BatteryService::poll() {
  const platform::PowerStatus s = platform::queryPower();
  const int minutes = s.secondsLeft < 0 ? -1 : s.secondsLeft / 60;
  const bool charging = s.charging || (s.acOnline && s.percent >= 0 && s.percent < 100);
  if (s.hasBattery == m_present && s.percent == m_percent && charging == m_charging && s.acOnline == m_plugged &&
      minutes == m_minutesLeft) {
    return;
  }
  m_present = s.hasBattery;
  m_percent = s.percent;
  m_charging = charging;
  m_plugged = s.acOnline;
  m_minutesLeft = minutes;
  emit changed();
}

QString BatteryService::glyph() const {
  // Same thresholds as noctalia/src/dbus/upower/upower_service.cpp (batteryGlyphName).
  if (m_charging) {
    return "battery-charging";
  }
  if (m_plugged) {
    return "battery-plugged";
  }
  if (m_percent < 0) {
    return "battery-exclamation";
  }
  if (m_percent >= 85) {
    return "battery-4";
  }
  if (m_percent >= 55) {
    return "battery-3";
  }
  if (m_percent >= 30) {
    return "battery-2";
  }
  if (m_percent >= 10) {
    return "battery-1";
  }
  return "battery-0";
}

// ── Network ─────────────────────────────────────────────────────────────────

NetworkService::NetworkService(QObject* parent) : QObject(parent) {
  // On Windows Qt uses the "networklistmanager" backend (no polling, push notifications).
  if (QNetworkInformation::loadDefaultBackend() && QNetworkInformation::instance() != nullptr) {
    auto* info = QNetworkInformation::instance();
    connect(info, &QNetworkInformation::reachabilityChanged, this, &NetworkService::refresh);
    connect(info, &QNetworkInformation::transportMediumChanged, this, &NetworkService::refresh);
  }
  refresh();
}

void NetworkService::refresh() {
  auto* info = QNetworkInformation::instance();
  bool connected = true;
  bool wifi = false;
  bool ethernet = false;
  if (info != nullptr) {
    const auto r = info->reachability();
    connected = r == QNetworkInformation::Reachability::Online || r == QNetworkInformation::Reachability::Site ||
                r == QNetworkInformation::Reachability::Unknown;
    const auto medium = info->transportMedium();
    wifi = medium == QNetworkInformation::TransportMedium::WiFi;
    ethernet = medium == QNetworkInformation::TransportMedium::Ethernet;
  }
  if (connected == m_connected && wifi == m_wifi && ethernet == m_ethernet) {
    return;
  }
  m_connected = connected;
  m_wifi = wifi;
  m_ethernet = ethernet;
  emit changed();
}

QString NetworkService::glyph() const {
  if (!m_connected) {
    return m_wifi ? "wifi-off" : "network-off";
  }
  if (m_wifi) {
    return "wifi";
  }
  return m_ethernet ? "network" : "world";
}

QString NetworkService::label() const {
  if (!m_connected) {
    return QStringLiteral("Sem conexão");
  }
  if (m_wifi) {
    return QStringLiteral("Wi-Fi conectado");
  }
  return m_ethernet ? QStringLiteral("Cabo de rede conectado") : QStringLiteral("Conectado");
}

// ── Workspaces ──────────────────────────────────────────────────────────────

WorkspaceService::WorkspaceService(QObject* parent) : QObject(parent) {
  m_fast.setInterval(250);
  m_slow.setInterval(1500);
  connect(&m_fast, &QTimer::timeout, this, [this] { poll(false); });
  connect(&m_slow, &QTimer::timeout, this, [this] { poll(true); });
  m_fast.start();
  m_slow.start();
  poll(true);
}

void WorkspaceService::poll(bool withOccupancy) {
  if (QDateTime::currentMSecsSinceEpoch() < m_suppressUntil) {
    return;
  }
  const platform::DesktopState s = platform::queryDesktops(withOccupancy);
  QList<bool> occupied = m_occupied;
  if (withOccupancy || occupied.size() != s.count) {
    occupied.clear();
    for (int i = 0; i < s.count; ++i) {
      occupied.append(i < static_cast<int>(s.occupied.size()) && s.occupied[static_cast<size_t>(i)]);
    }
  }

  QVariantList model;
  for (int i = 0; i < s.count; ++i) {
    const QString name =
        i < static_cast<int>(s.names.size()) ? QString::fromStdWString(s.names[static_cast<size_t>(i)]) : QString();
    model.append(QVariantMap{
        {"index", i},
        {"name", name.isEmpty() ? QString::number(i + 1) : name},
        {"focused", i == s.current},
        // The focused desktop counts as occupied (Noctalia does the same).
        {"occupied", occupied.value(i) || i == s.current},
    });
  }
  if (model == m_model && s.current == m_current) {
    return;
  }
  m_model = model;
  m_current = s.current;
  m_occupied = occupied;
  emit changed();
}

void WorkspaceService::activate(int index) {
  if (index < 0 || index >= count() || index == m_current) {
    return;
  }
  platform::switchDesktop(m_current, index);
  m_suppressUntil = QDateTime::currentMSecsSinceEpoch() + 600;
  // Optimistic update so the pill animates immediately; the next poll confirms it.
  m_current = index;
  for (auto& v : m_model) {
    QVariantMap m = v.toMap();
    m["focused"] = m.value("index").toInt() == index;
    v = m;
  }
  emit changed();
}

void WorkspaceService::next() { activate(m_current + 1); }

void WorkspaceService::previous() { activate(m_current - 1); }

void WorkspaceService::create() {
  platform::createDesktop();
  poll(true);
}
