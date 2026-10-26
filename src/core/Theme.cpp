#include "core/Theme.h"

#include "core/Config.h"

#include <QFile>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QSettings>
#include <QStyleHints>

#include <algorithm>
#include <cmath>

namespace {

  // ── small color helpers (ported from noctalia/src/theme) ───────────────────

  QColor fromHsl(double h, double s, double l) {
    // Noctalia's HSL hue is in degrees 0..360; QColor wants 0..1 (or -1 for achromatic).
    return QColor::fromHslF(static_cast<float>(std::fmod(h + 360.0, 360.0) / 360.0),
                            static_cast<float>(std::clamp(s, 0.0, 1.0)),
                            static_cast<float>(std::clamp(l, 0.0, 1.0)));
  }

  struct Hsl {
    double h, s, l;
  };

  Hsl toHsl(const QColor& c) {
    float h = 0, s = 0, l = 0;
    c.getHslF(&h, &s, &l);
    return {h < 0 ? 0.0 : h * 360.0, s, l};
  }

  QColor mix(const QColor& a, const QColor& b, double t) {
    auto m = [t](int x, int y) { return static_cast<int>(x + (y - x) * t); };
    return QColor(m(a.red(), b.red()), m(a.green(), b.green()), m(a.blue(), b.blue()));
  }

