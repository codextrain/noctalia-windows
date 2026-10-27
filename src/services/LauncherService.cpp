#include "services/LauncherService.h"

#include "core/Config.h"
#include "core/MathEval.h"
#include "platform/Platform.h"

#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QStandardPaths>
#include <QUrl>

#include <algorithm>
#include <cmath>
#include <thread>

namespace {

  // Lowercase and strip accents so "configuracoes" finds "Configurações".
  QString fold(const QString& s) {
    const QString d = s.normalized(QString::NormalizationForm_D);
    QString out;
    out.reserve(d.size());
    for (const QChar c : d) {
      if (c.category() != QChar::Mark_NonSpacing) {
        out += c.toLower();
      }
    }
    return out;
  }

  QString initialsOf(const QString& folded) {
    QString out;
    bool atStart = true;
    for (const QChar c : folded) {
      if (c.isLetterOrNumber()) {
        if (atStart) {
          out += c;
        }
        atStart = false;
      } else {
        atStart = true;
      }
    }
    return out;
  }

  // Match one search term against a name. -1 = no match; higher = better.
  double termScore(const QString& name, const QString& initials, const QString& term) {
    if (term.isEmpty()) {
      return 50;
    }
    if (name == term) {
      return 100;
    }
    if (name.startsWith(term)) {
      return 90 - std::min<int>(name.size() - term.size(), 20) * 0.25;
    }
    // start of a word: "code" in "visual studio code"
    for (int i = 1; i < name.size(); ++i) {
      if (!name[i - 1].isLetterOrNumber() && name.mid(i).startsWith(term)) {
        return 80 - i * 0.1;
      }
    }
    if (term.size() >= 2 && initials.startsWith(term)) {
      return 72;
    }
    const int idx = name.indexOf(term);
    if (idx >= 0) {
      return 60 - idx * 0.2;
    }
    // fuzzy: letters in order ("vsc" -> "VS Code")
    int pos = 0;
    int last = -1;
    int gaps = 0;
    for (const QChar c : term) {
      const int found = name.indexOf(c, pos);
      if (found < 0) {
        return -1;
      }
      if (last >= 0) {
        gaps += found - last - 1;
      }
      last = found;
      pos = found + 1;
    }
    return std::max(5.0, 40.0 - gaps * 1.5);
  }

  QString iconUrl(const QString& kind, const QString& value) {
    return QStringLiteral("image://icons/%1/%2").arg(kind, QString::fromLatin1(QUrl::toPercentEncoding(value)));
  }

} // namespace

LauncherService::LauncherService(Config* config, QObject* parent) : QObject(parent), m_config(config) {
  connect(m_config, &Config::changed, this, &LauncherService::prefixChanged);
  loadUsage();
  refreshApps();
}

LauncherService::~LauncherService() = default;

QString LauncherService::prefix() const {
  const QString p = m_config->launcher().value("provider_prefix", "/").toString();
  return p.isEmpty() ? QStringLiteral("/") : p;
}

void LauncherService::refreshApps() {
  if (m_loading) {
    return;
  }
  m_loading = true;
  emit appsChanged();

  // Listing the Start-menu apps takes a few hundred ms: do it off the UI thread.
  QPointer<LauncherService> self(this);
  std::thread([self] {
    platform::initializeThread();
    std::vector<platform::AppEntry> entries = platform::enumerateApps();
    if (!self) {
      return;
    }
    QMetaObject::invokeMethod(
        self.data(),
        [self, entries = std::move(entries)] {
          if (!self) {
            return;
          }
          self->m_apps.clear();
          self->m_appIndex.clear();
          for (const auto& e : entries) {
            App a;
            a.id = QString::fromStdWString(e.id);
            a.name = QString::fromStdWString(e.name);
            a.folded = fold(a.name);
            a.initials = initialsOf(a.folded);
            a.store = e.storeApp;
            self->m_appIndex.insert(a.id, static_cast<int>(self->m_apps.size()));
            self->m_apps.append(std::move(a));
          }
          self->m_loading = false;
          emit self->appsChanged();
        },
        Qt::QueuedConnection);
  }).detach();
}

// ── results ─────────────────────────────────────────────────────────────────

QVariantMap LauncherService::appResult(const App& app) const {
  return QVariantMap{
      {"key", "app:" + app.id},
      {"kind", "app"},
      {"id", app.id},
      {"title", app.name},
      {"subtitle", app.store ? QStringLiteral("Microsoft Store") : QStringLiteral("Aplicativo")},
      {"icon", iconUrl("app", app.id)},
      {"glyph", "apps"},
      {"pinned", isPinned(app.id)},
  };
}

