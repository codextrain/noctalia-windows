#include "services/ClipboardService.h"

#include "core/Config.h"

#include <QClipboard>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeData>
#include <QStandardPaths>
#include <QUrl>
#include <QVariantMap>

namespace {
  // Formats Windows apps (password managers, banking apps...) set to opt out of clipboard history.
  bool excludedFromHistory(const QMimeData* data) {
    for (const QString& f : data->formats()) {
      if (f.contains("ExcludeClipboardContentFromMonitorProcessing") || f.contains("CanIncludeInClipboardHistory") ||
          f.contains("Clipboard Viewer Ignore") || f.contains("x-kde-passwordManagerHint")) {
        return true;
      }
    }
    return false;
  }
} // namespace

ClipboardService::ClipboardService(Config* config, QObject* parent) : QObject(parent), m_config(config) {
  load();
  // Apps often set the clipboard several times in a row (text, then HTML...): wait a moment.
  m_debounce.setSingleShot(true);
  m_debounce.setInterval(150);
  connect(&m_debounce, &QTimer::timeout, this, &ClipboardService::onClipboardChanged);
  connect(QGuiApplication::clipboard(), &QClipboard::dataChanged, this, [this] { m_debounce.start(); });

  m_saveTimer.setSingleShot(true);
  m_saveTimer.setInterval(1000);
  connect(&m_saveTimer, &QTimer::timeout, this, &ClipboardService::save);
}

bool ClipboardService::enabled() const { return m_config->shell().value("clipboard_enabled", true).toBool(); }

QString ClipboardService::dataDir() const {
  return QDir(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)).filePath("noctalia-win/clipboard");
}

QVariantList ClipboardService::entries() const {
  QVariantList out;
  // pinned first, then newest first (Noctalia keeps pinned entries on top)
  for (const bool pinnedPass : {true, false}) {
    for (const Entry& e : m_entries) {
      if (e.pinned != pinnedPass) {
        continue;
      }
      QString preview = e.text.left(400).simplified();
      out.append(QVariantMap{
          {"id", e.id},
          {"kind", e.kind},
          {"text", e.text.left(4000)},
          {"preview", e.kind == "image" ? QStringLiteral("Imagem") : preview},
          {"image", e.kind == "image" ? QUrl::fromLocalFile(e.imagePath).toString() : QString()},
          {"time", QDateTime::fromSecsSinceEpoch(e.time).toString("dd/MM HH:mm")},
          {"pinned", e.pinned},
      });
    }
  }
  return out;
}

void ClipboardService::onClipboardChanged() {
  if (m_ignoreNext) {
    m_ignoreNext = false;
    return;
  }
  if (!enabled()) {
    return;
  }
  const QMimeData* data = QGuiApplication::clipboard()->mimeData();
  if (data == nullptr || excludedFromHistory(data)) {
    return;
  }

  Entry e;
  e.time = QDateTime::currentSecsSinceEpoch();
  if (data->hasImage()) {
    const QImage img = qvariant_cast<QImage>(data->imageData());
    if (img.isNull()) {
      return;
    }
    const QByteArray hash = QCryptographicHash::hash(
        QByteArray::fromRawData(reinterpret_cast<const char*>(img.constBits()), img.sizeInBytes()), QCryptographicHash::Sha1);
    e.id = "img-" + QString::fromLatin1(hash.toHex().left(16));
    e.kind = "image";
    QDir().mkpath(dataDir());
    e.imagePath = QDir(dataDir()).filePath(e.id + ".png");
    if (!QFile::exists(e.imagePath)) {
      img.save(e.imagePath, "PNG");
    }
  } else if (data->hasText()) {
    e.text = data->text();
    if (e.text.trimmed().isEmpty() || e.text.size() > 200000) {
      return;
    }
    const QByteArray hash = QCryptographicHash::hash(e.text.toUtf8(), QCryptographicHash::Sha1);
    e.id = "txt-" + QString::fromLatin1(hash.toHex().left(16));
    e.kind = "text";
  } else {
    return;
  }

  // Same content again: move it to the top (keeping its pin).
  for (int i = 0; i < m_entries.size(); ++i) {
    if (m_entries[i].id == e.id) {
      e.pinned = m_entries[i].pinned;
      m_entries.removeAt(i);
      break;
    }
  }
  m_entries.prepend(e);
  trim();
  m_saveTimer.start();
  emit changed();
}

void ClipboardService::trim() {
  const int maxEntries = std::clamp(m_config->shell().value("clipboard_history_max_entries", 100).toInt(), 10, 10000);
  int unpinned = 0;
  for (int i = 0; i < m_entries.size();) {
    if (!m_entries[i].pinned && ++unpinned > maxEntries) {
      if (m_entries[i].kind == "image") {
        QFile::remove(m_entries[i].imagePath);
      }
      m_entries.removeAt(i);
    } else {
      ++i;
    }
  }
}

bool ClipboardService::activate(const QString& id) {
  for (int i = 0; i < m_entries.size(); ++i) {
    if (m_entries[i].id != id) {
      continue;
    }
    const Entry e = m_entries[i];
    m_ignoreNext = true;
    if (e.kind == "image") {
      QGuiApplication::clipboard()->setImage(QImage(e.imagePath));
    } else {
      QGuiApplication::clipboard()->setText(e.text);
    }
    // move to top
    m_entries.removeAt(i);
    m_entries.prepend(e);
    m_saveTimer.start();
    emit changed();
    return true;
  }
  return false;
}

void ClipboardService::remove(const QString& id) {
  for (int i = 0; i < m_entries.size(); ++i) {
    if (m_entries[i].id == id) {
      if (m_entries[i].kind == "image") {
        QFile::remove(m_entries[i].imagePath);
      }
      m_entries.removeAt(i);
      m_saveTimer.start();
      emit changed();
      return;
    }
  }
}

void ClipboardService::togglePin(const QString& id) {
  for (Entry& e : m_entries) {
    if (e.id == id) {
      e.pinned = !e.pinned;
      m_saveTimer.start();
      emit changed();
      return;
    }
  }
}

void ClipboardService::clear() {
  for (int i = 0; i < m_entries.size();) {
    if (!m_entries[i].pinned) {
      if (m_entries[i].kind == "image") {
        QFile::remove(m_entries[i].imagePath);
      }
      m_entries.removeAt(i);
    } else {
      ++i;
    }
  }
  m_saveTimer.start();
  emit changed();
}

void ClipboardService::save() {
  QJsonArray arr;
  for (const Entry& e : m_entries) {
    arr.append(QJsonObject{{"id", e.id}, {"kind", e.kind}, {"text", e.text}, {"image", e.imagePath},
                           {"time", static_cast<double>(e.time)}, {"pinned", e.pinned}});
  }
  QDir().mkpath(dataDir());
  QFile f(QDir(dataDir()).filePath("history.json"));
  if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    f.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
  }
}

void ClipboardService::load() {
  QFile f(QDir(dataDir()).filePath("history.json"));
  if (!f.open(QIODevice::ReadOnly)) {
    return;
  }
  for (const QJsonValue& v : QJsonDocument::fromJson(f.readAll()).array()) {
    const QJsonObject o = v.toObject();
    Entry e;
    e.id = o.value("id").toString();
    e.kind = o.value("kind").toString();
    e.text = o.value("text").toString();
    e.imagePath = o.value("image").toString();
    e.time = static_cast<qint64>(o.value("time").toDouble());
    e.pinned = o.value("pinned").toBool();
    if (e.kind == "image" && !QFile::exists(e.imagePath)) {
      continue;
    }
    m_entries.append(e);
  }
}
