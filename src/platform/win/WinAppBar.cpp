// Windows implementation of the "app bar": the same mechanism the Windows taskbar uses to
// keep maximised windows from covering it. This is the Windows equivalent of the
// Wayland layer-shell "exclusive zone" Noctalia uses on Linux.

#include "platform/Platform.h"

#include <windows.h>
#include <shellapi.h>

namespace platform {

  AppBar::AppBar(void* hwnd, unsigned callbackMessage) : m_hwnd(hwnd), m_message(callbackMessage) {}

  AppBar::~AppBar() { release(); }

  RectI AppBar::monitorRect() const {
    RectI out;
    HMONITOR mon = MonitorFromWindow(static_cast<HWND>(m_hwnd), MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    if (GetMonitorInfoW(mon, &mi)) {
      out = {mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom};
    }
    return out;
  }

  RectI AppBar::reserve(Edge edge, int thicknessPx) {
    HWND hwnd = static_cast<HWND>(m_hwnd);
    if (hwnd == nullptr || thicknessPx <= 0) {
      release();
      return {};
    }

    if (!m_registered) {
      APPBARDATA reg{};
      reg.cbSize = sizeof(reg);
      reg.hWnd = hwnd;
      reg.uCallbackMessage = m_message;
      if (!SHAppBarMessage(ABM_NEW, &reg)) {
        return {};
      }
      m_registered = true;
    }

    const RectI mon = monitorRect();
    APPBARDATA abd{};
    abd.cbSize = sizeof(abd);
    abd.hWnd = hwnd;
    abd.uEdge = edge == Edge::Top ? ABE_TOP : ABE_BOTTOM;
    abd.rc = {mon.left, mon.top, mon.right, mon.bottom};

    auto clampToThickness = [&]() {
      if (edge == Edge::Top) {
        abd.rc.bottom = abd.rc.top + thicknessPx;
      } else {
        abd.rc.top = abd.rc.bottom - thicknessPx;
      }
    };

    clampToThickness();
    SHAppBarMessage(ABM_QUERYPOS, &abd); // the system may shift us (e.g. taskbar on the same edge)
    clampToThickness();
    SHAppBarMessage(ABM_SETPOS, &abd);

    return {abd.rc.left, abd.rc.top, abd.rc.right, abd.rc.bottom};
  }

  void AppBar::release() {
    if (!m_registered) {
      return;
    }
    APPBARDATA abd{};
    abd.cbSize = sizeof(abd);
    abd.hWnd = static_cast<HWND>(m_hwnd);
    SHAppBarMessage(ABM_REMOVE, &abd);
    m_registered = false;
  }

  AppBar::Notify AppBar::handleCallback(std::uintptr_t wParam, std::intptr_t lParam) {
    switch (static_cast<UINT>(wParam)) {
    case ABN_POSCHANGED:
      return Notify::PositionChanged;
    case ABN_FULLSCREENAPP:
      return lParam != 0 ? Notify::FullscreenEntered : Notify::FullscreenLeft;
    default:
      return Notify::None;
    }
  }

  void AppBar::notifyActivated() {
    if (!m_registered) {
      return;
    }
    APPBARDATA abd{};
    abd.cbSize = sizeof(abd);
    abd.hWnd = static_cast<HWND>(m_hwnd);
    SHAppBarMessage(ABM_ACTIVATE, &abd);
  }

  void AppBar::notifyWindowPosChanged() {
    if (!m_registered) {
      return;
    }
    APPBARDATA abd{};
    abd.cbSize = sizeof(abd);
    abd.hWnd = static_cast<HWND>(m_hwnd);
    SHAppBarMessage(ABM_WINDOWPOSCHANGED, &abd);
  }

} // namespace platform
