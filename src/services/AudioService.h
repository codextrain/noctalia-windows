#pragma once

#include "platform/Platform.h"

#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <memory>

// Audio for QML:
//   output: Audio.volume (0..1), Audio.muted, Audio.setVolume(v), Audio.toggleMute()
//   input:  Audio.micVolume, Audio.micMuted, Audio.setMicVolume(v), Audio.toggleMicMute()
//   devices: Audio.outputs / Audio.inputs -> [{id, name, isDefault}], Audio.setDefaultDevice(id)
//   apps:    Audio.sessions -> [{id, name, exePath, volume, muted, active}], setSessionVolume(id, v)
class AudioService : public QObject {
  Q_OBJECT
  Q_PROPERTY(qreal volume READ volume NOTIFY changed)
  Q_PROPERTY(bool muted READ muted NOTIFY changed)
  Q_PROPERTY(bool available READ available NOTIFY changed)
  Q_PROPERTY(QString deviceName READ deviceName NOTIFY changed)
  Q_PROPERTY(qreal micVolume READ micVolume NOTIFY micChanged)
  Q_PROPERTY(bool micMuted READ micMuted NOTIFY micChanged)
  Q_PROPERTY(bool micAvailable READ micAvailable NOTIFY micChanged)
  Q_PROPERTY(QString micName READ micName NOTIFY micChanged)
  Q_PROPERTY(QVariantList outputs READ outputs NOTIFY devicesChanged)
  Q_PROPERTY(QVariantList inputs READ inputs NOTIFY devicesChanged)
  Q_PROPERTY(QVariantList sessions READ sessions NOTIFY sessionsChanged)
  // QML sets this while the audio tab is visible: app volumes are polled only then.
  Q_PROPERTY(bool watchSessions READ watchSessions WRITE setWatchSessions NOTIFY sessionsChanged)

public:
  explicit AudioService(QObject* parent = nullptr);
  ~AudioService() override;

  qreal volume() const { return m_out.volume; }
  bool muted() const { return m_out.muted; }
  bool available() const { return m_out.available; }
  QString deviceName() const { return m_out.name; }
  qreal micVolume() const { return m_in.volume; }
  bool micMuted() const { return m_in.muted; }
  bool micAvailable() const { return m_in.available; }
  QString micName() const { return m_in.name; }
  QVariantList outputs() const { return m_outputs; }
  QVariantList inputs() const { return m_inputs; }
  QVariantList sessions() const { return m_sessions; }
  bool watchSessions() const { return m_sessionTimer.isActive(); }
  void setWatchSessions(bool on);

  Q_INVOKABLE void setVolume(qreal value);
  Q_INVOKABLE void changeVolume(qreal delta); // e.g. +0.05 on scroll up
  Q_INVOKABLE void setMuted(bool muted);
  Q_INVOKABLE void toggleMute();
  Q_INVOKABLE void setMicVolume(qreal value);
  Q_INVOKABLE void setMicMuted(bool muted);
  Q_INVOKABLE void toggleMicMute();

  Q_INVOKABLE void refreshDevices();
  Q_INVOKABLE void setDefaultDevice(const QString& id);
  Q_INVOKABLE void refreshSessions();
  Q_INVOKABLE void setSessionVolume(const QString& id, qreal value);
  Q_INVOKABLE void setSessionMuted(const QString& id, bool muted);

signals:
  void changed();
  void micChanged();
  void devicesChanged();
  void sessionsChanged();
  // Emitted for changes made by the user (keys, scroll, other apps): drives the OSD.
  void volumeChangedExternally();

private:
  struct Endpoint {
    std::unique_ptr<platform::AudioEndpoint> endpoint;
    qreal volume = 0;
    bool muted = false;
    bool available = false;
    QString name;
  };

  void onEndpointChanged(bool capture, float volume, bool muted);
  void rebind(bool capture);
  Endpoint& ep(bool capture) { return capture ? m_in : m_out; }

  Endpoint m_out;
  Endpoint m_in;
  QVariantList m_outputs;
  QVariantList m_inputs;
  QVariantList m_sessions;
  QTimer m_sessionTimer;
  bool m_selfChange = false;
};
