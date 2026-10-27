#include "services/PanelService.h"

#include "core/Config.h"
#include "platform/Platform.h"

#include <QCoreApplication>
#include <QCursor>
#include <QGuiApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QScreen>
#include <QTimer>
#ifdef Q_OS_WIN
#include <windows.h>
namespace {
HHOOK winKeyHook = nullptr, winMouseHook = nullptr;
PanelService* winPanels = nullptr;
bool winHeld = false, winCombined = false;
LRESULT CALLBACK launcherMouse(int code, WPARAM message, LPARAM data) {
  if (code == HC_ACTION && winHeld && message != WM_MOUSEMOVE)
    winCombined = true;
  return CallNextHookEx(nullptr, code, message, data);
}
LRESULT CALLBACK launcherKey(int code, WPARAM message, LPARAM data) {
  if (code != HC_ACTION) return CallNextHookEx(nullptr, code, message, data);
  const auto* key = reinterpret_cast<KBDLLHOOKSTRUCT*>(data);
  if (key->dwExtraInfo == 0x4e4f4354) return CallNextHookEx(nullptr, code, message, data);
  const bool isWin = key->vkCode == VK_LWIN || key->vkCode == VK_RWIN;
  const bool down = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
  if (isWin && down && !winHeld) {
    winHeld = true;
    winCombined = (GetAsyncKeyState(VK_CONTROL) & 0x8000)
               || (GetAsyncKeyState(VK_SHIFT) & 0x8000)
               || (GetAsyncKeyState(VK_MENU) & 0x8000)
               || (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
               || (GetAsyncKeyState(VK_RBUTTON) & 0x8000);
  } else if (!isWin && down && winHeld) {
    winCombined = true;
  } else if (isWin && !down && winHeld) {
    winHeld = false;
    if (!winCombined && winPanels) {
      // Mark Win as used before forwarding its release: suppress Start without
      // swallowing Win down, which would break Windows shortcuts.
      INPUT mask[2]{};
      for (auto& input : mask) { input.type = INPUT_KEYBOARD; input.ki.wVk = 0xE8; input.ki.dwExtraInfo = 0x4e4f4354; }
      mask[1].ki.dwFlags = KEYEVENTF_KEYUP;
      if (SendInput(2, mask, sizeof(INPUT)) == 2) {
        QMetaObject::invokeMethod(winPanels, [panel = winPanels] { panel->toggle("launcher"); }, Qt::QueuedConnection);
      }
    }
  }
  return CallNextHookEx(nullptr, code, message, data);
}
void disableWinLauncher() {
  if (winKeyHook) UnhookWindowsHookEx(winKeyHook);
  if (winMouseHook) UnhookWindowsHookEx(winMouseHook);
  winKeyHook = winMouseHook = nullptr;
  winPanels = nullptr;
  winHeld = winCombined = false;
}
}
#endif

PanelService::PanelService(Config* config, QObject* parent) : QObject(parent), m_config(config) {
  qApp->installNativeEventFilter(this);
  connect(m_config, &Config::changed, this, &PanelService::registerHotkeys);
  registerHotkeys();

  // `noctalia.exe toggle launcher` from another process (e.g. bound to a mouse button or AutoHotkey).
  m_server = new QLocalServer(this);
  QLocalServer::removeServer(kServerName);
  m_server->listen(kServerName);
  connect(m_server, &QLocalServer::newConnection, this, [this] {
    while (QLocalSocket* socket = m_server->nextPendingConnection()) {
      connect(socket, &QLocalSocket::readyRead, socket, [this, socket] {
        const QStringList args = QString::fromUtf8(socket->readAll()).split('\n', Qt::SkipEmptyParts);
        runCommand(args);
        socket->disconnectFromServer();
      });
      connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
    }
  });
}

PanelService::~PanelService() {
#ifdef Q_OS_WIN
  disableWinLauncher();
#endif
  unregisterHotkeys();
  if (qApp != nullptr) {
    qApp->removeNativeEventFilter(this);
  }
}

int PanelService::screenAtCursor() const {
  QScreen* s = QGuiApplication::screenAt(QCursor::pos());
  const QList<QScreen*> screens = QGuiApplication::screens();
  const int idx = static_cast<int>(screens.indexOf(s));
  return idx < 0 ? 0 : idx;
}

void PanelService::open(const QString& name) {
  if (name.isEmpty()) {
    close();
    return;
  }
  if (m_current.isEmpty()) {
    // Remember who had focus, to give it back when the panel closes (and paste there).
    m_previousForeground = platform::foregroundWindow();
  }
  m_screenIndex = screenAtCursor();
  m_current = name;
  emit currentChanged();
}

void PanelService::toggle(const QString& name) {
  if (m_current == name) {
    close();
  } else {
    open(name);
  }
}

void PanelService::close() {
  if (m_current.isEmpty()) {
    return;
  }
  m_current.clear();
  emit currentChanged();
}

void PanelService::closeAndRestoreFocus(bool paste) {
  const std::uintptr_t previous = m_previousForeground;
  close();
  if (previous == 0) {
    return;
  }
  // Let our window disappear first, then hand focus back.
  QTimer::singleShot(60, this, [previous, paste] {
    platform::activateWindow(previous);
    if (paste) {
      QTimer::singleShot(120, [] { platform::sendPaste(); });
    }
  });
}

void PanelService::focusWindow(QWindow* window) {
  if (window == nullptr) {
    return;
  }
  window->requestActivate();
  platform::bringToFront(reinterpret_cast<void*>(window->winId()));
}

bool PanelService::runCommand(const QStringList& args) {
  if (args.isEmpty()) {
    return false;
  }
  const QString cmd = args.value(0).toLower();
  const QString arg = args.value(1).toLower();
  if (cmd == "toggle" && !arg.isEmpty()) {
    toggle(arg);
  } else if (cmd == "open" && !arg.isEmpty()) {
    open(arg);
  } else if (cmd == "close") {
    close();
  } else if (cmd == "reload") {
    m_config->reload();
  } else if (cmd == "quit") {
    QCoreApplication::quit();
  } else {
    return false;
  }
  return true;
}

// ── global shortcuts ────────────────────────────────────────────────────────

void PanelService::unregisterHotkeys() {
  for (auto it = m_hotkeys.constBegin(); it != m_hotkeys.constEnd(); ++it) {
    platform::unregisterHotkey(it.key());
  }
  m_hotkeys.clear();
}

void PanelService::registerHotkeys() {
  unregisterHotkeys();
  m_hotkeyErrors.clear();
#ifdef Q_OS_WIN
  const bool useWin = m_config->windows().value("win_opens_launcher").toBool();
  if (!useWin) disableWinLauncher();
  else if (!winKeyHook) {
    winPanels = this;
    winMouseHook = SetWindowsHookExW(WH_MOUSE_LL, launcherMouse, GetModuleHandleW(nullptr), 0);
    winKeyHook = SetWindowsHookExW(WH_KEYBOARD_LL, launcherKey, GetModuleHandleW(nullptr), 0);
    if (!winKeyHook || !winMouseHook) {
      disableWinLauncher();
      m_hotkeyErrors.append(QStringLiteral("Não foi possível ativar Win para abrir o lançador."));
    }
  }
#endif

  const QVariantMap binds = m_config->keybinds();
  int id = 0x4E01;
  for (auto it = binds.constBegin(); it != binds.constEnd(); ++it) {
    const QString spec = it.value().toString().trimmed();
    if (spec.isEmpty()) {
      continue;
    }
    if (platform::registerHotkey(id, spec.toStdWString())) {
      m_hotkeys.insert(id, it.key());
    } else {
      m_hotkeyErrors.append(QStringLiteral("%1: \"%2\" (atalho inválido ou já usado por outro programa)").arg(it.key(), spec));
    }
    ++id;
  }
  emit hotkeysChanged();
}

bool PanelService::nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) {
  Q_UNUSED(result)
  if (eventType != "windows_generic_MSG") {
    return false;
  }
  const int id = platform::hotkeyFromMessage(message);
  if (id < 0 || !m_hotkeys.contains(id)) {
    return false;
  }
  toggle(m_hotkeys.value(id));
  return true;
}
