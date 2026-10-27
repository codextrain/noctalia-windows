#include "services/WallpaperStore.h"

#include "core/Config.h"
#include "services/WallpaperService.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QRegularExpression>

#include <algorithm>

namespace {

  constexpr int kMaxParallel = 3;
  constexpr qint64 kCollectionCacheMs = 24LL * 3600 * 1000;

  // Collections shown by default: well-known "rice" wallpaper repositories (r/unixporn).
  struct KnownCollection {
    const char* repo;
    const char* name;
    const char* description;
  };
  const KnownCollection kKnown[] = {
      {"dharmx/walls", "dharmx/walls", "Coleção enorme, separada por estilo"},
      {"D3Ext/aesthetic-wallpapers", "Aesthetic", "Minimalistas e aesthetic"},
      {"orangci/walls-catppuccin-mocha", "Catppuccin Mocha", "Tudo na paleta Catppuccin Mocha"},
      {"zhichaoh/catppuccin-wallpapers", "Catppuccin", "Paleta Catppuccin"},
      {"AngelJumbo/gruvbox-wallpapers", "Gruvbox", "Paleta Gruvbox"},
      {"linuxdotexe/nordic-wallpapers", "Nord", "Paleta Nord"},
      {"rose-pine/wallpapers", "Rosé Pine", "Oficiais do Rosé Pine"},
      {"JaKooLit/Wallpaper-Bank", "JaKooLit", "Dos rices de Hyprland do JaKooLit"},
      {"mylinuxforwork/wallpaper", "ML4W", "Do projeto My Linux for Work"},
      {"Gingeh/wallpapers", "Gingeh", "Variados, muito usados em rices"},
  };

  QStringList defaultCollections() {
    QStringList out;
    for (const auto& k : kKnown) out << QString::fromLatin1(k.repo);
    return out;
  }

  bool isImagePath(const QString& path) {
    // Formats Windows can use as a wallpaper without extra codecs.
    const QString lower = path.toLower();
    return lower.endsWith(".jpg") || lower.endsWith(".jpeg") || lower.endsWith(".png") || lower.endsWith(".bmp");
  }

  QString encodePath(const QString& path) {
    QStringList parts = path.split('/');
    for (QString& p : parts) p = QString::fromUtf8(QUrl::toPercentEncoding(p));
    return parts.join('/');
  }

  QString thumbnailFor(const QString& fullUrl) {
    // GitHub only serves full-size files (often 5-20 MB). wsrv.nl is a free image proxy that
    // returns a small JPEG thumbnail, so browsing thousands of images stays fast.
    QString host = fullUrl;
    host.remove(QRegularExpression("^https?://"));
    return QStringLiteral("https://wsrv.nl/?url=%1&w=400&h=250&fit=cover&output=jpg&q=72")
        .arg(QString::fromUtf8(QUrl::toPercentEncoding(host)));
  }

  QString safeName(QString s) {
    static const QRegularExpression bad(R"([<>:"\\|?*\x00-\x1f])");
    s.replace(bad, "_");
    return s;
  }

  QString humanSize(qint64 bytes) {
    if (bytes <= 0) return {};
    if (bytes < 1024 * 1024) return QString::number(bytes / 1024) + " KB";
    return QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + " MB";
  }

} // namespace

WallpaperStore::WallpaperStore(Config* config, WallpaperService* wallpaper, QObject* parent)
    : QObject(parent), m_config(config), m_wallpaper(wallpaper) {
  connect(m_config, &Config::changed, this, &WallpaperStore::collectionsChanged);
}

// ── settings ────────────────────────────────────────────────────────────────

QString WallpaperStore::downloadDirectory() const {
  QString dir = m_config->wallpaper().value("download_directory").toString().trimmed();
  if (dir.isEmpty()) {
    dir = QDir(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)).filePath("Wallpapers");
  }
  dir.replace("%USERPROFILE%", QDir::homePath(), Qt::CaseInsensitive);
  if (dir.startsWith("~")) dir = QDir::homePath() + dir.mid(1);
  return QDir::cleanPath(dir);
}

