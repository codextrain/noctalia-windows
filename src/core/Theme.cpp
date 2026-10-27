#include "core/Theme.h"

#include "core/Config.h"
#include "services/WallpaperService.h"

// Noctalia's own theme code (third_party/noctalia, MIT)
#include "render/core/color.h"
#include "theme/fixed_palette.h"
#include "theme/palette_generator.h"
#include "theme/scheme.h"

#include <QFile>
#include <cstring>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QJsonDocument>
#include <QSettings>
#include <QStyleHints>
#include <QDateTime>

namespace {

  namespace nt = noctalia::theme;

  // "surface_container_high" -> "surfaceContainerHigh" (the names QML uses)
  QString camel(const std::string& token) {
    QString out;
    bool upper = false;
    for (const char c : token) {
      if (c == '_') {
        upper = true;
      } else {
        out += upper ? QChar(c).toUpper() : QChar(c);
        upper = false;
      }
    }
    return out;
  }

  QVariantMap toColors(const nt::TokenMap& tokens) {
    QVariantMap out;
    for (const auto& [key, argb] : tokens) {
      out.insert(camel(key), QColor::fromRgb(static_cast<QRgb>(argb | 0xFF000000u)));
    }
    return out;
  }

  ::Color hexColor(const QJsonObject& o, const char* key) {
    const QColor c(o.value(QLatin1String(key)).toString());
    if (!c.isValid()) {
      return rgba(1, 0, 1);
    }
    return rgba(static_cast<float>(c.redF()), static_cast<float>(c.greenF()), static_cast<float>(c.blueF()));
  }

  nt::FixedPalette fixedFromJson(const QJsonObject& p) {
    nt::FixedPalette f;
    f.primary = hexColor(p, "primary");
    f.onPrimary = hexColor(p, "onPrimary");
    f.secondary = hexColor(p, "secondary");
    f.onSecondary = hexColor(p, "onSecondary");
    f.tertiary = hexColor(p, "tertiary");
    f.onTertiary = hexColor(p, "onTertiary");
    f.error = hexColor(p, "error");
    f.onError = hexColor(p, "onError");
    f.surface = hexColor(p, "surface");
    f.onSurface = hexColor(p, "onSurface");
    f.surfaceVariant = hexColor(p, "surfaceVariant");
    f.onSurfaceVariant = hexColor(p, "onSurfaceVariant");
    f.outline = hexColor(p, "outline");
    f.shadow = hexColor(p, "shadow");
    f.hover = p.contains("hover") ? hexColor(p, "hover") : f.tertiary;
    f.onHover = p.contains("onHover") ? hexColor(p, "onHover") : f.onTertiary;
    return f;
  }

  struct Generated {
    bool ok = false;
    QString error;
    QVariantMap dark;
    QVariantMap light;
  };
  QString wallpaperKey(const QString& path, const QString& scheme) {
    const QFileInfo file(path);
    return file.absoluteFilePath() + '|' + QString::number(file.lastModified().toMSecsSinceEpoch())
        + '|' + QString::number(file.size()) + '|' + scheme;
  }

} // namespace

Theme::Theme(Config* config, WallpaperService* wallpaper, QObject* parent)
    : QObject(parent), m_config(config), m_wallpaper(wallpaper) {
  QFile f(":/assets/palettes.json");
  if (f.open(QIODevice::ReadOnly)) {
    m_palettes = QJsonDocument::fromJson(f.readAll()).object();
  }
  connect(m_config, &Config::changed, this, &Theme::apply);
  if (m_wallpaper != nullptr) {
    connect(m_wallpaper, &WallpaperService::currentChanged, this, &Theme::apply);
  }

  // theme.mode = "auto" follows Windows' "app mode" (Settings > Personalization > Colors).
  m_systemPoll.setInterval(2000);
  connect(&m_systemPoll, &QTimer::timeout, this, [this] {
    if (systemPrefersDark() != m_dark) {
      apply();
    }
  });
  apply();
}

QStringList Theme::schemes() const {
  QStringList out;
  for (const auto& name : nt::kSchemeNames) {
    out.append(QString::fromLatin1(name.data(), static_cast<qsizetype>(name.size())));
  }
  return out;
}

bool Theme::systemPrefersDark() const {
#ifdef Q_OS_WIN
  QSettings reg(R"(HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize)",
                QSettings::NativeFormat);
  return reg.value("AppsUseLightTheme", 1).toInt() == 0;
#else
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
  return QGuiApplication::styleHints()->colorScheme() != Qt::ColorScheme::Light;
#else
  return true;
#endif
#endif
}

void Theme::publish(const QVariantMap& colors, const QString& name, bool isDark) {
  if (colors.isEmpty() || (colors == m_colors && name == m_palette && isDark == m_dark)) {
    return;
  }
  m_colors = colors;
  m_palette = name;
  m_dark = isDark;
  emit changed();
}

