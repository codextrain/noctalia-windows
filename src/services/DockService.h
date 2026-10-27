#pragma once

#include <QObject>
#include <QTimer>
#include <QVariantList>

class Config;
class LauncherService;

// Noctalia's dock: pinned apps + running windows grouped by app.
// QML: Dock.items [{key, appId, name, icon, pinned, running, focused, windows: [{handle, title,
//      minimized, focused}]}], Dock.activate(key), Dock.setPinned(appId, on), Dock.closeAll(key)
class DockService : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariantList items READ items NOTIFY changed)

public:
  DockService(Config* config, LauncherService* launcher, QObject* parent = nullptr);

  QVariantList items() const { return m_items; }

  Q_INVOKABLE void refresh();
  // Click: launch when not running, focus it, minimize when already focused, or cycle windows.
  Q_INVOKABLE void activate(const QString& key);
  Q_INVOKABLE void launchNew(const QString& key);
  Q_INVOKABLE void focusWindow(const QString& handle);
  Q_INVOKABLE void closeAll(const QString& key);
  Q_INVOKABLE void setPinned(const QString& appId, bool pinned);
  Q_INVOKABLE bool isPinned(const QString& appId) const;

signals:
  void changed();

private:
  QVariantMap find(const QString& key) const;

  Config* m_config = nullptr;
  LauncherService* m_launcher = nullptr;
  QVariantList m_items;
  QTimer m_timer;
  QHash<QString, int> m_cycle; // key -> next window index when an app has several
};
