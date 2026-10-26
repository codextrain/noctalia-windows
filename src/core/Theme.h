#pragma once

#include <QColor>
#include <QJsonObject>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QVariantMap>

class Config;

// Holds the active color palette. The 10 built-in palettes are copied from Noctalia
// (assets/palettes.json) and expanded with the same rules as noctalia/src/theme/fixed_palette.cpp,
// so "surface_container", "primary_container" and friends match the Linux shell.
//
// QML: Theme.colors.primary, Theme.colors.surfaceContainerHigh, Theme.dark, ...
class Theme : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariantMap colors READ colors NOTIFY changed)
  Q_PROPERTY(bool dark READ dark NOTIFY changed)
  Q_PROPERTY(QString palette READ palette NOTIFY changed)
  Q_PROPERTY(QStringList palettes READ palettes CONSTANT)

public:
  explicit Theme(Config* config, QObject* parent = nullptr);

  QVariantMap colors() const { return m_colors; }
  bool dark() const { return m_dark; }
  QString palette() const { return m_palette; }
  QStringList palettes() const { return m_palettes.keys(); }

  // Expand Noctalia's 16 "fixed palette" colors into the full token set (exposed for tests).
  static QVariantMap expand(const QJsonObject& fixed, bool dark);

signals:
  void changed();

private:
  void apply();
  bool systemPrefersDark() const;

  Config* m_config = nullptr;
  QJsonObject m_palettes;
  QVariantMap m_colors;
  QString m_palette;
  bool m_dark = true;
  QTimer m_systemPoll; // for theme.mode = "auto"
};
