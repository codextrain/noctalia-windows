#pragma once

#include <QAbstractNativeEventFilter>
#include <QHash>
#include <QObject>
#include <QStringList>
#include <QWindow>
#include <cstdint>

class Config;
class QLocalServer;

// Opens / closes Noctalia's panels (launcher, control center, ...). Like Noctalia, only one
// panel is open at a time. Also owns the global shortcuts ("keybinds" in config) and the
// command channel used by `noctalia.exe toggle launcher`.
//
// QML: Panels.current, Panels.toggle("launcher"), Panels.close(), Panels.screenIndex
class PanelService : public QObject, public QAbstractNativeEventFilter {
  Q_OBJECT
  Q_PROPERTY(QString current READ current NOTIFY currentChanged)
  Q_PROPERTY(int screenIndex READ screenIndex NOTIFY currentChanged)
  Q_PROPERTY(QStringList hotkeyErrors READ hotkeyErrors NOTIFY hotkeysChanged)

public:
  static constexpr const char* kServerName = "noctalia-win-commands";

  explicit PanelService(Config* config, QObject* parent = nullptr);
  ~PanelService() override;

  QString current() const { return m_current; }
  int screenIndex() const { return m_screenIndex; }
  QStringList hotkeyErrors() const { return m_hotkeyErrors; }

  Q_INVOKABLE void open(const QString& name);
  Q_INVOKABLE void toggle(const QString& name);
  Q_INVOKABLE void close();
  // Close the panel and give focus back to the window that had it; optionally paste (Ctrl+V).
  Q_INVOKABLE void closeAndRestoreFocus(bool paste);
  // Give one of our panel windows real keyboard focus.
  Q_INVOKABLE void focusWindow(QWindow* window);

  // Runs a command line from a second `noctalia.exe` instance: "toggle launcher", "close"...
  bool runCommand(const QStringList& args);

  bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

signals:
  void currentChanged();
  void hotkeysChanged();

private:
  void registerHotkeys();
  void unregisterHotkeys();
  int screenAtCursor() const;

  Config* m_config = nullptr;
  QString m_current;
  int m_screenIndex = 0;
  std::uintptr_t m_previousForeground = 0;
  QHash<int, QString> m_hotkeys; // id -> panel name
  QStringList m_hotkeyErrors;
  QLocalServer* m_server = nullptr;
};