QVariantList WallpaperStore::collections() const {
  const QVariant saved = m_config->wallpaper().value("collections");
  const QStringList repos = saved.isValid() && saved.canConvert<QStringList>() && !saved.toStringList().isEmpty()
                                ? saved.toStringList()
                                : defaultCollections();
  QVariantList out;
  for (const QString& repo : repos) {
    QVariantMap c{{"repo", repo}, {"name", repo}, {"description", QString()}};
    for (const auto& k : kKnown) {
      if (repo.compare(QLatin1String(k.repo), Qt::CaseInsensitive) == 0) {
        c["name"] = QString::fromUtf8(k.name);
        c["description"] = QString::fromUtf8(k.description);
      }
    }
    const QString folder = QDir(downloadDirectory()).filePath("github/" + QString(repo).replace('/', '-'));
    c["folder"] = QDir::toNativeSeparators(folder);
    out.append(c);
  }
  return out;
}

bool WallpaperStore::addCollection(const QString& input) {
  // Accepts "owner/repo", "owner/repo/sub/folder" or a github.com URL.
  QString repo = input.trimmed();
  repo.remove(QRegularExpression("^https?://(www\\.)?github\\.com/"));
  repo.remove(QRegularExpression("\\.git$"));
  repo.replace(QRegularExpression("/tree/[^/]+"), QString());
  while (repo.endsWith('/')) repo.chop(1);
  if (repo.count('/') < 1 || repo.contains(' ')) {
    setError("Use o formato dono/repositório, por exemplo dharmx/walls.");
    return false;
  }
  QStringList repos;
  for (const auto& c : collections()) repos << c.toMap().value("repo").toString();
  if (!repos.contains(repo, Qt::CaseInsensitive)) repos << repo;
  m_config->setValue("wallpaper.collections", repos);
  setError({});
  return true;
}

void WallpaperStore::removeCollection(const QString& repo) {
  QStringList repos;
  for (const auto& c : collections()) {
    const QString r = c.toMap().value("repo").toString();
    if (r.compare(repo, Qt::CaseInsensitive) != 0) repos << r;
  }
  m_config->setValue("wallpaper.collections", repos);
}

// ── networking helpers ──────────────────────────────────────────────────────

QNetworkReply* WallpaperStore::get(const QString& url, bool api) {
  QNetworkRequest req{QUrl(url)};
  req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("noctalia-win/" NOCTALIA_WIN_VERSION));
  req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
  req.setTransferTimeout(api ? 20000 : 90000);
  if (api && url.startsWith("https://api.github.com/")) {
    req.setRawHeader("Accept", "application/vnd.github+json");
    const QString token = m_config->wallpaper().value("github_token").toString().trimmed();
    if (!token.isEmpty()) req.setRawHeader("Authorization", "Bearer " + token.toUtf8());
  }
  return m_net.get(req);
}

void WallpaperStore::setBusy(bool on) {
  if (m_busy != on) {
    m_busy = on;
    emit busyChanged();
  }
}

void WallpaperStore::setError(const QString& message) {
  if (m_error != message) {
    m_error = message;
    emit errorChanged();
  }
}

// ── Wallhaven ───────────────────────────────────────────────────────────────

QUrl WallpaperStore::wallhavenUrl(const QString& query, const QString& sorting, const QString& minResolution, int page) const {
  const QVariantMap wh = m_config->wallpaper().value("wallhaven").toMap();
  QUrl url("https://wallhaven.cc/api/v1/search");
  QUrlQuery q;
  if (!query.trimmed().isEmpty()) q.addQueryItem("q", query.trimmed());
  const QString sort = sorting.isEmpty() ? QStringLiteral("toplist") : sorting;
  q.addQueryItem("sorting", sort);
  q.addQueryItem("order", "desc");
  if (sort == "toplist") q.addQueryItem("topRange", wh.value("top_range", "1y").toString());
  q.addQueryItem("categories", wh.value("categories", "111").toString()); // general / anime / people
  q.addQueryItem("purity", wh.value("purity", "100").toString());         // SFW only by default
  if (!minResolution.isEmpty()) q.addQueryItem("atleast", minResolution);
  const QString ratios = wh.value("ratios", "16x9,16x10,21x9").toString();
  if (!ratios.isEmpty()) q.addQueryItem("ratios", ratios);
  if (sort == "random" && page > 1 && !m_seed.isEmpty()) {
    q.addQueryItem("seed", m_seed); // keep the same random order while scrolling
  }
  q.addQueryItem("page", QString::number(std::max(1, page)));
  const QString key = wh.value("api_key").toString().trimmed();
  if (!key.isEmpty()) q.addQueryItem("apikey", key);
  url.setQuery(q);
  return url;
}

