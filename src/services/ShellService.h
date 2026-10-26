#pragma once

#include <QObject>
#include <QPointer>
#include <cstdint>
#include <QWindow>
#include <memory>

namespace platform {
  class AppBar;
}
class Config;

// General shell actions for QML (Shell.startMenu(), Shell.quit(), ...) and the Windows taskbar
// auto-hide option.
class ShellService : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString version READ version CONSTANT)
  Q_PROPERTY(bool isWindows READ isWindows CONSTANT)

public:
  explicit ShellService(Config* config, QObject* parent = nullptr);
  ~ShellService() override;

  QString version() const;
  bool isWindows() const;

  // Windows' own UI, used by bar buttons until our panels (launcher, control center) exist.
  Q_INVOKABLE void startMenu() const;
  Q_INVOKABLE void search() const;
  Q_INVOKABLE void quickSettings() const;
  Q_INVOKABLE void notificationCenter() const;
  Q_INVOKABLE void powerMenu() const;
  Q_INVOKABLE void taskView() const;
  Q_INVOKABLE void openUri(const QString& uri) const;
  Q_INVOKABLE void quit() const;
  // "lock" | "logout" | "suspend" | "hibernate" | "reboot" | "shutdown"
  Q_INVOKABLE void session(const QString& action) const;

private:
  void applyTaskbarSetting();
  void restoreTaskbar();

  Config* m_config = nullptr;
  bool m_changedTaskbar = false;
  bool m_taskbarWasAutoHide = false;
};

// QML type that reserves screen space for a bar window (Windows "app bar" API).
//
//   AppBar { window: barWindow; edge: "top"; thickness: 44; active: true }
//
// `offset` tells QML how far from the screen edge the reserved strip starts (non-zero when
// e.g. the Windows taskbar sits on the same edge). `fullscreenApp` turns true while a
// fullscreen game/video is running on that monitor. Hide the bar *content* then, not the
// window: an invisible window gives its space back and stops receiving the notification.
class AppBarItem : public QObject {
  Q_OBJECT
  Q_PROPERTY(QWindow* window READ window WRITE setWindow NOTIFY windowChanged)
  Q_PROPERTY(QString edge READ edge WRITE setEdge NOTIFY settingsChanged)
  Q_PROPERTY(int thickness READ thickness WRITE setThickness NOTIFY settingsChanged)
  Q_PROPERTY(bool active READ active WRITE setActive NOTIFY settingsChanged)
  Q_PROPERTY(int offset READ offset NOTIFY offsetChanged)
  Q_PROPERTY(bool fullscreenApp READ fullscreenApp NOTIFY fullscreenAppChanged)

public:
  explicit AppBarItem(QObject* parent = nullptr);
  ~AppBarItem() override;

  QWindow* window() const { return m_window; }
  void setWindow(QWindow* w);
  QString edge() const { return m_edge; }
  void setEdge(const QString& e);
  int thickness() const { return m_thickness; }
  void setThickness(int t);
  bool active() const { return m_active; }
  void setActive(bool a);
  int offset() const { return m_offset; }
  bool fullscreenApp() const { return m_fullscreen; }

  // Called by the native event filter (Windows only).
  bool handleNativeMessage(unsigned message, std::uintptr_t wParam, std::intptr_t lParam);
  void* hwnd() const;

signals:
  void windowChanged();
  void settingsChanged();
  void offsetChanged();
  void fullscreenAppChanged();

private:
  void scheduleUpdate();
  void update();

  QPointer<QWindow> m_window;
  QString m_edge = QStringLiteral("top");
  int m_thickness = 0;
  bool m_active = true;
  int m_offset = 0;
  bool m_fullscreen = false;
  bool m_updateQueued = false;
  unsigned m_callbackMessage = 0;
  std::unique_ptr<platform::AppBar> m_appBar;
};
