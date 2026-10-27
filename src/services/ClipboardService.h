#pragma once

#include <QObject>
#include <QTimer>
#include <QVariantList>

class Config;

// Clipboard history (Noctalia's clipboard panel): remembers copied text and images, keeps
// pinned items across restarts, and puts an entry back on the clipboard (+ pastes it).
//
// Respects the Windows "don't record this" flags password managers set, like Win+V does.
// QML: Clipboard.entries [{id, kind, text, preview, image, time, pinned}], activate(id), remove(id),
//      togglePin(id), clear()
class ClipboardService : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariantList entries READ entries NOTIFY changed)
  Q_PROPERTY(bool enabled READ enabled NOTIFY changed)

public:
  explicit ClipboardService(Config* config, QObject* parent = nullptr);

  QVariantList entries() const;
  bool enabled() const;

  // Copies the entry back to the clipboard. Returns true if the panel should paste it.
  Q_INVOKABLE bool activate(const QString& id);
  Q_INVOKABLE void remove(const QString& id);
  Q_INVOKABLE void togglePin(const QString& id);
  Q_INVOKABLE void clear(); // keeps pinned entries

signals:
  void changed();

private:
  struct Entry {
    QString id;
    QString kind; // "text" | "image"
    QString text;
    QString imagePath;
    qint64 time = 0;
    bool pinned = false;
  };

  void onClipboardChanged();
  void trim();
  void save();
  void load();
  QString dataDir() const;

  Config* m_config = nullptr;
  QList<Entry> m_entries; // newest first
  bool m_ignoreNext = false;
  QTimer m_saveTimer;
  QTimer m_debounce;
};