void Theme::apply() {
  const QVariantMap cfg = m_config->theme();
  const QString mode = cfg.value("mode", "dark").toString();
  const bool isDark = mode == "auto" ? systemPrefersDark() : mode != "light";
  if (mode == "auto") {
    m_systemPoll.start();
  } else {
    m_systemPoll.stop();
  }

  m_source = cfg.value("source", "builtin").toString();
  if ((m_source != "wallpaper" || m_wallpaper == nullptr || m_wallpaper->current().isEmpty()) && m_generating) {
    ++m_requestSerial;
    m_pendingKey.clear();
    m_generating = false;
  }
  if (m_source == "wallpaper" && m_wallpaper != nullptr && !m_wallpaper->current().isEmpty()) {
    const QString scheme = cfg.value("wallpaper_scheme", "m3-content").toString();
    const QString path = m_wallpaper->current();
    const QString key = wallpaperKey(path, scheme);
    if (key == m_generatedKey && !m_generatedDark.isEmpty()) {
      if (m_generating) { ++m_requestSerial; m_pendingKey.clear(); m_generating = false; }
      publish(isDark ? m_generatedDark : m_generatedLight, QStringLiteral("Papel de parede"), isDark);
      return;
    }
    generateFromWallpaper(path, scheme);
    if (!m_colors.isEmpty()) {
      return; // keep the current colors until the new ones are ready
    }
  }

  // builtin palette (also the fallback while the wallpaper palette is being generated)
  QString name = cfg.value("builtin", "Noctalia").toString();
  if (!m_palettes.contains(name)) {
    name = "Noctalia";
  }
  const QJsonObject pal = m_palettes.value(name).toObject().value(isDark ? "dark" : "light").toObject();
  publish(toColors(nt::expandFixedPaletteMode(fixedFromJson(pal), isDark)), name, isDark);
}

void Theme::generateFromWallpaper(const QString& path, const QString& schemeName) {
  const QString key = wallpaperKey(path, schemeName);
  if (m_generating && key == m_pendingKey) {
    return;
  }
  m_pendingKey = key;
  const quint64 requestSerial = ++m_requestSerial;
  m_lastError.clear();
  m_generating = true;
  emit changed();

  m_worker.run(
      [path, schemeName] {
        Generated g;
        QImage img(path);
        if (img.isNull()) {
          g.error = QStringLiteral("Não foi possível abrir o papel de parede");
          return g;
        }
        // Noctalia samples a 112x112 RGB thumbnail.
        img = img.scaled(112, 112, Qt::IgnoreAspectRatio, Qt::SmoothTransformation).convertToFormat(QImage::Format_RGB888);
        std::vector<uint8_t> rgb(112 * 112 * 3);
        for (int y = 0; y < 112; ++y) {
          memcpy(rgb.data() + y * 112 * 3, img.constScanLine(y), 112 * 3);
        }
        const auto scheme = nt::schemeFromString(schemeName.toStdString()).value_or(nt::Scheme::Content);
        auto result = nt::generate(rgb, scheme);
        if (!result) {
          g.error = QString::fromStdString(result.error());
          return g;
        }
        nt::GeneratedPalette palette = std::move(*result);
        nt::setGeneratedHoverTokens(palette);
        g.dark = toColors(palette.dark);
        g.light = toColors(palette.light);
        g.ok = true;
        return g;
      },
      this,
      [this, key, requestSerial](Generated g) {
        if (requestSerial != m_requestSerial || m_config->theme().value("source").toString() != "wallpaper") {
          return; // a newer request is running
        }
        m_generating = false;
        m_pendingKey.clear();
        if (!g.ok) {
          qWarning("noctalia-win: %s", qPrintable(g.error));
          m_lastError = g.error;
          emit changed();
          return;
        }
        m_generatedDark = g.dark;
        m_generatedLight = g.light;
        m_generatedKey = key;
        const QString mode = m_config->theme().value("mode", "dark").toString();
        const bool isDark = mode == "auto" ? systemPrefersDark() : mode != "light";
        publish(isDark ? m_generatedDark : m_generatedLight, QStringLiteral("Papel de parede"), isDark);
        emit changed();
      });
}

void Theme::setBuiltin(const QString& name) {
  m_config->setValue("theme.source", "builtin");
  m_config->setValue("theme.builtin", name);
}

void Theme::setMode(const QString& mode) { m_config->setValue("theme.mode", mode); }

void Theme::useWallpaper(const QString& scheme) {
  m_config->setValue("theme.wallpaper_scheme", scheme.isEmpty() ? QStringLiteral("m3-content") : scheme);
  m_config->setValue("theme.source", "wallpaper");
}

void Theme::refreshWallpaperPalette() {
  if (m_config->theme().value("source").toString() != "wallpaper") return;
  ++m_requestSerial;
  m_generatedKey.clear();
  m_pendingKey.clear();
  m_generating = false;
  apply();
}
