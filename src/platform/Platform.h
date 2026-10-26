#pragma once

// Platform layer: everything that talks to Windows lives behind this header.
// It is plain C++ (no Qt) so it can be compiled and tested on its own.
// Implementations:
//   platform/win/*.cpp   -> real Windows code (Win32 / COM)
//   platform/stub/*.cpp  -> fake data, used when building on Linux for UI work

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace platform {

  // Call once at startup on the UI thread (initialises COM on Windows).
  void initialize();

  // ── Audio ────────────────────────────────────────────────────────────────
  class AudioEndpoint {
  public:
    // onChange: volume (0..1) and mute changed. May be called from ANY thread.
    // onDefaultDeviceChanged: default output device changed. May be called from ANY thread;
    //                         the owner should call rebind() on its own thread afterwards.
    static std::unique_ptr<AudioEndpoint> create(std::function<void(float, bool)> onChange,
                                                 std::function<void()> onDefaultDeviceChanged);
    virtual ~AudioEndpoint() = default;

    virtual bool rebind() = 0; // (re)attach to the current default output device
    virtual bool valid() const = 0;
    virtual float volume() const = 0;
    virtual bool muted() const = 0;
    virtual void setVolume(float value) = 0;
    virtual void setMuted(bool muted) = 0;
    virtual std::wstring deviceName() const = 0;
  };

  // ── Power ────────────────────────────────────────────────────────────────
  struct PowerStatus {
    bool hasBattery = false;
    int percent = -1;      // -1 = unknown
    bool acOnline = false; // plugged in
    bool charging = false;
    int secondsLeft = -1;  // -1 = unknown
  };
  PowerStatus queryPower();

  // ── Virtual desktops (Noctalia "workspaces") ─────────────────────────────
  struct DesktopState {
    int count = 1;
    int current = 0;                 // 0-based
    std::vector<std::wstring> names; // empty string = no custom name
    std::vector<bool> occupied;      // has at least one normal window
  };
  DesktopState queryDesktops(bool withOccupancy);
  void switchDesktop(int fromIndex, int toIndex);
  void createDesktop();

  // ── Windows shell shortcuts (used until our own panels exist) ────────────
  enum class ShellAction { StartMenu, Search, QuickSettings, NotificationCenter, PowerUserMenu, TaskView };
  void triggerShellAction(ShellAction action);

  // ── Session (Noctalia's session menu) ────────────────────────────────────
  enum class SessionAction { Lock, Logout, Suspend, Hibernate, Reboot, Shutdown };
  void sessionAction(SessionAction action);

  // ── App bar: reserve screen space like the Windows taskbar does ──────────
  struct RectI {
    int left = 0, top = 0, right = 0, bottom = 0;
    int width() const { return right - left; }
    int height() const { return bottom - top; }
  };

  enum class Edge { Top, Bottom };

  class AppBar {
  public:
    // hwnd: native window handle. callbackMessage: a WM_APP+N id we will receive notifications on.
    AppBar(void* hwnd, unsigned callbackMessage);
    ~AppBar();
    AppBar(const AppBar&) = delete;
    AppBar& operator=(const AppBar&) = delete;

    // Reserve `thicknessPx` physical pixels on `edge` of the monitor that holds the window.
    // Returns the reserved rectangle in physical pixels (the system may move it, e.g. if the
    // Windows taskbar sits on the same edge).
    RectI reserve(Edge edge, int thicknessPx);
    void release();
    bool registered() const { return m_registered; }

    // Monitor rectangle (physical pixels) of the monitor holding the window.
    RectI monitorRect() const;

    enum class Notify { None, PositionChanged, FullscreenEntered, FullscreenLeft };
    Notify handleCallback(std::uintptr_t wParam, std::intptr_t lParam);
    void notifyActivated();
    void notifyWindowPosChanged();
    unsigned callbackMessage() const { return m_message; }
    void* hwnd() const { return m_hwnd; }

  private:
    void* m_hwnd = nullptr;
    unsigned m_message = 0;
    bool m_registered = false;
  };

  // Turn the Windows taskbar's auto-hide on/off. Returns the previous auto-hide state.
  bool setTaskbarAutoHide(bool enabled);

  // Mark a window so it never shows in the taskbar / Alt+Tab and never steals focus.
  void makeShellWindow(void* hwnd);

} // namespace platform
