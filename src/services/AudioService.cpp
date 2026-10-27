#include "services/AudioService.h"

#include <QPointer>
#include <QVariantMap>

#include <algorithm>
#include <cmath>

AudioService::AudioService(QObject* parent) : QObject(parent) {
  QPointer<AudioService> self(this);
  for (const bool capture : {false, true}) {
    // Core Audio calls these from its own threads: hop back to the UI thread before touching state.
    ep(capture).endpoint = platform::AudioEndpoint::create(
        [self, capture](float v, bool m) {
          if (self) {
            QMetaObject::invokeMethod(
                self.data(), [self, capture, v, m] { if (self) self->onEndpointChanged(capture, v, m); },
                Qt::QueuedConnection);
          }
        },
        [self, capture] {
          if (self) {
            QMetaObject::invokeMethod(
                self.data(), [self, capture] {
                  if (self) {
                    self->rebind(capture);
                    self->refreshDevices();
                  }
                },
                Qt::QueuedConnection);
          }
        },
        capture);
    rebind(capture);
  }
  refreshDevices();

  m_sessionTimer.setInterval(1500);
  connect(&m_sessionTimer, &QTimer::timeout, this, &AudioService::refreshSessions);
}

AudioService::~AudioService() = default;

void AudioService::rebind(bool capture) {
  Endpoint& e = ep(capture);
  if (!e.endpoint) {
    return;
  }
  e.endpoint->rebind();
  e.available = e.endpoint->valid();
  e.name = QString::fromStdWString(e.endpoint->deviceName());
  e.volume = e.endpoint->volume();
  e.muted = e.endpoint->muted();
  if (capture) {
    emit micChanged();
  } else {
    emit changed();
  }
}

void AudioService::onEndpointChanged(bool capture, float volume, bool muted) {
  Endpoint& e = ep(capture);
  if (std::abs(volume - e.volume) < 0.0005 && muted == e.muted) {
    return;
  }
  e.volume = volume;
  e.muted = muted;
  if (capture) {
    emit micChanged();
  } else {
    emit changed();
    if (!m_selfChange) {
      emit volumeChangedExternally();
    }
  }
}

// ── output ──────────────────────────────────────────────────────────────────

void AudioService::setVolume(qreal value) {
  if (!m_out.endpoint || !m_out.available) {
    return;
  }
  // Snap to whole percents like Noctalia's volume widget.
  value = std::round(std::clamp(value, 0.0, 1.0) * 100.0) / 100.0;
  m_selfChange = true;
  m_out.endpoint->setVolume(static_cast<float>(value));
  if (m_out.muted && value > 0) {
    m_out.endpoint->setMuted(false);
  }
  onEndpointChanged(false, static_cast<float>(value), m_out.muted && value <= 0);
  m_selfChange = false;
}

void AudioService::changeVolume(qreal delta) {
  setVolume(m_out.volume + delta);
  emit volumeChangedExternally(); // scrolling on the bar shows the OSD too
}

void AudioService::setMuted(bool muted) {
  if (!m_out.endpoint || !m_out.available) {
    return;
  }
  m_out.endpoint->setMuted(muted);
  onEndpointChanged(false, static_cast<float>(m_out.volume), muted);
}

void AudioService::toggleMute() { setMuted(!m_out.muted); }

// ── input ───────────────────────────────────────────────────────────────────

void AudioService::setMicVolume(qreal value) {
  if (!m_in.endpoint || !m_in.available) {
    return;
  }
  value = std::round(std::clamp(value, 0.0, 1.0) * 100.0) / 100.0;
  m_in.endpoint->setVolume(static_cast<float>(value));
  onEndpointChanged(true, static_cast<float>(value), m_in.muted);
}

void AudioService::setMicMuted(bool muted) {
  if (!m_in.endpoint || !m_in.available) {
    return;
  }
  m_in.endpoint->setMuted(muted);
  onEndpointChanged(true, static_cast<float>(m_in.volume), muted);
}

void AudioService::toggleMicMute() { setMicMuted(!m_in.muted); }

// ── devices ─────────────────────────────────────────────────────────────────

void AudioService::refreshDevices() {
  auto toList = [](const std::vector<platform::AudioDevice>& devices) {
    QVariantList out;
    for (const auto& d : devices) {
      out.append(QVariantMap{{"id", QString::fromStdWString(d.id)},
                             {"name", QString::fromStdWString(d.name)},
                             {"isDefault", d.isDefault}});
    }
    return out;
  };
  const QVariantList outs = toList(platform::listAudioDevices(false));
  const QVariantList ins = toList(platform::listAudioDevices(true));
  if (outs == m_outputs && ins == m_inputs) {
    return;
  }
  m_outputs = outs;
  m_inputs = ins;
  emit devicesChanged();
}

void AudioService::setDefaultDevice(const QString& id) {
  platform::setDefaultAudioDevice(id.toStdWString());
  // The default-device notification rebinds the endpoints; refresh the list right away too.
  refreshDevices();
}

// ── per-app volume ──────────────────────────────────────────────────────────

void AudioService::setWatchSessions(bool on) {
  if (on == m_sessionTimer.isActive()) {
    return;
  }
  if (on) {
    refreshSessions();
    m_sessionTimer.start();
  } else {
    m_sessionTimer.stop();
  }
  emit sessionsChanged();
}

void AudioService::refreshSessions() {
  QVariantList list;
  for (const auto& s : platform::listAudioSessions()) {
    list.append(QVariantMap{{"id", QString::fromStdWString(s.id)},
                            {"name", QString::fromStdWString(s.name)},
                            {"exePath", QString::fromStdWString(s.exePath)},
                            {"volume", s.volume},
                            {"muted", s.muted},
                            {"active", s.active}});
  }
  if (list == m_sessions) {
    return;
  }
  m_sessions = list;
  emit sessionsChanged();
}

void AudioService::setSessionVolume(const QString& id, qreal value) {
  platform::setAudioSessionVolume(id.toStdWString(), static_cast<float>(std::clamp(value, 0.0, 1.0)));
  refreshSessions();
}

void AudioService::setSessionMuted(const QString& id, bool muted) {
  platform::setAudioSessionMuted(id.toStdWString(), muted);
  refreshSessions();
}
