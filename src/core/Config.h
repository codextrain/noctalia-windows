#pragma once

#include <QFileSystemWatcher>
#include <QJsonObject>
#include <QObject>
#include <QTimer>
#include <QVariantMap>

// Loads %LOCALAPPDATA%\noctalia-win\config.json, merges it over the defaults (same keys and
// default values as Noctalia's config.toml where a feature exists) and hot-reloads on save.
class Config : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariantMap shell READ shell NOTIFY changed)
  Q_PROPERTY(QVariantMap theme READ theme NOTIFY changed)
  Q_PROPERTY(QVariantMap bar READ bar NOTIFY changed)
  Q_PROPERTY(QVariantMap windows READ windows NOTIFY changed)
  Q_PROPERTY(QVariantMap launcher READ launcher NOTIFY changed)
  Q_PROPERTY(QVariantMap keybinds READ keybinds NOTIFY changed)
  Q_PROPERTY(QVariantMap weather READ weather NOTIFY changed)
  Q_PROPERTY(QVariantMap location READ location NOTIFY changed)
  Q_PROPERTY(QVariantMap osd READ osd NOTIFY changed)
  Q_PROPERTY(QString path READ path CONSTANT)
  Q_PROPERTY(QString lastError READ lastError NOTIFY changed)

public:
  explicit Config(QObject* parent = nullptr);

  static QJsonObject defaults();

  QVariantMap shell() const { return m_data.value("shell").toObject().toVariantMap(); }
  QVariantMap theme() const { return m_data.value("theme").toObject().toVariantMap(); }
  QVariantMap bar() const { return m_data.value("bar").toObject().toVariantMap(); }
  QVariantMap windows() const { return m_data.value("windows").toObject().toVariantMap(); }
  QVariantMap launcher() const { return m_data.value("launcher").toObject().toVariantMap(); }
  QVariantMap keybinds() const { return m_data.value("keybinds").toObject().toVariantMap(); }
  QVariantMap weather() const { return m_data.value("weather").toObject().toVariantMap(); }
  QVariantMap location() const { return m_data.value("location").toObject().toVariantMap(); }
  QVariantMap osd() const { return m_data.value("osd").toObject().toVariantMap(); }
  QString path() const { return m_path; }
  QString lastError() const { return m_lastError; }

  Q_INVOKABLE void reload();
  // Change one setting and save it to config.json, e.g. setValue("launcher.pinned", list).
  // Only that key is touched: the rest of the user's file is kept as is.
  Q_INVOKABLE bool setValue(const QString& dottedPath, const QVariant& value);
  Q_INVOKABLE void openInEditor() const;
  Q_INVOKABLE void openFolder() const;

signals:
  void changed();

private:
  void watch();

  QString m_path;
  QJsonObject m_data;
  QString m_lastError;
  QFileSystemWatcher m_watcher;
  QTimer m_debounce;
};
