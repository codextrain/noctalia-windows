// Fake platform used when building on Linux/macOS. It lets the whole UI run (and be
// screenshotted) without Windows: a battery at 72%, 4 workspaces, volume at 45%.

#include "platform/Platform.h"

#include <cstdio>

namespace platform {

  namespace {
    int g_current = 0;
    int g_count = 4;

    class StubAudio final : public AudioEndpoint {
    public:
      explicit StubAudio(std::function<void(float, bool)> fn) : m_fn(std::move(fn)) {}
      bool rebind() override { return true; }
      bool valid() const override { return true; }
      float volume() const override { return m_volume; }
      bool muted() const override { return m_muted; }
      void setVolume(float v) override {
        m_volume = v < 0 ? 0 : (v > 1 ? 1 : v);
        m_fn(m_volume, m_muted);
      }
      void setMuted(bool m) override {
        m_muted = m;
        m_fn(m_volume, m_muted);
      }
      std::wstring deviceName() const override { return L"Alto-falantes (simulado)"; }

    private:
      std::function<void(float, bool)> m_fn;
      float m_volume = 0.45F;
      bool m_muted = false;
    };
  } // namespace

  void initialize() {}

  std::unique_ptr<AudioEndpoint> AudioEndpoint::create(std::function<void(float, bool)> onChange,
                                                       std::function<void()>) {
    return std::make_unique<StubAudio>(std::move(onChange));
  }

  PowerStatus queryPower() { return {.hasBattery = true, .percent = 72, .acOnline = false, .charging = false, .secondsLeft = 9000}; }

  DesktopState queryDesktops(bool) {
    DesktopState s;
    s.count = g_count;
    s.current = g_current;
    s.names.assign(static_cast<size_t>(g_count), L"");
    s.occupied.assign(static_cast<size_t>(g_count), false);
    s.occupied[0] = true;
    if (g_count > 2) {
      s.occupied[2] = true;
    }
    return s;
  }

  void switchDesktop(int, int to) {
    if (to >= 0 && to < g_count) {
      g_current = to;
    }
  }

  void createDesktop() { ++g_count; }

  void triggerShellAction(ShellAction a) { std::printf("[stub] shell action %d\n", static_cast<int>(a)); }

  void sessionAction(SessionAction a) { std::printf("[stub] session action %d\n", static_cast<int>(a)); }

  AppBar::AppBar(void* hwnd, unsigned msg) : m_hwnd(hwnd), m_message(msg) {}
  AppBar::~AppBar() = default;
  RectI AppBar::monitorRect() const { return {}; }
  RectI AppBar::reserve(Edge, int) { return {}; }
  void AppBar::release() {}
  AppBar::Notify AppBar::handleCallback(std::uintptr_t, std::intptr_t) { return Notify::None; }
  void AppBar::notifyActivated() {}
  void AppBar::notifyWindowPosChanged() {}

  bool setTaskbarAutoHide(bool) { return false; }
  void makeShellWindow(void*) {}

} // namespace platform
