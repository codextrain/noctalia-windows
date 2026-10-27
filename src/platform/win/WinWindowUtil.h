#pragma once

// Shared helpers for code that walks top-level windows.

#include <windows.h>
#include <dwmapi.h>

namespace platform::win {

  // A window counts as a "user window" if it would appear in Alt+Tab / the taskbar.
  // Windows on other virtual desktops still count (they are cloaked by the shell).
  inline bool isUserWindow(HWND hwnd) {
    if (!IsWindowVisible(hwnd) || GetWindow(hwnd, GW_OWNER) != nullptr) {
      return false;
    }
    const LONG_PTR ex = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if ((ex & WS_EX_TOOLWINDOW) != 0 && (ex & WS_EX_APPWINDOW) == 0) {
      return false;
    }
    if (GetWindowTextLengthW(hwnd) == 0) {
      return false;
    }
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == GetCurrentProcessId()) {
      return false;
    }
    // Cloaked by the app itself (suspended Store apps, hidden UWP frames) -> not a real window.
    DWORD cloaked = 0;
    if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked)))) {
      if ((cloaked & (DWM_CLOAKED_APP | DWM_CLOAKED_INHERITED)) != 0) {
        return false;
      }
    }
    return true;
  }

  inline bool isCloakedByShell(HWND hwnd) {
    DWORD cloaked = 0;
    return SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))) &&
           (cloaked & DWM_CLOAKED_SHELL) != 0;
  }

} // namespace platform::win
