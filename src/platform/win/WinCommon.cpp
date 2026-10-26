// Windows implementation: COM init, power status, shell shortcuts, window styling, taskbar.

#include "platform/Platform.h"
#include "platform/win/WinInput.h"

#include <windows.h>
#include <dwmapi.h>
#include <objbase.h>
#include <shellapi.h>

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif
#ifndef DWMWA_COLOR_NONE
#define DWMWA_COLOR_NONE 0xFFFFFFFE
#endif

namespace platform {

  void initialize() {
    // Qt already initialises COM as STA on the GUI thread; this is a harmless safety net.
    const HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    (void)hr; // S_OK, S_FALSE or RPC_E_CHANGED_MODE are all fine for us.
  }

  PowerStatus queryPower() {
    PowerStatus out;
    SYSTEM_POWER_STATUS s{};
    if (!GetSystemPowerStatus(&s)) {
      return out;
    }
    // BatteryFlag: 128 = no system battery, 255 = unknown status.
    out.hasBattery = s.BatteryFlag != 128 && s.BatteryFlag != 255;
    out.percent = s.BatteryLifePercent == 255 ? -1 : static_cast<int>(s.BatteryLifePercent);
    out.acOnline = s.ACLineStatus == 1;
    out.charging = (s.BatteryFlag & 8) != 0;
    out.secondsLeft = s.BatteryLifeTime == static_cast<DWORD>(-1) ? -1 : static_cast<int>(s.BatteryLifeTime);
    return out;
  }

  void triggerShellAction(ShellAction action) {
    switch (action) {
    case ShellAction::StartMenu:
      win::sendChord({VK_LWIN});
      break;
    case ShellAction::Search:
      win::sendChord({VK_LWIN, 'S'});
      break;
    case ShellAction::QuickSettings:
      win::sendChord({VK_LWIN, 'A'});
      break;
    case ShellAction::NotificationCenter:
      win::sendChord({VK_LWIN, 'N'});
      break;
    case ShellAction::PowerUserMenu:
      win::sendChord({VK_LWIN, 'X'});
      break;
    case ShellAction::TaskView:
      win::sendChord({VK_LWIN, VK_TAB});
      break;
    }
  }

  void makeShellWindow(void* hwndPtr) {
    HWND hwnd = static_cast<HWND>(hwndPtr);
    if (hwnd == nullptr) {
      return;
    }
    LONG_PTR ex = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    ex |= WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
    ex &= ~static_cast<LONG_PTR>(WS_EX_APPWINDOW);
    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, ex);

    // Windows 11 draws its own rounded corners and a 1px border on every top-level
    // window. Our bar draws its own shape, so switch both off.
    const int noRound = 1; // DWMWCP_DONOTROUND
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &noRound, sizeof(noRound));
    const COLORREF noBorder = DWMWA_COLOR_NONE;
    DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &noBorder, sizeof(noBorder));
    const BOOL excluded = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_EXCLUDED_FROM_PEEK, &excluded, sizeof(excluded));

    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
  }

  bool setTaskbarAutoHide(bool enabled) {
    APPBARDATA abd{};
    abd.cbSize = sizeof(abd);
    abd.hWnd = FindWindowW(L"Shell_TrayWnd", nullptr);
    const auto state = static_cast<UINT>(SHAppBarMessage(ABM_GETSTATE, &abd));
    const bool previous = (state & ABS_AUTOHIDE) != 0;
    if (previous != enabled) {
      abd.lParam = enabled ? (state | ABS_AUTOHIDE) : (state & ~static_cast<UINT>(ABS_AUTOHIDE));
      SHAppBarMessage(ABM_SETSTATE, &abd);
    }
    return previous;
  }

} // namespace platform

// ── Session actions ─────────────────────────────────────────────────────────

#include <powrprof.h>

namespace platform {

  namespace {
    void runShutdownExe(const wchar_t* args) {
      // shutdown.exe handles the SE_SHUTDOWN privilege for us.
      ShellExecuteW(nullptr, L"open", L"shutdown.exe", args, nullptr, SW_HIDE);
    }
  } // namespace

  void sessionAction(SessionAction action) {
    switch (action) {
    case SessionAction::Lock:
      LockWorkStation();
      break;
    case SessionAction::Logout:
      ExitWindowsEx(EWX_LOGOFF, SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_FLAG_PLANNED);
      break;
    case SessionAction::Suspend:
      SetSuspendState(FALSE, FALSE, FALSE);
      break;
    case SessionAction::Hibernate:
      SetSuspendState(TRUE, FALSE, FALSE);
      break;
    case SessionAction::Reboot:
      runShutdownExe(L"/r /t 0");
      break;
    case SessionAction::Shutdown:
      runShutdownExe(L"/s /t 0");
      break;
    }
  }

} // namespace platform
