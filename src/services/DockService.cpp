#include "services/DockService.h"

#include "core/Config.h"
#include "platform/Platform.h"
#include "services/LauncherService.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QUrl>
#include <QVariantMap>

namespace {
  QString fileIconUrl(const QString& path) {
    return QStringLiteral("image://icons/file/%1").arg(QString::fromLatin1(QUrl::toPercentEncoding(path)));
  }
} // namespace

DockService::DockService(Config* config, LauncherService* launcher, QObject* parent)
    : QObject(parent), m_config(config), m_launcher(launcher) {
  m_timer.setInterval(1000);
  connect(&m_timer, &QTimer::timeout, this, &DockService::refresh);
  connect(m_config, &Config::changed, this, [this] {
    const bool on = m_config->dock().value("enabled", false).toBool();
    if (on && !m_timer.isActive()) {
      m_timer.start();
      refresh();
    } else if (!on) {
      m_timer.stop();
    }
  });
  connect(m_launcher, &LauncherService::appsChanged, this, &DockService::refresh);
  if (m_config->dock().value("enabled", false).toBool()) {
    m_timer.start();
  }
  refresh();
}

bool DockService::isPinned(const QString& appId) const {
  return m_config->dock().value("pinned").toStringList().contains(appId);
}

void DockService::setPinned(const QString& appId, bool pinned) {
  QStringList ids = m_config->dock().value("pinned").toStringList();
  if (pinned == ids.contains(appId)) {
    return;
  }
  if (pinned) {
    ids.append(appId);
  } else {
    ids.removeAll(appId);
  }
  m_config->setValue("dock.pinned", ids);
  refresh();
}

void DockService::refresh() {
  const QStringList pinned = m_config->dock().value("pinned").toStringList();
  const bool showRunning = m_config->dock().value("show_running", true).toBool();

  QList<QVariantMap> items;
  QHash<QString, int> index; // key -> position in items

  for (const QString& id : pinned) {
    const QVariantMap app = m_launcher->appInfo(id);
    if (app.isEmpty()) {
      continue;
    }
    index.insert("app:" + id, static_cast<int>(items.size()));
    items.append(QVariantMap{{"key", "app:" + id}, {"appId", id}, {"name", app.value("name")}, {"icon", app.value("icon")},
                             {"pinned", true}, {"running", false}, {"focused", false}, {"windows", QVariantList{}}});
  }

  for (const auto& w : platform::enumerateWindows()) {
    if (!w.onCurrentDesktop) {
      continue;
    }
    const QString exe = QString::fromStdWString(w.exePath);
    const QString appId = m_launcher->appForWindow(QString::fromStdWString(w.appId), exe);
    const QString key = appId.isEmpty() ? "exe:" + exe.toLower() : "app:" + appId;
    if (!index.contains(key)) {
      if (!showRunning) {
        continue;
      }
      QVariantMap item;
      if (!appId.isEmpty()) {
        const QVariantMap app = m_launcher->appInfo(appId);
        item = {{"key", key}, {"appId", appId}, {"name", app.value("name")}, {"icon", app.value("icon")}};
      } else {
        item = {{"key", key}, {"appId", QString()}, {"name", QFileInfo(exe).completeBaseName()}, {"icon", fileIconUrl(exe)}};
      }
      item["pinned"] = false;
      item["running"] = false;
      item["focused"] = false;
      item["windows"] = QVariantList{};
      item["exePath"] = exe;
      index.insert(key, static_cast<int>(items.size()));
      items.append(item);
    }
    QVariantMap& item = items[index.value(key)];
    QVariantList windows = item.value("windows").toList();
    windows.append(QVariantMap{{"handle", QString::number(w.handle)},
                               {"title", QString::fromStdWString(w.title)},
                               {"minimized", w.minimized},
                               {"focused", w.focused}});
    item["windows"] = windows;
    item["running"] = true;
    item["focused"] = item.value("focused").toBool() || w.focused;
    if (!item.contains("exePath")) {
      item["exePath"] = exe;
    }
  }

  QVariantList out;
  for (const QVariantMap& m : items) {
    out.append(m);
  }
  if (out != m_items) {
    m_items = out;
    emit changed();
  }
}

QVariantMap DockService::find(const QString& key) const {
  for (const QVariant& v : m_items) {
    const QVariantMap m = v.toMap();
    if (m.value("key").toString() == key) {
      return m;
    }
  }
  return {};
}

void DockService::activate(const QString& key) {
  const QVariantMap item = find(key);
  if (item.isEmpty()) {
    return;
  }
  const QVariantList windows = item.value("windows").toList();
  if (windows.isEmpty()) {
    launchNew(key);
    return;
  }
  if (windows.size() == 1) {
    const QVariantMap w = windows.first().toMap();
    const auto handle = static_cast<std::uintptr_t>(w.value("handle").toString().toULongLong());
    if (w.value("focused").toBool() && !w.value("minimized").toBool()) {
      platform::minimizeWindow(handle); // like the taskbar: click again to minimize
    } else {
      platform::activateWindow(handle);
    }
  } else {
    int next = m_cycle.value(key, 0) % static_cast<int>(windows.size());
    platform::activateWindow(static_cast<std::uintptr_t>(windows[next].toMap().value("handle").toString().toULongLong()));
    m_cycle.insert(key, next + 1);
  }
  QTimer::singleShot(250, this, &DockService::refresh);
}

void DockService::launchNew(const QString& key) {
  const QVariantMap item = find(key);
  const QString appId = item.value("appId").toString();
  if (!appId.isEmpty()) {
    m_launcher->launch(appId);
  } else if (!item.value("exePath").toString().isEmpty()) {
    QDesktopServices::openUrl(QUrl::fromLocalFile(item.value("exePath").toString()));
  }
}

void DockService::focusWindow(const QString& handle) {
  platform::activateWindow(static_cast<std::uintptr_t>(handle.toULongLong()));
  QTimer::singleShot(250, this, &DockService::refresh);
}

void DockService::closeAll(const QString& key) {
  for (const QVariant& w : find(key).value("windows").toList()) {
    platform::closeWindow(static_cast<std::uintptr_t>(w.toMap().value("handle").toString().toULongLong()));
  }
  QTimer::singleShot(500, this, &DockService::refresh);
}
