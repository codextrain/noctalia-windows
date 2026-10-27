#pragma once
#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QVariantList>

// Read once on demand, outside the UI thread. No network or administrator access.
class HardwareService : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool busy READ busy NOTIFY changed)
  Q_PROPERTY(QString error READ error NOTIFY changed)
  Q_PROPERTY(QVariantList components READ components NOTIFY changed)
public:
  explicit HardwareService(QObject* parent = nullptr);
  bool busy() const { return m_busy; }
  QString error() const { return m_error; }
  QVariantList components() const { return m_components; }
  Q_INVOKABLE void refresh();
  Q_INVOKABLE void ensureLoaded();
private:
  void fail(const QString& message);
  QProcess m_process;
  QTimer m_timeout;
  bool m_busy = false;
  bool m_loaded = false;
  QString m_error;
  QVariantList m_components;
signals:
  void changed();
};
