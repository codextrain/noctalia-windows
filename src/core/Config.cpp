#include "core/Config.h"

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QStandardPaths>
#include <QUrl>

namespace {

  // Recursively merge `over` into `base` (objects merge, everything else replaces).
  QJsonObject deepMerge(QJsonObject base, const QJsonObject& over) {
    for (auto it = over.begin(); it != over.end(); ++it) {
      const QJsonValue existing = base.value(it.key());
      if (existing.isObject() && it.value().isObject()) {
        base.insert(it.key(), deepMerge(existing.toObject(), it.value().toObject()));
      } else {
        base.insert(it.key(), it.value());
      }
    }
    return base;
  }

  QString configPath() {
    const QByteArray overridePath = qgetenv("NOCTALIA_CONFIG");
    if (!overridePath.isEmpty()) {
      return QString::fromLocal8Bit(overridePath);
    }
    // Windows: %LOCALAPPDATA%\noctalia-win\config.json
    const QString base = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    return QDir(base).filePath("noctalia-win/config.json");
  }

} // namespace

QJsonObject Config::defaults() {
  // Values mirror Noctalia's example.toml defaults ([shell], [theme], [bar.main]).
  return QJsonObject{
      {"shell",
       QJsonObject{
           {"font_family", ""}, // empty = Segoe UI Variable / system default
           {"time_format", "HH:mm"},
           {"date_format", "dddd, d 'de' MMMM"},
           {"corner_radius_scale", 1.0},
           {"ui_scale", 1.0},
           {"animation_enabled", true},
           {"animation_speed", 1.0},
       }},
      {"theme",
       QJsonObject{
           {"mode", "dark"},        // dark | light | auto (follows Windows)
           {"builtin", "Noctalia"}, // Ayu | Catppuccin | Dracula | Eldritch | Gruvbox | Kanagawa |
                                    // Noctalia | Nord | Rosé Pine | Tokyo-Night
       }},
      {"bar",
       QJsonObject{
           {"position", "top"}, // top | bottom
           {"thickness", 34},
           {"background_opacity", 1.0},
           {"radius", 12},
           {"margin_ends", 180},
           {"margin_edge", 10},
           {"margin_opposite_edge", 6}, // gap below the bar: room for the shadow (Windows has no window gaps)
           {"padding", 14},
           {"widget_spacing", 6},
           {"shadow", true},
           {"reserve_space", true},
           {"capsule", false},
           {"start", QJsonArray{"launcher", "workspaces"}},
           {"center", QJsonArray{"clock"}},
           // Same order as Noctalia's default (widgets not ported yet are skipped).
           {"end", QJsonArray{"notifications", "network", "volume", "battery", "control-center", "session"}},
       }},
      {"windows",
       QJsonObject{
           {"autohide_taskbar", false},   // auto-hide the Windows taskbar while the shell runs
           {"hide_on_fullscreen", true},  // hide the bar while a game/video is fullscreen
       }},
  };
}

Config::Config(QObject* parent) : QObject(parent), m_path(configPath()) {
  m_debounce.setSingleShot(true);
  m_debounce.setInterval(150);
  connect(&m_debounce, &QTimer::timeout, this, &Config::reload);
  connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this] { m_debounce.start(); });
  connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this] { m_debounce.start(); });

  // First run: write the defaults so there is something to edit.
  if (!QFileInfo::exists(m_path)) {
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QFile f(m_path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      f.write(QJsonDocument(defaults()).toJson(QJsonDocument::Indented));
    }
  }
  reload();
}

void Config::reload() {
  QJsonObject merged = defaults();
  QString error;

  QFile f(m_path);
  if (f.open(QIODevice::ReadOnly)) {
    QJsonParseError pe{};
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &pe);
    if (pe.error != QJsonParseError::NoError) {
      error = QStringLiteral("config.json: %1 (posição %2)").arg(pe.errorString()).arg(pe.offset);
      // Keep the last good config instead of falling back to defaults mid-edit.
      if (!m_data.isEmpty()) {
        merged = m_data;
      }
    } else if (doc.isObject()) {
      merged = deepMerge(merged, doc.object());
    }
  }

  const bool same = merged == m_data && error == m_lastError;
  m_data = merged;
  m_lastError = error;
  watch();
  if (!same) {
    emit changed();
  }
}

void Config::watch() {
  // Editors often replace the file instead of writing in place, which drops the watch.
  if (!m_watcher.files().contains(m_path) && QFileInfo::exists(m_path)) {
    m_watcher.addPath(m_path);
  }
  const QString dir = QFileInfo(m_path).absolutePath();
  if (!m_watcher.directories().contains(dir) && QFileInfo::exists(dir)) {
    m_watcher.addPath(dir);
  }
}

void Config::openInEditor() const { QDesktopServices::openUrl(QUrl::fromLocalFile(m_path)); }

void Config::openFolder() const { QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(m_path).absolutePath())); }
