#pragma once

#include "platform/Platform.h"

#include <QObject>
#include <memory>

// Default output volume. QML: Audio.volume (0..1), Audio.muted, Audio.setVolume(v), Audio.toggleMute()
class AudioService : public QObject {
  Q_OBJECT
  Q_PROPERTY(qreal volume READ volume NOTIFY changed)
  Q_PROPERTY(bool muted READ muted NOTIFY changed)
  Q_PROPERTY(bool available READ available NOTIFY changed)
  Q_PROPERTY(QString deviceName READ deviceName NOTIFY changed)

public:
  explicit AudioService(QObject* parent = nullptr);
  ~AudioService() override;

  qreal volume() const { return m_volume; }
  bool muted() const { return m_muted; }
  bool available() const { return m_available; }
  QString deviceName() const { return m_deviceName; }

  Q_INVOKABLE void setVolume(qreal value);
  Q_INVOKABLE void changeVolume(qreal delta); // e.g. +0.05 on scroll up
  Q_INVOKABLE void setMuted(bool muted);
  Q_INVOKABLE void toggleMute();

signals:
  void changed();

private:
  void onEndpointChanged(float volume, bool muted);
  void rebind();

  std::unique_ptr<platform::AudioEndpoint> m_endpoint;
  qreal m_volume = 0;
  bool m_muted = false;
  bool m_available = false;
  QString m_deviceName;
};
