#pragma once

#include <QHash>
#include <QJsonArray>
#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

class Config;

// Search engine behind the launcher, modelled on Noctalia's launcher providers:
//   Applications (default), Calculator (also in plain search), Emoji ("/emo"),
//   Windows ("/win"), Session ("/session"). Typing "/" lists the providers.
//
// Each result is a map: { key, kind, id, title, subtitle, icon, glyph, badge, pinned }
//   icon  = image URL (app icons), glyph = Tabler icon name, badge = big text (emoji / "=").
class LauncherService : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool loading READ loading NOTIFY appsChanged)
  Q_PROPERTY(int appCount READ appCount NOTIFY appsChanged)
  Q_PROPERTY(QString prefix READ prefix NOTIFY prefixChanged)

public:
  explicit LauncherService(Config* config, QObject* parent = nullptr);
  ~LauncherService() override;

  bool loading() const { return m_loading; }
  int appCount() const { return m_apps.size(); }
  QString prefix() const;

  // view: "recent" | "pinned" | "all" — only used when `text` is empty.
  Q_INVOKABLE QVariantList query(const QString& text, const QString& view);
  // Runs a result. Returns what the launcher should do next:
  //   "close" (default), "close-paste" (text copied: paste into the previous window), "keep".
  Q_INVOKABLE QString activate(const QVariantMap& result);
  Q_INVOKABLE void setPinned(const QString& appId, bool pinned);
  Q_INVOKABLE bool isPinned(const QString& appId) const;
  Q_INVOKABLE void refreshApps();
  Q_INVOKABLE void resetUsage();

signals:
  void appsChanged();
  void prefixChanged();
  void usageChanged();

private:
  struct App {
    QString id;
    QString name;
    QString folded; // lowercase, accents removed: what we search in
    QString initials;
    bool store = false;
  };
  struct Usage {
    int count = 0;
    qint64 last = 0;
  };

  QVariantMap appResult(const App& app) const;
  QVariantList searchApps(const QString& q) const;
  QVariantList searchEmoji(const QString& q);
  QVariantList searchWindows(const QString& q) const;
  QVariantList sessionResults(const QString& q) const;
  QVariantList providerOverview(const QString& q) const;
  QVariantList calculatorResult(const QString& expr, bool standalone) const;
  double usageBoost(const QString& id) const;
  void recordUsage(const QString& id);
  void loadUsage();
  void saveUsage() const;
  QString usagePath() const;
  QStringList pinnedIds() const;

  Config* m_config = nullptr;
  QList<App> m_apps;
  QHash<QString, int> m_appIndex;
  QHash<QString, Usage> m_usage;
  QJsonArray m_emoji;
  bool m_emojiLoaded = false;
  bool m_loading = false;
};
