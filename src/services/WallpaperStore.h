#pragma once

#include <QHash>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QQueue>
#include <QSet>
#include <QVariantList>
#include <QVariantMap>

class Config;
class WallpaperService;
class QNetworkReply;

// Online wallpapers for the wallpaper panel ("Explorar"): search Wallhaven, browse "rice"
// collections hosted on GitHub, and download them (one, a whole page, or thousands) into the
// wallpaper folder, where the normal wallpaper grid and rotation pick them up.
//
// QML: WallStore.searchWallhaven("pixel art", "toplist"), WallStore.openCollection("owner/repo"),
//      WallStore.results [{key, source, name, thumb, full, resolution, fileSize, target}],
//      WallStore.download(item, apply), WallStore.downloadResults(), WallStore.downloadMany(...)
//      WallStore.downloaded[key] -> local path once the file exists.
class WallpaperStore : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariantList results READ results NOTIFY resultsChanged)
  Q_PROPERTY(QString resultsTitle READ resultsTitle NOTIFY resultsChanged)
  Q_PROPERTY(int page READ page NOTIFY resultsChanged)
  Q_PROPERTY(int lastPage READ lastPage NOTIFY resultsChanged)
  Q_PROPERTY(int total READ total NOTIFY resultsChanged)
  Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
  Q_PROPERTY(QString error READ error NOTIFY errorChanged)
  Q_PROPERTY(QVariantList collections READ collections NOTIFY collectionsChanged)
  Q_PROPERTY(QVariantMap downloaded READ downloaded NOTIFY downloadedChanged)
  Q_PROPERTY(QString downloadDirectory READ downloadDirectory NOTIFY collectionsChanged)
  // download queue
  Q_PROPERTY(bool downloading READ downloading NOTIFY queueChanged)
  Q_PROPERTY(int queueTotal READ queueTotal NOTIFY queueChanged)
  Q_PROPERTY(int queueDone READ queueDone NOTIFY queueChanged)
  Q_PROPERTY(int queueFailed READ queueFailed NOTIFY queueChanged)
  Q_PROPERTY(QString queueStatus READ queueStatus NOTIFY queueChanged)

public:
  WallpaperStore(Config* config, WallpaperService* wallpaper, QObject* parent = nullptr);

  QVariantList results() const { return m_results; }
  QString resultsTitle() const { return m_resultsTitle; }
  int page() const { return m_page; }
  int lastPage() const { return m_lastPage; }
  int total() const { return m_total; }
  bool busy() const { return m_busy; }
  QString error() const { return m_error; }
  QVariantList collections() const;
  QVariantMap downloaded() const { return m_downloaded; }
  QString downloadDirectory() const;
  bool downloading() const { return m_queueTotal > m_queueDone + m_queueFailed; }
  int queueTotal() const { return m_queueTotal; }
  int queueDone() const { return m_queueDone; }
  int queueFailed() const { return m_queueFailed; }
  QString queueStatus() const { return m_queueStatus; }

  // Wallhaven. sorting: toplist | date_added | random | relevance | views | favorites.
  // minResolution: "" | "1920x1080" | "2560x1440" | "3840x2160". page 1 replaces results,
  // later pages append (infinite scroll).
  Q_INVOKABLE void searchWallhaven(const QString& query, const QString& sorting,
                                   const QString& minResolution, int page = 1);
  Q_INVOKABLE void loadMore();

  // GitHub collections: "owner/repo" (optionally "owner/repo/sub/folder").
  Q_INVOKABLE void openCollection(const QString& repo);
  Q_INVOKABLE bool addCollection(const QString& repo);
  Q_INVOKABLE void removeCollection(const QString& repo);

  // Downloads. apply = set as wallpaper when done.
  Q_INVOKABLE void download(const QVariantMap& item, bool apply = false);
  Q_INVOKABLE void downloadResults();            // everything currently listed
  // Bulk Wallhaven: walks result pages of the given search until `count` images are queued.
  Q_INVOKABLE void downloadWallhaven(const QString& query, const QString& sorting,
                                     const QString& minResolution, int count);
  Q_INVOKABLE void cancelDownloads();
  Q_INVOKABLE void openDownloadFolder() const;

signals:
  void resultsChanged();
  void busyChanged();
  void errorChanged();
  void collectionsChanged();
  void downloadedChanged();
  void queueChanged();

private:
  struct Job {
    QString key;
    QString url;
    QString target;
    bool apply = false;
  };

  QNetworkReply* get(const QString& url, bool api);
  void setBusy(bool on);
  void setError(const QString& message);
  QUrl wallhavenUrl(const QString& query, const QString& sorting, const QString& minResolution, int page) const;
  void handleWallhaven(const QByteArray& body, int page);
  QVariantList parseWallhaven(const QByteArray& json, int* page, int* lastPage, int* total) const;
  void showCollection(const QString& repo, const QString& branch, const QString& subdir, const QJsonArray& tree, bool truncated);
  QString cachePath(const QString& repo) const;
  void enqueue(const Job& job);
  void pump();
  void finishJob(QNetworkReply* reply, const Job& job);
  void queueStep(const QString& status);
  void markDownloaded(const QString& key, const QString& path);
  void wallhavenBulkPage();

  Config* m_config = nullptr;
  WallpaperService* m_wallpaper = nullptr;
  QNetworkAccessManager m_net;

  QVariantList m_results;
  QString m_resultsTitle;
  int m_page = 0;
  int m_lastPage = 0;
  int m_total = 0;
  bool m_busy = false;
  QString m_error;
  QPointer<QNetworkReply> m_listReply;
  // last Wallhaven search, for loadMore()
  QString m_query, m_sorting, m_minResolution, m_seed;
  bool m_lastWasWallhaven = false;

  QVariantMap m_downloaded;
  QQueue<Job> m_queue;
  QSet<QString> m_queuedKeys;
  QHash<QNetworkReply*, Job> m_active;
  int m_queueTotal = 0;
  int m_queueDone = 0;
  int m_queueFailed = 0;
  int m_sinceRefresh = 0;
  QString m_queueStatus;

  // bulk Wallhaven state
  struct Bulk {
    QString query, sorting, minResolution;
    int wanted = 0;
    int queued = 0;
    int page = 0;
    bool active = false;
  } m_bulk;
  QPointer<QNetworkReply> m_bulkReply;
};
