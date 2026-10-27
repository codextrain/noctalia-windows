#pragma once

#include "core/Worker.h"

#include <QColor>
#include <QJsonObject>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QVariantMap>

class Config;
class WallpaperService;

// The active color palette, computed with Noctalia's own theme code (third_party/noctalia):
//   theme.source = "builtin"   -> one of the 10 built-in palettes (theme.builtin)
//   theme.source = "wallpaper" -> generated from the current wallpaper (theme.wallpaper_scheme:
//                                 m3-content, m3-tonal-spot, m3-fruit-salad, m3-rainbow,
//                                 m3-monochrome, vibrant, faithful, soft, dysfunctional, muted)
//
// QML: Theme.colors.primary, Theme.colors.surfaceContainerHigh, Theme.dark, ...
class Theme : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariantMap colors READ colors NOTIFY changed)
  Q_PROPERTY(bool dark READ dark NOTIFY changed)
  Q_PROPERTY(QString palette READ palette NOTIFY changed)
  Q_PROPERTY(QString source READ source NOTIFY changed)
  Q_PROPERTY(bool generating READ generating NOTIFY changed)
  Q_PROPERTY(QString lastError READ lastError NOTIFY changed)
  Q_PROPERTY(QStringList palettes READ palettes CONSTANT)
  Q_PROPERTY(QStringList schemes READ schemes CONSTANT)

public:
  Theme(Config* config, WallpaperService* wallpaper, QObject* parent = nullptr);

  QVariantMap colors() const { return m_colors; }
  bool dark() const { return m_dark; }
  QString palette() const { return m_palette; }
  QString source() const { return m_source; }
  bool generating() const { return m_generating; }
  QString lastError() const { return m_lastError; }
  QStringList palettes() const { return m_palettes.keys(); }
  QStringList schemes() const;

  // Convenience for QML settings: change and save.
  Q_INVOKABLE void setBuiltin(const QString& name);
  Q_INVOKABLE void setMode(const QString& mode);
  Q_INVOKABLE void useWallpaper(const QString& scheme);
  Q_INVOKABLE void refreshWallpaperPalette();

signals:
  void changed();

private:
  void apply();
  void generateFromWallpaper(const QString& path, const QString& scheme);
  void publish(const QVariantMap& colors, const QString& name, bool dark);
  bool systemPrefersDark() const;

  Config* m_config = nullptr;
  WallpaperService* m_wallpaper = nullptr;
  Worker m_worker{"theme"};
  QJsonObject m_palettes;
  QVariantMap m_colors;
  QString m_palette;
  QString m_source = "builtin";
  bool m_dark = true;
  bool m_generating = false;
  // last generated wallpaper palette: both modes, keyed by "path|scheme"
  QString m_generatedKey;
  QString m_pendingKey;
  QString m_lastError;
  quint64 m_requestSerial = 0;
  QVariantMap m_generatedDark;
  QVariantMap m_generatedLight;
  QTimer m_systemPoll; // for theme.mode = "auto"
};