QVariantList WallpaperStore::parseWallhaven(const QByteArray& json, int* page, int* lastPage, int* total) const {
  QVariantList out;
  const QJsonObject root = QJsonDocument::fromJson(json).object();
  const QJsonObject meta = root.value("meta").toObject();
  if (page) *page = meta.value("current_page").toInt(1);
  if (lastPage) *lastPage = meta.value("last_page").toInt(1);
  if (total) *total = meta.value("total").toInt(0);
  const QString dir = QDir(downloadDirectory()).filePath("wallhaven");
  for (const QJsonValue& v : root.value("data").toArray()) {
    const QJsonObject o = v.toObject();
    const QString full = o.value("path").toString();
    if (full.isEmpty()) continue;
    const QString id = o.value("id").toString();
    const QString ext = QFileInfo(QUrl(full).path()).suffix();
    out.append(QVariantMap{
        {"key", "wh:" + id},
        {"source", "wallhaven"},
        {"name", "wallhaven-" + id},
        {"thumb", o.value("thumbs").toObject().value("large").toString()},
        {"full", full},
        {"page", o.value("url").toString()},
        {"resolution", o.value("resolution").toString()},
        {"fileSize", humanSize(o.value("file_size").toVariant().toLongLong())},
        {"colors", o.value("colors").toArray().toVariantList()},
        {"target", QDir::toNativeSeparators(QDir(dir).filePath("wallhaven-" + id + "." + ext))},
    });
  }
  return out;
}

void WallpaperStore::searchWallhaven(const QString& query, const QString& sorting, const QString& minResolution, int page) {
  if (m_listReply) m_listReply->abort();
  m_lastWasWallhaven = true;
  if (page <= 1) {
    m_query = query;
    m_sorting = sorting;
    m_minResolution = minResolution;
  }
  setError({});
  // Debug: NOCTALIA_WALLHAVEN_FIXTURE=<file with a saved API response> (UI previews offline).
  const QString fixture = qEnvironmentVariable("NOCTALIA_WALLHAVEN_FIXTURE");
  if (!fixture.isEmpty()) {
    QFile f(fixture);
    if (f.open(QIODevice::ReadOnly)) handleWallhaven(f.readAll(), page);
    return;
  }
  setBusy(true);
  QNetworkReply* reply = get(wallhavenUrl(m_query, m_sorting, m_minResolution, page).toString(), true);
  m_listReply = reply;
  connect(reply, &QNetworkReply::finished, this, [this, reply, page] {
    reply->deleteLater();
    if (reply != m_listReply) return;
    setBusy(false);
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (reply->error() == QNetworkReply::OperationCanceledError) return;
    if (status == 429) {
      setError("O Wallhaven pediu uma pausa (muitas buscas seguidas). Tente de novo em um minuto.");
      return;
    }
    if (reply->error() != QNetworkReply::NoError) {
      setError("Não foi possível falar com o Wallhaven: " + reply->errorString());
      return;
    }
    handleWallhaven(reply->readAll(), page);
  });
}

void WallpaperStore::handleWallhaven(const QByteArray& body, int page) {
  {
    int cur = 1, last = 1, total = 0;
    QVariantList items = parseWallhaven(body, &cur, &last, &total);
    for (const QVariant& v : items) {
      const QVariantMap m = v.toMap();
      if (QFileInfo::exists(m.value("target").toString())) m_downloaded.insert(m.value("key").toString(), m.value("target"));
    }
    if (page <= 1) {
      m_results = items;
      m_seed = QJsonDocument::fromJson(body).object().value("meta").toObject().value("seed").toString();
      m_resultsTitle = m_query.isEmpty() ? QStringLiteral("Wallhaven") : m_query;
    } else {
      m_results.append(items);
    }
    m_page = cur;
    m_lastPage = last;
    m_total = total;
    if (items.isEmpty() && page <= 1) setError("Nada encontrado no Wallhaven para essa busca.");
    emit downloadedChanged();
    emit resultsChanged();
  }
}