QVariantList LauncherService::searchApps(const QString& rawQuery) const {
  const QStringList terms = fold(rawQuery).split(' ', Qt::SkipEmptyParts);
  const bool byUsage = m_config->launcher().value("sort_by_usage", true).toBool();

  struct Hit {
    double score;
    int index;
  };
  QList<Hit> hits;
  for (int i = 0; i < m_apps.size(); ++i) {
    const App& a = m_apps[i];
    double total = 0;
    bool ok = true;
    for (const QString& t : terms) {
      const double s = termScore(a.folded, a.initials, t);
      if (s < 0) {
        ok = false;
        break;
      }
      total += s;
    }
    if (!ok) {
      continue;
    }
    double score = terms.isEmpty() ? 0 : total / terms.size();
    if (byUsage) {
      score += usageBoost(a.id);
    }
    hits.append({score, i});
  }
  std::stable_sort(hits.begin(), hits.end(), [](const Hit& x, const Hit& y) { return x.score > y.score; });
  // When there are real matches, drop the loose fuzzy ones ("code" shouldn't list "Bloco de Notas").
  if (!hits.isEmpty() && hits.first().score >= 50) {
    hits.erase(std::remove_if(hits.begin(), hits.end(), [](const Hit& h) { return h.score < 30; }), hits.end());
  }

  QVariantList out;
  for (int i = 0; i < hits.size() && i < 60; ++i) {
    out.append(appResult(m_apps[hits[i].index]));
  }
  return out;
}

QVariantList LauncherService::calculatorResult(const QString& expr, bool standalone) const {
  if (!standalone && !matheval::looksLikeMath(expr)) {
    return {};
  }
  const auto value = matheval::evaluate(expr);
  if (!value) {
    if (standalone && !expr.trimmed().isEmpty()) {
      return {QVariantMap{{"key", "calc:invalid"},
                          {"kind", "info"},
                          {"title", QStringLiteral("Expressão inválida")},
                          {"subtitle", QStringLiteral("Ex.: 2+2*3, sqrt(2), 15% * 80, 2^10")},
                          {"glyph", "calculator"}}};
    }
    return {};
  }
  const QString text = matheval::format(*value);
  return {QVariantMap{
      {"key", "calc:" + text},
      {"kind", "copy"},
      {"id", text},
      {"title", text},
      {"subtitle", QStringLiteral("%1 · Enter copia o resultado").arg(expr.trimmed())},
      {"badge", "="},
      {"glyph", "calculator"},
  }};
}

QVariantList LauncherService::searchEmoji(const QString& q) {
  if (!m_emojiLoaded) {
    QFile f(":/assets/emoji.json");
    if (f.open(QIODevice::ReadOnly)) {
      m_emoji = QJsonDocument::fromJson(f.readAll()).array();
    }
    m_emojiLoaded = true;
  }
  const QString term = fold(q.trimmed());
  QVariantList out;
  for (const QJsonValue& v : std::as_const(m_emoji)) {
    const QJsonObject e = v.toObject();
    const QString name = e.value("name").toString();
    bool match = term.isEmpty() || fold(name).contains(term);
    if (!match) {
      for (const QJsonValue& k : e.value("keywords").toArray()) {
        if (fold(k.toString()).contains(term)) {
          match = true;
          break;
        }
      }
    }
    if (!match) {
      continue;
    }
    const QString emoji = e.value("emoji").toString();
    out.append(QVariantMap{
        {"key", "emoji:" + emoji},
        {"kind", "copy"},
        {"id", emoji},
        {"title", name.left(1).toUpper() + name.mid(1)},
        {"subtitle", e.value("category").toString()},
        {"badge", emoji},
    });
    if (out.size() >= 120) {
      break;
    }
  }
  return out;
}

