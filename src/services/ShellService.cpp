#include "services/ShellService.h"

#include "core/Config.h"
#include "platform/Platform.h"

#include <QAbstractNativeEventFilter>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QHash>
#include <QScreen>
#include <QSet>
#include <QSysInfo>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QTimer>
#include <QUrl>

#include <cmath>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

// ── ShellService ────────────────────────────────────────────────────────────

ShellService::ShellService(Config* config, QObject* parent) : QObject(parent), m_config(config) {
  connect(m_config, &Config::changed, this, &ShellService::applyTaskbarSetting);
  // Always give the taskbar back, even on a normal quit from the menu.
  connect(qApp, &QCoreApplication::aboutToQuit, this, &ShellService::restoreTaskbar);

  // NiriWin may start after us (or stop): look for its status file now and every few seconds.
  connect(m_config, &Config::changed, this, &ShellService::findNiriStatus);
  m_niriTimer = new QTimer(this);
  m_niriTimer->setInterval(5000);
  connect(m_niriTimer, &QTimer::timeout, this, &ShellService::findNiriStatus);
  m_niriTimer->start();
  findNiriStatus();
  applyTaskbarSetting();
}

ShellService::~ShellService() { restoreTaskbar(); }

QString ShellService::version() const { return QCoreApplication::applicationVersion(); }

bool ShellService::isWindows() const {
#ifdef Q_OS_WIN
  return true;
#else
  return false;
#endif
}

QString ShellService::userName() const {
  QString name = qEnvironmentVariable("USERNAME");
  if (name.isEmpty()) {
    name = qEnvironmentVariable("USER");
  }
  return name;
}

QString ShellService::osName() const { return QSysInfo::prettyProductName(); }

namespace {
  constexpr const char* kRunKey = R"(HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run)";
}

bool ShellService::autostart() const {
#ifdef Q_OS_WIN
  QSettings run(kRunKey, QSettings::NativeFormat);
  return run.contains("Noctalia");
#else
  return false;
#endif
}

void ShellService::setAutostart(bool on) {
#ifdef Q_OS_WIN
  QSettings run(kRunKey, QSettings::NativeFormat);
  if (on) {
    run.setValue("Noctalia", "\"" + QDir::toNativeSeparators(QCoreApplication::applicationFilePath()) + "\"");
  } else {
    run.remove("Noctalia");
  }
#else
  Q_UNUSED(on)
#endif
  emit autostartChanged();
}

// Where is NiriWin's state/status.json? In order:
//   1. NOCTALIA_NIRIWIN_STATUS (file URL or path; set by tools/Start-Noctalia.ps1)
//   2. windows.niriwin_path in config.json (the status.json itself or the NiriWin folder)
//   3. next to a running process with "niri" in its name (<exe dir>\state, or one level up)
//   4. common install folders (C:\NiriWin, %LOCALAPPDATA%\NiriWin, ...)
void ShellService::findNiriStatus() {
  auto toUrl = [](const QString& file) { return QUrl::fromLocalFile(QDir::cleanPath(file)).toString(); };
  auto fromFolderOrFile = [](const QString& p) -> QString {
    const QFileInfo fi(p);
    if (fi.isFile()) return fi.absoluteFilePath();
    if (fi.isDir()) {
      for (const char* rel : {"state/status.json", "status.json", "../state/status.json"}) {
        const QFileInfo c(QDir(fi.absoluteFilePath()).filePath(QString::fromLatin1(rel)));
        if (c.isFile()) return c.absoluteFilePath();
      }
    }
    return {};
  };

  QString url;
  const QString env = qEnvironmentVariable("NOCTALIA_NIRIWIN_STATUS");
  if (!env.isEmpty()) {
    url = env.contains("://") ? env : toUrl(env);
  }
  if (url.isEmpty()) {
    const QString configured = m_config->windows().value("niriwin_path").toString().trimmed();
    if (!configured.isEmpty()) {
      const QString f = fromFolderOrFile(QDir::fromNativeSeparators(configured).remove('"'));
      if (!f.isEmpty()) url = toUrl(f);
    }
  }
  if (url.isEmpty()) {
    for (const auto& exe : platform::runningProcessPaths(L"niri")) {
      const QString f = fromFolderOrFile(QFileInfo(QString::fromStdWString(exe)).absolutePath());
      if (!f.isEmpty()) { url = toUrl(f); break; }
    }
  }
  if (url.isEmpty()) {
    const QString home = QDir::homePath();
    const QStringList folders = {
        "C:/NiriWin", "C:/niriwin",
        qEnvironmentVariable("LOCALAPPDATA") + "/NiriWin",
        qEnvironmentVariable("APPDATA") + "/NiriWin",
        home + "/NiriWin", home + "/Documents/NiriWin", home + "/Documentos/NiriWin",
        home + "/Desktop/NiriWin", home + "/Downloads/NiriWin",
        qEnvironmentVariable("ProgramFiles") + "/NiriWin",
    };
    for (const auto& folder : folders) {
      const QString f = fromFolderOrFile(folder);
      if (!f.isEmpty()) { url = toUrl(f); break; }
    }
  }
  if (url != m_niriStatusUrl) {
    m_niriStatusUrl = url;
    emit niriStatusUrlChanged();
  }
}