void WallpaperStore::loadMore() {
  if (!m_lastWasWallhaven || m_busy || m_page >= m_lastPage) return;
  searchWallhaven(m_query, m_sorting, m_minResolution, m_page + 1);
}

void WallpaperStore::downloadWallhaven(const QString& query, const QString& sorting, const QString& minResolution, int count) {
  if (!downloading() && !m_bulk.active) {
    m_queueTotal = m_queueDone = m_queueFailed = 0;
  }
  m_bulk = Bulk{query, sorting, minResolution, std::clamp(count, 1, 10000), 0, 0, true};
  m_seed.clear();
  m_queueStatus = "Buscando a lista no Wallhaven...";
  emit queueChanged();
  wallhavenBulkPage();
}

void WallpaperStore::wallhavenBulkPage() {
  if (!m_bulk.active) return;
  ++m_bulk.page;
  QNetworkReply* reply = get(wallhavenUrl(m_bulk.query, m_bulk.sorting, m_bulk.minResolution, m_bulk.page).toString(), true);
  m_bulkReply = reply;
  connect(reply, &QNetworkReply::finished, this, [this, reply] {
    reply->deleteLater();
    if (!m_bulk.active) return;
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status == 429) {
      // Wallhaven allows ~45 API calls per minute: wait and retry the same page.
      --m_bulk.page;
      m_queueStatus = "Aguardando o Wallhaven liberar mais buscas...";
      emit queueChanged();
      QTimer::singleShot(20000, this, &WallpaperStore::wallhavenBulkPage);
      return;
    }
    if (reply->error() != QNetworkReply::NoError) {
      m_bulk.active = false;
      setError("Download em massa parou: " + reply->errorString());
      emit queueChanged();
      return;
    }
    int cur = 1, last = 1;
    const QVariantList items = parseWallhaven(reply->readAll(), &cur, &last, nullptr);
    for (const QVariant& v : items) {
      if (m_bulk.queued >= m_bulk.wanted) break;
      const QVariantMap m = v.toMap();
      enqueue({m.value("key").toString(), m.value("full").toString(), m.value("target").toString(), false});
      ++m_bulk.queued;
    }
    if (m_bulk.queued >= m_bulk.wanted || cur >= last || items.isEmpty()) {
      m_bulk.active = false;
      emit queueChanged();
      return;
    }
    // Stay well under the API limit.
    QTimer::singleShot(1500, this, &WallpaperStore::wallhavenBulkPage);
  });
}

// ── GitHub collections ──────────────────────────────────────────────────────

QString WallpaperStore::cachePath(const QString& repo) const {
  const QString dir = QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)).filePath("collections");
  QDir().mkpath(dir);
  return QDir(dir).filePath(safeName(QString(repo).replace('/', '_')) + ".json");
}