QVariantList LauncherService::searchWindows(const QString& q) const {
  const QString term = fold(q.trimmed());
  QVariantList out;
  for (const auto& w : platform::enumerateWindows()) {
    const QString title = QString::fromStdWString(w.title);
    const QString exe = QFileInfo(QString::fromStdWString(w.exePath)).completeBaseName();
    if (!term.isEmpty() && !fold(title).contains(term) && !fold(exe).contains(term)) {
      continue;
    }
    const QString appId = QString::fromStdWString(w.appId);
    QString icon;
    if (!appId.isEmpty() && m_appIndex.contains(appId)) {
      icon = iconUrl("app", appId);
    } else {
      icon = iconUrl("file", QString::fromStdWString(w.exePath));
    }
    out.append(QVariantMap{
        {"key", QStringLiteral("win:%1").arg(w.handle)},
        {"kind", "window"},
        {"id", QString::number(w.handle)},
        {"title", title},
        {"subtitle", exe + (w.minimized ? QStringLiteral(" · minimizada") : QString())},
        {"icon", icon},
        {"glyph", "app-window"},
    });
  }
  return out;
}

QVariantList LauncherService::sessionResults(const QString& q) const {
  struct S {
    const char* id;
    const char* title;
    const char* glyph;
  };
  static const S actions[] = {
      {"lock", "Bloquear", "lock"},           {"logout", "Sair da conta", "logout"},
      {"suspend", "Suspender", "suspend"},    {"hibernate", "Hibernar", "hibernate"},
      {"reboot", "Reiniciar", "reboot"},      {"shutdown", "Desligar", "shutdown"},
  };
  const QString term = fold(q.trimmed());
  QVariantList out;
  for (const S& a : actions) {
    const QString title = QString::fromUtf8(a.title);
    if (!term.isEmpty() && !fold(title).contains(term) && !QString::fromLatin1(a.id).contains(term)) {
      continue;
    }
    out.append(QVariantMap{
        {"key", QStringLiteral("session:%1").arg(a.id)},
        {"kind", "session"},
        {"id", QString::fromLatin1(a.id)},
        {"title", title},
        {"subtitle", QStringLiteral("Executar ação de sessão")},
        {"glyph", QString::fromLatin1(a.glyph)},
    });
  }
  return out;
}

QVariantList LauncherService::providerOverview(const QString& q) const {
  struct P {
    const char* trigger;
    const char* title;
    const char* subtitle;
    const char* glyph;
  };
  static const P providers[] = {
      {"calc", "Calculadora", "Contas rápidas: 2+2*3, sqrt(2), 15% * 80", "calculator"},
      {"emo", "Emoji", "Busque e copie emojis", "mood-smile"},
      {"win", "Janelas", "Alterne entre as janelas abertas", "app-window"},
      {"session", "Sessão", "Bloquear, suspender, reiniciar, desligar", "shutdown"},
  };
  const QString term = fold(q.trimmed());
  QVariantList out;
  for (const P& p : providers) {
    const QString trig = QString::fromLatin1(p.trigger);
    const QString title = QString::fromUtf8(p.title);
    if (!term.isEmpty() && !trig.startsWith(term) && !fold(title).contains(term)) {
      continue;
    }
    out.append(QVariantMap{
        {"key", "provider:" + trig},
        {"kind", "provider"},
        {"id", prefix() + trig + ' '},
        {"title", title},
        {"subtitle", prefix() + trig + QStringLiteral(" · ") + QString::fromUtf8(p.subtitle)},
        {"glyph", QString::fromLatin1(p.glyph)},
    });
  }
  return out;
}

