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

#include <functional>

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
           {"clipboard_enabled", true},
           {"clipboard_history_max_entries", 100},
       }},
      {"theme",
       QJsonObject{
           {"mode", "dark"},        // dark | light | auto (follows Windows)
           {"source", "builtin"},   // builtin | wallpaper (Material You from the wallpaper)
           {"wallpaper_scheme", "m3-content"}, // m3-content | m3-tonal-spot | m3-fruit-salad | m3-rainbow |
                                               // m3-monochrome | vibrant | faithful | soft | dysfunctional | muted
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
           {"start", QJsonArray{"launcher", "wallpaper", "workspaces"}},
           {"center", QJsonArray{"clock"}},
           // Same order as Noctalia's default (widgets not ported yet are skipped).
           {"end", QJsonArray{"media", "notifications", "clipboard", "network", "bluetooth", "volume", "brightness", "battery", "control-center", "session"}},
       }},
      {"launcher",
       QJsonObject{
           // Same names as Noctalia's [shell.launcher].
           {"width", 760},
           {"default_view", "recent"}, // recent | pinned | all
           {"app_grid", false},
           {"compact", false},
           {"show_icons", true},
           {"track_usage", true},
           {"sort_by_usage", true},
           {"provider_prefix", "/"},
           {"auto_paste", true}, // paste emoji / calculator results into the previous window
           {"pinned", QJsonArray{}},
       }},
      {"keybinds",
       QJsonObject{
           // Global shortcuts. Empty string = disabled. Examples: "Alt+Space", "Ctrl+Shift+K", "Win+Alt+V".
           {"launcher", "Alt+Space"},
           {"control-center", ""},
           {"clipboard", "Alt+V"},
           {"wallpaper", ""},
       }},
      {"wallpaper",
       QJsonObject{
           {"directory", ""},      // empty = Pictures\Wallpapers (or Pictures)
           {"fill_mode", "crop"},  // crop | fit | stretch | center | repeat | span
           {"recursive", true},
           {"automation", QJsonObject{{"enabled", false}, {"interval_seconds", 1800}, {"order", "random"}}},
       }},
      {"dock",
       QJsonObject{
           // Same keys as Noctalia's [dock]. Tip: with the dock on, set windows.autohide_taskbar = true.
           {"enabled", false},
           {"position", "bottom"}, // bottom | top
           {"icon_size", 48},
           {"item_spacing", 6},
           {"main_axis_padding", 16},
           {"cross_axis_padding", 8},
           {"background_opacity", 0.88},
           {"radius", 16},
           {"margin_edge", 8},
           {"shadow", true},
           {"show_running", true},
           {"reserve_space", true},
           {"magnification", true},
           {"magnification_scale", 1.45},
           {"inactive_opacity", 0.85},
           {"show_instance_count", true},
           {"launcher_position", "start"}, // none | start | end
           {"pinned", QJsonArray{}},
       }},
      {"desktop_widgets",
       QJsonObject{
           {"enabled", false},
           // Drag them with the mouse; positions are saved here. Types: clock, media, sysmon, weather
           {"widgets", QJsonArray{QJsonObject{{"type", "clock"}, {"x", 80}, {"y", 120}, {"scale", 1.0}}}},
       }},
      {"weather",
       QJsonObject{
           {"enabled", true},
           {"unit", "celsius"}, // celsius | fahrenheit
           {"refresh_minutes", 30},
       }},
      {"location",
       QJsonObject{
           {"auto_locate", false}, // true = find the city by IP address
           {"address", ""},        // e.g. "Joinville, SC"
       }},
      {"osd",
       QJsonObject{
           // Same keys as Noctalia's [osd].
           {"enabled", true},
           {"position", "top_right"}, // top_right | top_left | top_center | bottom_right | bottom_left | bottom_center
           {"hide_delay_ms", 1400},
           {"background_opacity", 0.97},
           {"border", true},
           {"offset_x", 20},
           {"offset_y", 8},
           {"kinds", QJsonObject{{"volume_output", true}, {"volume_input", true}, {"brightness", true}, {"wifi", true}, {"bluetooth", true}}},
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

bool Config::setValue(const QString& dottedPath, const QVariant& value) {
  const QStringList keys = dottedPath.split('.', Qt::SkipEmptyParts);
  if (keys.isEmpty()) {
    return false;
  }

  QJsonObject root;
  QFile in(m_path);
  if (in.open(QIODevice::ReadOnly)) {
    QJsonParseError pe{};
    const QJsonDocument doc = QJsonDocument::fromJson(in.readAll(), &pe);
    if (pe.error != QJsonParseError::NoError) {
      return false; // never overwrite a file the user is in the middle of editing
    }
    root = doc.object();
  }
  in.close();

  // Walk down creating objects as needed, then rebuild the chain on the way back up.
  std::function<QJsonObject(QJsonObject, int)> set = [&](QJsonObject obj, int depth) {
    const QString& key = keys[depth];
    if (depth == keys.size() - 1) {
      obj.insert(key, QJsonValue::fromVariant(value));
    } else {
      obj.insert(key, set(obj.value(key).toObject(), depth + 1));
    }
    return obj;
  };
  root = set(root, 0);

  QFile out(m_path);
  if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    return false;
  }
  out.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
  out.close();
  reload();
  return true;
}

void Config::openInEditor() const { QDesktopServices::openUrl(QUrl::fromLocalFile(m_path)); }

void Config::openFolder() const { QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(m_path).absolutePath())); }