void WallpaperStore::openCollection(const QString& input) {
  if (m_listReply) m_listReply->abort();
  m_lastWasWallhaven = false;
  const QStringList parts = input.split('/', Qt::SkipEmptyParts);
  if (parts.size() < 2) {
    setError("Coleção inválida: " + input);
    return;
  }
  const QString repo = parts[0] + "/" + parts[1];
  const QString subdir = parts.mid(2).join('/');
  setError({});

  // Cached listing (GitHub allows 60 API calls per hour without a token).
  QFile cache(cachePath(input));
  if (cache.open(QIODevice::ReadOnly)) {
    const QJsonObject c = QJsonDocument::fromJson(cache.readAll()).object();
    if (QDateTime::currentMSecsSinceEpoch() - c.value("fetched").toVariant().toLongLong() < kCollectionCacheMs) {
      showCollection(input, c.value("branch").toString(), subdir, c.value("tree").toArray(), c.value("truncated").toBool());
      return;
    }
  }

  setBusy(true);
  QNetworkReply* info = get("https://api.github.com/repos/" + repo, true);
  m_listReply = info;
  connect(info, &QNetworkReply::finished, this, [this, info, input, repo, subdir] {
    info->deleteLater();
    if (info != m_listReply) return;
    if (info->error() == QNetworkReply::OperationCanceledError) { setBusy(false); return; }
    const int status = info->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status == 404) { setBusy(false); setError("Repositório " + repo + " não encontrado no GitHub."); return; }
    if (status == 403 || status == 429) {
      setBusy(false);
      setError("O GitHub limitou as consultas (60 por hora sem conta). Tente mais tarde ou coloque um token em wallpaper.github_token.");
      return;
    }
    if (info->error() != QNetworkReply::NoError) { setBusy(false); setError("GitHub: " + info->errorString()); return; }
    const QString branch = QJsonDocument::fromJson(info->readAll()).object().value("default_branch").toString("main");

    QNetworkReply* treeReply = get("https://api.github.com/repos/" + repo + "/git/trees/" + branch + "?recursive=1", true);
    m_listReply = treeReply;
    connect(treeReply, &QNetworkReply::finished, this, [this, treeReply, input, branch, subdir] {
      treeReply->deleteLater();
      if (treeReply != m_listReply) return;
      setBusy(false);
      if (treeReply->error() == QNetworkReply::OperationCanceledError) return;
      if (treeReply->error() != QNetworkReply::NoError) { setError("GitHub: " + treeReply->errorString()); return; }
      const QJsonObject root = QJsonDocument::fromJson(treeReply->readAll()).object();
      // Keep only image blobs in the cache (trees of big repos are several MB).
      QJsonArray images;
      for (const QJsonValue& v : root.value("tree").toArray()) {
        const QJsonObject o = v.toObject();
        if (o.value("type").toString() == "blob" && isImagePath(o.value("path").toString())) {
          images.append(QJsonObject{{"path", o.value("path")}, {"size", o.value("size")}});
        }
      }
      const bool truncated = root.value("truncated").toBool();
      QSaveFile out(cachePath(input));
      if (out.open(QIODevice::WriteOnly)) {
        out.write(QJsonDocument(QJsonObject{{"branch", branch},
                                            {"truncated", truncated},
                                            {"fetched", QDateTime::currentMSecsSinceEpoch()},
                                            {"tree", images}})
                      .toJson(QJsonDocument::Compact));
        out.commit();
      }
      showCollection(input, branch, subdir, images, truncated);
    });
  });
}

void WallpaperStore::showCollection(const QString& input, const QString& branch, const QString& subdir, const QJsonArray& tree, bool truncated) {
  const QStringList parts = input.split('/', Qt::SkipEmptyParts);
  const QString repo = parts[0] + "/" + parts[1];
  const QString folder = QDir(downloadDirectory()).filePath("github/" + QString(repo).replace('/', '-'));
  QVariantList items;
  for (const QJsonValue& v : tree) {
    const QJsonObject o = v.toObject();
    const QString path = o.value("path").toString();
    if (!subdir.isEmpty() && !path.startsWith(subdir + "/")) continue;
    const QString full = "https://raw.githubusercontent.com/" + repo + "/" + encodePath(branch) + "/" + encodePath(path);
    const QString target = QDir::toNativeSeparators(QDir(folder).filePath(safeName(path)));
    const QString key = "gh:" + repo + ":" + path;
    if (QFileInfo::exists(target)) m_downloaded.insert(key, target);
    items.append(QVariantMap{
        {"key", key},
        {"source", "github"},
        {"name", QFileInfo(path).completeBaseName()},
        {"thumb", thumbnailFor(full)},
        {"full", full},
        {"page", "https://github.com/" + repo + "/blob/" + branch + "/" + encodePath(path)},
        {"resolution", QString()},
        {"fileSize", humanSize(o.value("size").toVariant().toLongLong())},
        {"folder", QFileInfo(path).path() == "." ? QString() : QFileInfo(path).path()},
        {"target", target},
    });
  }
  m_results = items;
  m_resultsTitle = input;
  m_page = 1;
  m_lastPage = 1;
  m_total = static_cast<int>(items.size());
  if (items.isEmpty()) setError("Nenhuma imagem JPG/PNG nesse repositório.");
  else if (truncated) setError("O repositório é grande demais e o GitHub mandou só parte da lista.");
  emit downloadedChanged();
  emit resultsChanged();
}

// ── downloads ───────────────────────────────────────────────────────────────

void WallpaperStore::download(const QVariantMap& item, bool apply) {
  const QString key = item.value("key").toString();
  const QString target = item.value("target").toString();
  if (QFileInfo::exists(target)) {
    markDownloaded(key, target);
    if (apply) m_wallpaper->set(target);
    return;
  }
  enqueue({key, item.value("full").toString(), target, apply});
}