QVariantList LauncherService::query(const QString& text, const QString& view) {
  const QString pfx = prefix();
  const QString trimmed = text.trimmed();

  // "/trigger rest" -> one provider only
  if (trimmed.startsWith(pfx)) {
    const QString body = trimmed.mid(pfx.size());
    const int space = body.indexOf(' ');
    const QString trigger = (space < 0 ? body : body.left(space)).toLower();
    const QString rest = space < 0 ? QString() : body.mid(space + 1);
    const bool complete = space >= 0 || text.endsWith(' ');
    if (complete || trigger == "calc" || trigger == "emo" || trigger == "win" || trigger == "session") {
      if (trigger == "calc") return calculatorResult(rest, true);
      if (trigger == "emo") return searchEmoji(rest);
      if (trigger == "win") return searchWindows(rest);
      if (trigger == "session") return sessionResults(rest);
    }
    return providerOverview(trigger);
  }

  if (trimmed.isEmpty()) {
    QVariantList out;
    if (view == "pinned") {
      for (const QString& id : pinnedIds()) {
        const auto it = m_appIndex.constFind(id);
        if (it != m_appIndex.constEnd()) {
          out.append(appResult(m_apps[it.value()]));
        }
      }
      return out;
    }
    if (view == "recent") {
      QList<QPair<qint64, int>> recent;
      for (auto it = m_usage.constBegin(); it != m_usage.constEnd(); ++it) {
        const auto idx = m_appIndex.constFind(it.key());
        if (idx != m_appIndex.constEnd()) {
          recent.append({it.value().last, idx.value()});
        }
      }
      std::sort(recent.begin(), recent.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
      for (int i = 0; i < recent.size() && i < 30; ++i) {
        out.append(appResult(m_apps[recent[i].second]));
      }
      return out;
    }
    for (const App& a : std::as_const(m_apps)) {
      out.append(appResult(a));
    }
    return out;
  }

  // Plain search: calculator first (when it looks like math), then apps.
  QVariantList out = calculatorResult(trimmed, false);
  out.append(searchApps(trimmed));
  return out;
}

QString LauncherService::activate(const QVariantMap& result) {
  const QString kind = result.value("kind").toString();
  const QString id = result.value("id").toString();

  if (kind == "app") {
    platform::launchApp(id.toStdWString());
    recordUsage(id);
    return "close";
  }
  if (kind == "window") {
    platform::activateWindow(static_cast<std::uintptr_t>(id.toULongLong()));
    return "close";
  }
  if (kind == "copy") {
    QGuiApplication::clipboard()->setText(id);
    return m_config->launcher().value("auto_paste", true).toBool() ? "close-paste" : "close";
  }
  if (kind == "session") {
    using platform::SessionAction;
    static const QHash<QString, SessionAction> map{
        {"lock", SessionAction::Lock},           {"logout", SessionAction::Logout}, {"suspend", SessionAction::Suspend},
        {"hibernate", SessionAction::Hibernate}, {"reboot", SessionAction::Reboot}, {"shutdown", SessionAction::Shutdown},
    };
    if (map.contains(id)) {
      platform::sessionAction(map.value(id));
    }
    return "close";
  }
  if (kind == "provider") {
    return "set:" + id;
  }
  return "keep";
}

// ── pinned ──────────────────────────────────────────────────────────────────

QStringList LauncherService::pinnedIds() const { return m_config->launcher().value("pinned").toStringList(); }

bool LauncherService::isPinned(const QString& appId) const { return pinnedIds().contains(appId); }

void LauncherService::setPinned(const QString& appId, bool pinned) {
  QStringList ids = pinnedIds();
  if (pinned == ids.contains(appId)) {
    return;
  }
  if (pinned) {
    ids.append(appId);
  } else {
    ids.removeAll(appId);
  }
  m_config->setValue("launcher.pinned", ids);
}

// ── usage tracking (Recent view + "sort by usage") ─────────────────────────

QString LauncherService::usagePath() const {
  return QDir(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)).filePath("noctalia-win/launcher-usage.json");
}

double LauncherService::usageBoost(const QString& id) const {
  const auto it = m_usage.constFind(id);
  if (it == m_usage.constEnd()) {
    return 0;
  }
  // Same idea as Noctalia: +0.1 per launch, capped at 0.5 (scaled to our 0..100 scores).
  return std::min(it.value().count * 0.1, 0.5) * 20;
}

void LauncherService::recordUsage(const QString& id) {
  if (!m_config->launcher().value("track_usage", true).toBool()) {
    return;
  }
  Usage& u = m_usage[id];
  ++u.count;
  u.last = QDateTime::currentSecsSinceEpoch();
  saveUsage();
  emit usageChanged();
}

void LauncherService::loadUsage() {
  QFile f(usagePath());
  if (!f.open(QIODevice::ReadOnly)) {
    return;
  }
  const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
  for (auto it = root.begin(); it != root.end(); ++it) {
    const QJsonObject o = it.value().toObject();
    m_usage.insert(it.key(), Usage{o.value("count").toInt(), static_cast<qint64>(o.value("last").toDouble())});
  }
}

void LauncherService::saveUsage() const {
  QJsonObject root;
  for (auto it = m_usage.constBegin(); it != m_usage.constEnd(); ++it) {
    root.insert(it.key(), QJsonObject{{"count", it.value().count}, {"last", static_cast<double>(it.value().last)}});
  }
  QDir().mkpath(QFileInfo(usagePath()).absolutePath());
  QFile f(usagePath());
  if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    f.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
  }
}

void LauncherService::resetUsage() {
  m_usage.clear();
  saveUsage();
  emit usageChanged();
}
