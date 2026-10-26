#pragma once

#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QVariantList>

// ── Battery ─────────────────────────────────────────────────────────────────
// QML: Battery.present, Battery.percent, Battery.charging, Battery.plugged, Battery.glyph
class BatteryService : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool present READ present NOTIFY changed)
  Q_PROPERTY(int percent READ percent NOTIFY changed)
  Q_PROPERTY(bool charging READ charging NOTIFY changed)
  Q_PROPERTY(bool plugged READ plugged NOTIFY changed)
  Q_PROPERTY(int minutesLeft READ minutesLeft NOTIFY changed)
  Q_PROPERTY(QString glyph READ glyph NOTIFY changed)

public:
  explicit BatteryService(QObject* parent = nullptr);
  bool present() const { return m_present; }
  int percent() const { return m_percent; }
  bool charging() const { return m_charging; }
  bool plugged() const { return m_plugged; }
  int minutesLeft() const { return m_minutesLeft; }
  QString glyph() const;

signals:
  void changed();

private:
  void poll();
  QTimer m_timer;
  bool m_present = false;
  int m_percent = -1;
  bool m_charging = false;
  bool m_plugged = false;
  int m_minutesLeft = -1;
};

// ── Network ─────────────────────────────────────────────────────────────────
// QML: Network.connected, Network.wifi, Network.glyph, Network.label
class NetworkService : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool connected READ connected NOTIFY changed)
  Q_PROPERTY(bool wifi READ wifi NOTIFY changed)
  Q_PROPERTY(QString glyph READ glyph NOTIFY changed)
  Q_PROPERTY(QString label READ label NOTIFY changed)

public:
  explicit NetworkService(QObject* parent = nullptr);
  bool connected() const { return m_connected; }
  bool wifi() const { return m_wifi; }
  QString glyph() const;
  QString label() const;

signals:
  void changed();

private:
  void refresh();
  bool m_connected = true;
  bool m_wifi = false;
  bool m_ethernet = false;
};

// ── Workspaces (Windows virtual desktops) ──────────────────────────────────
// QML: Workspaces.model -> [{index, name, focused, occupied}], Workspaces.activate(i)
class WorkspaceService : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariantList model READ model NOTIFY changed)
  Q_PROPERTY(int current READ current NOTIFY changed)
  Q_PROPERTY(int count READ count NOTIFY changed)

public:
  explicit WorkspaceService(QObject* parent = nullptr);
  QVariantList model() const { return m_model; }
  int current() const { return m_current; }
  int count() const { return m_model.size(); }

  Q_INVOKABLE void activate(int index);
  Q_INVOKABLE void next();
  Q_INVOKABLE void previous();
  Q_INVOKABLE void create();

signals:
  void changed();

private:
  void poll(bool withOccupancy);
  QTimer m_fast; // current desktop (cheap registry read)
  QTimer m_slow; // occupied desktops (walks all windows)
  QVariantList m_model;
  int m_current = 0;
  QList<bool> m_occupied;
  qint64 m_suppressUntil = 0; // ignore stale reads right after we switch
};