  double luminance(const QColor& c) {
    auto lin = [](int v) {
      const double n = v / 255.0;
      return n <= 0.04045 ? n / 12.92 : std::pow((n + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * lin(c.red()) + 0.7152 * lin(c.green()) + 0.0722 * lin(c.blue());
  }

  double contrast(const QColor& a, const QColor& b) {
    const double la = luminance(a), lb = luminance(b);
    return (std::max(la, lb) + 0.05) / (std::min(la, lb) + 0.05);
  }

  // Nudge `fg` lighter or darker (keeping hue/saturation) until it reaches `ratio` against `bg`.
  // Simplified version of noctalia's OKLCH-based ensureContrast.
  QColor ensureContrast(const QColor& fg, const QColor& bg, double ratio) {
    if (contrast(fg, bg) >= ratio) {
      return fg;
    }
    const Hsl hsl = toHsl(fg);
    const bool goLighter = luminance(bg) < 0.5;
    for (int step = 1; step <= 100; ++step) {
      const double l = goLighter ? hsl.l + (1.0 - hsl.l) * step / 100.0 : hsl.l * (1.0 - step / 100.0);
      const QColor c = fromHsl(hsl.h, hsl.s, l);
      if (contrast(c, bg) >= ratio) {
        return c;
      }
    }
    return goLighter ? QColor(Qt::white) : QColor(Qt::black);
  }

  QColor hex(const QJsonObject& o, const char* key, const QColor& fallback = QColor("#ff00ff")) {
    const QColor c(o.value(QLatin1String(key)).toString());
    return c.isValid() ? c : fallback;
  }

} // namespace

QVariantMap Theme::expand(const QJsonObject& p, bool isDark) {
  const QColor primary = hex(p, "primary"), onPrimary = hex(p, "onPrimary");
  const QColor secondary = hex(p, "secondary"), onSecondary = hex(p, "onSecondary");
  const QColor tertiary = hex(p, "tertiary"), onTertiary = hex(p, "onTertiary");
  const QColor error = hex(p, "error"), onError = hex(p, "onError");
  const QColor surface = hex(p, "surface"), onSurface = hex(p, "onSurface");
  const QColor surfaceVariant = hex(p, "surfaceVariant"), onSurfaceVariant = hex(p, "onSurfaceVariant");
  const QColor outlineRaw = hex(p, "outline"), shadow = hex(p, "shadow");
  const QColor hover = hex(p, "hover", tertiary), onHover = hex(p, "onHover", onTertiary);

  // A container is the accent mixed into the surface (keeps a pastel a pastel).
  auto container = [&](const QColor& accent) { return mix(surface, accent, 0.30); };
  const QColor primaryContainer = container(primary);
  const QColor secondaryContainer = container(secondary);
  const QColor tertiaryContainer = container(tertiary);
  const QColor errorContainer = container(error);

  const Hsl sv = toHsl(surfaceVariant);
  const Hsl s = toHsl(surface);
  const QColor surfaceContainer = surfaceVariant;
  const QColor surfaceContainerLowest = mix(surface, surfaceVariant, 0.2);
  const QColor surfaceContainerLow = mix(surface, surfaceVariant, 0.5);
  const QColor surfaceContainerHigh =
      isDark ? fromHsl(sv.h, sv.s, std::min(sv.l + 0.04, 0.40)) : fromHsl(sv.h, sv.s, std::max(sv.l - 0.04, 0.60));
  const QColor surfaceContainerHighest =
      isDark ? fromHsl(sv.h, sv.s, std::min(sv.l + 0.08, 0.45)) : fromHsl(sv.h, sv.s, std::max(sv.l - 0.08, 0.55));
  const QColor surfaceDim =
      isDark ? fromHsl(s.h, s.s, std::max(s.l - 0.04, 0.02)) : fromHsl(sv.h, sv.s, std::max(sv.l - 0.12, 0.50));
  const QColor surfaceBright =
      isDark ? fromHsl(sv.h, sv.s, std::min(sv.l + 0.12, 0.50)) : fromHsl(s.h, s.s, std::min(s.l + 0.03, 0.98));

  return QVariantMap{
      {"primary", primary},
      {"onPrimary", onPrimary},
      {"primaryContainer", primaryContainer},
      {"onPrimaryContainer", ensureContrast(primary, primaryContainer, 4.5)},
      {"secondary", secondary},
      {"onSecondary", onSecondary},
      {"secondaryContainer", secondaryContainer},
      {"onSecondaryContainer", ensureContrast(secondary, secondaryContainer, 4.5)},
      {"tertiary", tertiary},
      {"onTertiary", onTertiary},
      {"tertiaryContainer", tertiaryContainer},
      {"onTertiaryContainer", ensureContrast(tertiary, tertiaryContainer, 4.5)},
      {"error", error},
      {"onError", onError},
      {"errorContainer", errorContainer},
      {"onErrorContainer", ensureContrast(error, errorContainer, 4.5)},
      {"surface", surface},
      {"onSurface", onSurface},
      {"surfaceVariant", surfaceVariant},
      {"onSurfaceVariant", onSurfaceVariant},
      {"surfaceDim", surfaceDim},
      {"surfaceBright", surfaceBright},
      {"surfaceContainerLowest", surfaceContainerLowest},
      {"surfaceContainerLow", surfaceContainerLow},
      {"surfaceContainer", surfaceContainer},
      {"surfaceContainerHigh", surfaceContainerHigh},
      {"surfaceContainerHighest", surfaceContainerHighest},
      {"outline", ensureContrast(outlineRaw, surface, 3.0)},
      {"outlineVariant", outlineRaw},
      {"shadow", shadow},
      {"hover", hover},
      {"onHover", onHover},
      {"scrim", QColor(Qt::black)},
  };
}

Theme::Theme(Config* config, QObject* parent) : QObject(parent), m_config(config) {
  QFile f(":/assets/palettes.json");
  if (f.open(QIODevice::ReadOnly)) {
    m_palettes = QJsonDocument::fromJson(f.readAll()).object();
  }
  connect(m_config, &Config::changed, this, &Theme::apply);

  // theme.mode = "auto" follows Windows' "app mode" (Settings > Personalization > Colors).
  m_systemPoll.setInterval(2000);
  connect(&m_systemPoll, &QTimer::timeout, this, [this] {
    if (systemPrefersDark() != m_dark) {
      apply();
    }
  });
  apply();
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

void Theme::apply() {
  const QVariantMap cfg = m_config->theme();
  const QString mode = cfg.value("mode", "dark").toString();
  QString name = cfg.value("builtin", "Noctalia").toString();
  if (!m_palettes.contains(name)) {
    name = "Noctalia";
  }

  const bool isDark = mode == "auto" ? systemPrefersDark() : mode != "light";
  if (mode == "auto") {
    m_systemPoll.start();
  } else {
    m_systemPoll.stop();
  }

  const QJsonObject pal = m_palettes.value(name).toObject().value(isDark ? "dark" : "light").toObject();
  QVariantMap colors = expand(pal, isDark);
  if (colors == m_colors && name == m_palette && isDark == m_dark) {
    return;
  }
  m_colors = std::move(colors);
  m_palette = name;
  m_dark = isDark;
  emit changed();
}