void WallpaperStore::downloadResults() {
  for (const QVariant& v : m_results) {
    const QVariantMap m = v.toMap();
    if (!QFileInfo::exists(m.value("target").toString())) {
      enqueue({m.value("key").toString(), m.value("full").toString(), m.value("target").toString(), false});
    }
  }
  if (m_queueTotal == m_queueDone + m_queueFailed) {
    m_queueStatus = "Todas já estão baixadas.";
    emit queueChanged();
  }
}

void WallpaperStore::enqueue(const Job& job) {
  if (job.url.isEmpty() || job.target.isEmpty() || m_queuedKeys.contains(job.key)) return;
  if (QFileInfo::exists(job.target)) {
    markDownloaded(job.key, job.target);
    return;
  }
  if (!downloading() && !m_bulk.active) {
    // a new batch: reset the counters
    m_queueTotal = m_queueDone = m_queueFailed = 0;
  }
  m_queuedKeys.insert(job.key);
  m_queue.enqueue(job);
  ++m_queueTotal;
  emit queueChanged();
  pump();
}

void WallpaperStore::pump() {
  while (m_active.size() < kMaxParallel && !m_queue.isEmpty()) {
    const Job job = m_queue.dequeue();
    QNetworkReply* reply = get(job.url, false);
    m_active.insert(reply, job);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
      const Job j = m_active.take(reply);
      reply->deleteLater();
      finishJob(reply, j);
      pump();
    });
  }
}

void WallpaperStore::finishJob(QNetworkReply* reply, const Job& job) {
  m_queuedKeys.remove(job.key);
  if (reply->error() == QNetworkReply::OperationCanceledError) {
    return; // cancelDownloads() already adjusted the counters
  }
  const QByteArray data = reply->readAll();
  const QString type = reply->header(QNetworkRequest::ContentTypeHeader).toString();
  const bool looksLikeImage = data.size() > 2048 && !type.startsWith("text/") && !data.startsWith("<");
  bool ok = reply->error() == QNetworkReply::NoError && looksLikeImage;
  if (ok) {
    QDir().mkpath(QFileInfo(job.target).absolutePath());
    QSaveFile out(job.target);
    ok = out.open(QIODevice::WriteOnly) && out.write(data) == data.size() && out.commit();
  }
  if (ok) {
    ++m_queueDone;
    markDownloaded(job.key, job.target);
    if (job.apply) m_wallpaper->set(job.target);
  } else {
    ++m_queueFailed;
  }
  queueStep(QFileInfo(job.target).fileName());
}

void WallpaperStore::queueStep(const QString& lastFile) {
  ++m_sinceRefresh;
  const bool finished = !downloading() && !m_bulk.active;
  if (finished) {
    m_queueStatus = QString("Pronto: %1 baixados").arg(m_queueDone) +
                    (m_queueFailed > 0 ? QString(", %1 com erro").arg(m_queueFailed) : QString()) + ".";
  } else {
    m_queueStatus = QString("Baixando %1 de %2 · %3").arg(m_queueDone + m_queueFailed + 1).arg(m_queueTotal).arg(lastFile);
  }
  // Let the "my wallpapers" grid see the new files: in batches, and at the end.
  if (finished || m_sinceRefresh >= 40) {
    m_sinceRefresh = 0;
    m_wallpaper->refresh(true);
  }
  emit queueChanged();
}

void WallpaperStore::markDownloaded(const QString& key, const QString& path) {
  if (m_downloaded.value(key).toString() != path) {
    m_downloaded.insert(key, path);
    emit downloadedChanged();
  }
}

void WallpaperStore::cancelDownloads() {
  m_bulk.active = false;
  if (m_bulkReply) m_bulkReply->abort();
  const int pending = static_cast<int>(m_queue.size() + m_active.size());
  m_queue.clear();
  m_queuedKeys.clear();
  const auto active = m_active.keys();
  m_queueTotal -= pending;
  for (QNetworkReply* r : active) r->abort();
  m_queueStatus = QString("Cancelado. %1 baixados.").arg(m_queueDone);
  m_wallpaper->refresh(true);
  emit queueChanged();
}

void WallpaperStore::openDownloadFolder() const {
  QDir().mkpath(downloadDirectory());
  QDesktopServices::openUrl(QUrl::fromLocalFile(downloadDirectory()));
}
