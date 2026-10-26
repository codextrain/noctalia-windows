#pragma once

#include <QHash>
#include <QObject>
#include <QString>

// Icon lookup for the Tabler icon font Noctalia ships (assets/fonts/noctalia-tabler.ttf).
// Accepts Tabler names ("search", "battery-4") and Noctalia's semantic aliases
// ("volume-high", "shutdown", "wallpaper-selector"...), same table as glyph_registry.cpp.
//
// QML: Text { font.family: Glyphs.family; text: Glyphs.get("search") }
class Glyphs : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString family READ family CONSTANT)

public:
  explicit Glyphs(QObject* parent = nullptr);

  QString family() const { return m_family; }
  Q_INVOKABLE QString get(const QString& name) const;
  Q_INVOKABLE bool has(const QString& name) const;

private:
  QString m_family;
  QHash<QString, char32_t> m_codepoints;
  QHash<QString, QString> m_aliases;
};
