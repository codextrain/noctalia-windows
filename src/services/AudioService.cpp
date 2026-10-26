#include "services/AudioService.h"

#include <QPointer>

#include <algorithm>
#include <cmath>

AudioService::AudioService(QObject* parent) : QObject(parent) {
  QPointer<AudioService> self(this);
  // Core Audio calls these from its own threads: hop back to the UI thread before touching state.
  m_endpoint = platform::AudioEndpoint::create(
      [self](float v, bool m) {
        if (self) {
          QMetaObject::invokeMethod(self.data(), [self, v, m] { if (self) self->onEndpointChanged(v, m); },
                                    Qt::QueuedConnection);
        }
      },
      [self] {
        if (self) {
          QMetaObject::invokeMethod(self.data(), [self] { if (self) self->rebind(); }, Qt::QueuedConnection);
        }
      });
  rebind();
}

AudioService::~AudioService() = default;

void AudioService::rebind() {
  if (!m_endpoint) {
    return;
  }
  m_endpoint->rebind();
  m_available = m_endpoint->valid();
  m_deviceName = QString::fromStdWString(m_endpoint->deviceName());
  m_volume = m_endpoint->volume();
  m_muted = m_endpoint->muted();
  emit changed();
}

void AudioService::onEndpointChanged(float volume, bool muted) {
  if (std::abs(volume - m_volume) < 0.0005 && muted == m_muted) {
    return;
  }
  m_volume = volume;
  m_muted = muted;
  emit changed();
}

void AudioService::setVolume(qreal value) {
  if (!m_endpoint || !m_available) {
    return;
  }
  // Snap to whole percents like Noctalia's volume widget.
  value = std::round(std::clamp(value, 0.0, 1.0) * 100.0) / 100.0;
  m_endpoint->setVolume(static_cast<float>(value));
  if (m_muted && value > 0) {
    m_endpoint->setMuted(false);
  }
  onEndpointChanged(static_cast<float>(value), m_muted && value <= 0);
}

void AudioService::changeVolume(qreal delta) { setVolume(m_volume + delta); }

void AudioService::setMuted(bool muted) {
  if (!m_endpoint || !m_available) {
    return;
  }
  m_endpoint->setMuted(muted);
  onEndpointChanged(static_cast<float>(m_volume), muted);
}

void AudioService::toggleMute() { setMuted(!m_muted); }