QString ShellService::env(const QString& name) const { return qEnvironmentVariable(name.toUtf8().constData()); }

void ShellService::startMenu() const { platform::triggerShellAction(platform::ShellAction::StartMenu); }
void ShellService::search() const { platform::triggerShellAction(platform::ShellAction::Search); }
void ShellService::quickSettings() const { platform::triggerShellAction(platform::ShellAction::QuickSettings); }
void ShellService::notificationCenter() const {
  platform::triggerShellAction(platform::ShellAction::NotificationCenter);
}
void ShellService::powerMenu() const { platform::triggerShellAction(platform::ShellAction::PowerUserMenu); }
void ShellService::taskView() const { platform::triggerShellAction(platform::ShellAction::TaskView); }
void ShellService::trayOverflow() const { platform::triggerShellAction(platform::ShellAction::TrayOverflow); }
void ShellService::openUri(const QString& uri) const { QDesktopServices::openUrl(QUrl(uri)); }
void ShellService::quit() const { QCoreApplication::quit(); }

void ShellService::session(const QString& action) const {
  using platform::SessionAction;
  static const QHash<QString, SessionAction> map{
      {"lock", SessionAction::Lock},           {"logout", SessionAction::Logout}, {"suspend", SessionAction::Suspend},
      {"hibernate", SessionAction::Hibernate}, {"reboot", SessionAction::Reboot}, {"shutdown", SessionAction::Shutdown},
  };
  const auto it = map.constFind(action);
  if (it != map.constEnd()) {
    platform::sessionAction(it.value());
  }
}

void ShellService::applyTaskbarSetting() {
  const bool wanted = m_config->windows().value("autohide_taskbar", false).toBool();
  if (wanted && !m_changedTaskbar) {
    m_taskbarWasAutoHide = platform::setTaskbarAutoHide(true);
    m_changedTaskbar = true;
  } else if (!wanted && m_changedTaskbar) {
    restoreTaskbar();
  }
}

void ShellService::restoreTaskbar() {
  if (m_changedTaskbar) {
    platform::setTaskbarAutoHide(m_taskbarWasAutoHide);
    m_changedTaskbar = false;
  }
}

// ── AppBarItem ──────────────────────────────────────────────────────────────

namespace {

  // One process-wide native event filter that routes app-bar messages to the right bar.
  class AppBarEventFilter final : public QAbstractNativeEventFilter {
  public:
    static AppBarEventFilter& instance() {
      static AppBarEventFilter filter;
      return filter;
    }
    void add(AppBarItem* item) {
      if (!m_installed && qApp != nullptr) {
        qApp->installNativeEventFilter(this);
        m_installed = true;
      }
      m_items.insert(item);
    }
    void remove(AppBarItem* item) { m_items.remove(item); }

    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override {
#ifdef Q_OS_WIN
      if (eventType != "windows_generic_MSG") {
        return false;
      }
      const MSG* msg = static_cast<const MSG*>(message);
      for (AppBarItem* item : std::as_const(m_items)) {
        if (item->hwnd() == msg->hwnd &&
            item->handleNativeMessage(msg->message, static_cast<std::uintptr_t>(msg->wParam),
                                      static_cast<std::intptr_t>(msg->lParam))) {
          if (result != nullptr) {
            *result = 0;
          }
          return true;
        }
      }
#else
      Q_UNUSED(eventType)
      Q_UNUSED(message)
      Q_UNUSED(result)
#endif
      return false;
    }

  private:
    QSet<AppBarItem*> m_items;
    bool m_installed = false;
  };

  unsigned nextCallbackMessage() {
    static unsigned next = 0x8000 /* WM_APP */ + 0x4E00;
    return next++;
  }

} // namespace

AppBarItem::AppBarItem(QObject* parent) : QObject(parent), m_callbackMessage(nextCallbackMessage()) {
  AppBarEventFilter::instance().add(this);
}

AppBarItem::~AppBarItem() {
  AppBarEventFilter::instance().remove(this);
  m_appBar.reset(); // releases the reserved space
}

