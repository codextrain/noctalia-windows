#include "core/Glyphs.h"

#include <QDebug>
#include <QFile>
#include <QFontDatabase>
#include <QJsonDocument>
#include <QJsonObject>

Glyphs::Glyphs(QObject* parent) : QObject(parent) {
  const int id = QFontDatabase::addApplicationFont(":/assets/fonts/noctalia-tabler.ttf");
  const QStringList families = QFontDatabase::applicationFontFamilies(id);
  m_family = families.isEmpty() ? QStringLiteral("noctalia-tabler-icons") : families.first();

  QFile f(":/assets/glyphs.json");
  if (!f.open(QIODevice::ReadOnly)) {
    qWarning() << "glyphs.json missing from resources";
    return;
  }
  const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
  const QJsonObject cps = root.value("codepoints").toObject();
  m_codepoints.reserve(cps.size());
  for (auto it = cps.begin(); it != cps.end(); ++it) {
    m_codepoints.insert(it.key(), static_cast<char32_t>(it.value().toInt()));
  }
  const QJsonObject aliases = root.value("aliases").toObject();
  for (auto it = aliases.begin(); it != aliases.end(); ++it) {
    m_aliases.insert(it.key(), it.value().toString());
  }
}

bool Glyphs::has(const QString& name) const {
  return m_codepoints.contains(name) || m_aliases.contains(name);
}

QString Glyphs::get(const QString& name) const {
  auto it = m_codepoints.constFind(name);
  if (it == m_codepoints.constEnd()) {
    it = m_codepoints.constFind(m_aliases.value(name));
  }
  if (it == m_codepoints.constEnd()) {
    it = m_codepoints.constFind(QStringLiteral("question-mark"));
    if (it == m_codepoints.constEnd()) {
      return QStringLiteral("?");
    }
  }
  const char32_t cp = it.value();
  return QString::fromUcs4(&cp, 1);
}