void* AppBarItem::hwnd() const { return m_appBar ? m_appBar->hwnd() : nullptr; }

void AppBarItem::setWindow(QWindow* w) {
  if (w == m_window) {
    return;
  }
  if (m_window) {
    disconnect(m_window, nullptr, this, nullptr);
  }
  m_appBar.reset();
  m_window = w;
  if (m_window) {
    connect(m_window, &QWindow::visibleChanged, this, &AppBarItem::scheduleUpdate);
    connect(m_window, &QWindow::screenChanged, this, &AppBarItem::scheduleUpdate);
    connect(m_window, &QObject::destroyed, this, [this] { m_appBar.reset(); });
  }
  emit windowChanged();
  scheduleUpdate();
}

void AppBarItem::setEdge(const QString& e) {
  if (e == m_edge) {
    return;
  }
  m_edge = e;
  emit settingsChanged();
  scheduleUpdate();
}

void AppBarItem::setThickness(int t) {
  if (t == m_thickness) {
    return;
  }
  m_thickness = t;
  emit settingsChanged();
  scheduleUpdate();
}

void AppBarItem::setActive(bool a) {
  if (a == m_active) {
    return;
  }
  m_active = a;
  emit settingsChanged();
  scheduleUpdate();
}

void AppBarItem::scheduleUpdate() {
  if (m_updateQueued) {
    return;
  }
  m_updateQueued = true;
  // Coalesce: QML usually sets several properties in a row.
  QTimer::singleShot(0, this, [this] {
    m_updateQueued = false;
    update();
  });
}

void AppBarItem::update() {
  if (!m_window || !m_window->isVisible()) {
    if (m_appBar) {
      m_appBar->release();
    }
    return;
  }

  if (!m_appBar) {
    void* handle = reinterpret_cast<void*>(m_window->winId());
    m_appBar = std::make_unique<platform::AppBar>(handle, m_callbackMessage);
    platform::makeShellWindow(handle);
    if (QScreen* s = m_window->screen()) {
      connect(s, &QScreen::geometryChanged, this, &AppBarItem::scheduleUpdate, Qt::UniqueConnection);
    }
  }

  if (!m_active || m_thickness <= 0) {
    m_appBar->release();
    if (m_offset != 0) {
      m_offset = 0;
      emit offsetChanged();
    }
    return;
  }

  const qreal dpr = m_window->devicePixelRatio();
  const int px = static_cast<int>(std::lround(m_thickness * dpr));
  const bool top = m_edge != QLatin1String("bottom");
  const platform::RectI rc = m_appBar->reserve(top ? platform::Edge::Top : platform::Edge::Bottom, px);
  const platform::RectI mon = m_appBar->monitorRect();

  int offset = 0;
  if (rc.height() > 0 && mon.height() > 0) {
    const int physical = top ? rc.top - mon.top : mon.bottom - rc.bottom;
    offset = static_cast<int>(std::lround(physical / dpr));
  }
  if (offset != m_offset) {
    m_offset = offset;
    emit offsetChanged();
  }
}

bool AppBarItem::handleNativeMessage(unsigned message, std::uintptr_t wParam, std::intptr_t lParam) {
  if (!m_appBar) {
    return false;
  }
#ifdef Q_OS_WIN
  static const UINT taskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");
  if (message == taskbarCreated) {
    // Explorer forgot all registered app bars. Recreate the native registration.
    m_appBar.reset();
    if (m_fullscreen) { m_fullscreen=false; emit fullscreenAppChanged(); }
    scheduleUpdate();
    return false;
  }
  if (message == WM_ACTIVATE) {
    m_appBar->notifyActivated();
    return false; // let Qt see it too
  }
  if (message == WM_WINDOWPOSCHANGED) {
    m_appBar->notifyWindowPosChanged();
    return false;
  }
  if (message == WM_DISPLAYCHANGE || message == WM_DPICHANGED) {
    scheduleUpdate();
    return false;
  }
#endif
  if (message != m_callbackMessage) {
    return false;
  }
  switch (m_appBar->handleCallback(wParam, lParam)) {
  case platform::AppBar::Notify::PositionChanged:
    scheduleUpdate();
    break;
  case platform::AppBar::Notify::FullscreenEntered:
    if (!m_fullscreen) {
      m_fullscreen = true;
      emit fullscreenAppChanged();
    }
    break;
  case platform::AppBar::Notify::FullscreenLeft:
    if (m_fullscreen) {
      m_fullscreen = false;
      emit fullscreenAppChanged();
    }
    break;
  case platform::AppBar::Notify::None:
    break;
  }
  return true;
}
