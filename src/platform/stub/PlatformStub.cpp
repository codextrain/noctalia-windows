// Fake platform used when building on Linux/macOS. It lets the whole UI run (and be
// screenshotted) without Windows: a battery at 72%, 4 workspaces, volume at 45%.

#include "platform/Platform.h"

#include <cstdio>
#include <functional>

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
                                                       std::function<void()>, bool) {
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

// ── launcher / windows / hotkeys (stub) ─────────────────────────────────────

namespace platform {

  void initializeThread() {}

  std::vector<AppEntry> enumerateApps() {
    static const wchar_t* names[] = {
        L"Blender",           L"Calculadora",      L"Discord",        L"Explorador de Arquivos", L"Firefox",
        L"GIMP",              L"Google Chrome",    L"Microsoft Edge", L"Bloco de Notas",         L"OBS Studio",
        L"Paint",             L"Prompt de Comando", L"Spotify",       L"Steam",                  L"Terminal",
        L"Visual Studio Code", L"VLC media player", L"WhatsApp",      L"Word",                   L"Configurações",
    };
    std::vector<AppEntry> out;
    for (const wchar_t* n : names) {
      out.push_back({std::wstring(L"stub!") + n, n, false});
    }
    return out;
  }

  Image appIcon(const std::wstring& id, int size) {
    // A colored rounded square so the UI has something to show.
    Image img;
    img.width = img.height = size;
    img.bgra.resize(static_cast<size_t>(size) * size * 4);
    const unsigned h = static_cast<unsigned>(std::hash<std::wstring>{}(id));
    const std::uint8_t r = 80 + h % 150, g = 80 + (h / 7) % 150, b = 80 + (h / 49) % 150;
    const int rad = size / 5;
    for (int y = 0; y < size; ++y) {
      for (int x = 0; x < size; ++x) {
        const int dx = x < rad ? rad - x : (x >= size - rad ? x - (size - rad - 1) : 0);
        const int dy = y < rad ? rad - y : (y >= size - rad ? y - (size - rad - 1) : 0);
        const bool inside = dx * dx + dy * dy <= rad * rad;
        std::uint8_t* p = &img.bgra[(static_cast<size_t>(y) * size + x) * 4];
        p[0] = inside ? b : 0;
        p[1] = inside ? g : 0;
        p[2] = inside ? r : 0;
        p[3] = inside ? 255 : 0;
      }
    }
    return img;
  }

  Image fileIcon(const std::wstring& path, int size) { return appIcon(path, size); }

  bool launchApp(const std::wstring& id) {
    std::printf("[stub] launch %ls\n", id.c_str());
    return true;
  }

  std::vector<WindowEntry> enumerateWindows() {
    return {
        {.handle = 1, .title = L"Noctalia para Windows — Claude", .exePath = L"chrome.exe", .appId = L"stub!Google Chrome", .focused = true},
        {.handle = 2, .title = L"main.cpp — noctalia-win — Visual Studio Code", .exePath = L"code.exe", .appId = L"stub!Visual Studio Code"},
        {.handle = 3, .title = L"Spotify Premium", .exePath = L"spotify.exe", .appId = L"stub!Spotify", .minimized = true},
    };
  }
  void activateWindow(std::uintptr_t h) { std::printf("[stub] activate %zu\n", static_cast<size_t>(h)); }
  void closeWindow(std::uintptr_t) {}
  void minimizeWindow(std::uintptr_t) {}
  std::uintptr_t foregroundWindow() { return 0; }
  void bringToFront(void*) {}

  bool registerHotkey(int, const std::wstring&) { return true; }
  void unregisterHotkey(int) {}
  int hotkeyFromMessage(const void*) { return -1; }
  void sendPaste() { std::printf("[stub] paste\n"); }

} // namespace platform

// ── control center (stub) ───────────────────────────────────────────────────

namespace platform {

  std::vector<AudioDevice> listAudioDevices(bool capture) {
    if (capture) {
      return {{L"mic1", L"Microfone (Headset USB)", true}, {L"mic2", L"Microfone da webcam", false}};
    }
    return {{L"out1", L"Alto-falantes (Realtek Audio)", true}, {L"out2", L"Headset USB", false}, {L"out3", L"Monitor LG (HDMI)", false}};
  }
  bool setDefaultAudioDevice(const std::wstring&) { return true; }

  std::vector<AudioSession> listAudioSessions() {
    return {{L"s1", L"Spotify", L"spotify.exe", 0.8F, false, true},
            {L"s2", L"Google Chrome", L"chrome.exe", 0.55F, false, false},
            {L"s3", L"Discord", L"discord.exe", 1.0F, true, false}};
  }
  void setAudioSessionVolume(const std::wstring&, float) {}
  void setAudioSessionMuted(const std::wstring&, bool) {}

  std::vector<PowerPlan> listPowerPlans() {
    return {{L"{a}", L"Equilibrado", true}, {L"{b}", L"Alto desempenho", false}, {L"{c}", L"Economia de energia", false}};
  }
  bool setPowerPlan(const std::wstring&) { return true; }

  SystemStats querySystem() {
    return {.cpuPercent = 23.5, .memTotal = 16ULL << 30, .memUsed = 9ULL << 30, .uptimeSeconds = 3 * 3600 + 25 * 60,
            .diskTotal = 512ULL << 30, .diskFree = 180ULL << 30, .computerName = L"DIEGO-PC"};
  }

} // namespace platform

// ── media / radios / wifi / displays (stub) ─────────────────────────────────

namespace platform {

  namespace {
    class StubMedia final : public MediaController {
    public:
      explicit StubMedia(std::function<void(const MediaState&)> fn) : m_fn(std::move(fn)) { publish(); }
      void playPause() override {
        m_playing = !m_playing;
        publish();
      }
      void next() override { publish(); }
      void previous() override { publish(); }
      void seek(double) override { publish(); }

    private:
      void publish() {
        MediaState s;
        s.available = true;
        s.title = L"Midnight City";
        s.artist = L"M83";
        s.album = L"Hurry Up, We're Dreaming";
        s.appId = L"Spotify.exe";
        s.playing = m_playing;
        s.canNext = s.canPrevious = s.canSeek = true;
        s.position = 95;
        s.duration = 243;
        m_fn(s);
      }
      std::function<void(const MediaState&)> m_fn;
      bool m_playing = true;
    };
  } // namespace

  std::unique_ptr<MediaController> MediaController::create(std::function<void(const MediaState&)> onChange) {
    return std::make_unique<StubMedia>(std::move(onChange));
  }

  RadioState queryRadio(RadioKind kind) { return {true, kind == RadioKind::WiFi}; }
  bool setRadio(RadioKind, bool) { return true; }
  std::vector<BluetoothDevice> listBluetoothDevices() {
    return {{L"bt1", L"Galaxy Buds2", true, L"headset"}, {L"bt2", L"MX Master 3", false, L"mouse"}, {L"bt3", L"Xbox Wireless Controller", false, L"gamepad"}};
  }

  WifiState queryWifi() {
    WifiState s;
    s.available = true;
    s.connected = true;
    s.ssid = L"CasaDiego_5G";
    s.signal = 82;
    s.networks = {{L"CasaDiego_5G", 82, true, true, true}, {L"CasaDiego", 74, true, false, true},
                  {L"Vizinho 2.4", 40, true, false, false}, {L"Cafe Publico", 25, false, false, false}};
    return s;
  }
  void scanWifi() {}
  bool connectWifi(const std::wstring&, const std::wstring&) { return true; }
  bool disconnectWifi() { return true; }

  std::vector<DisplayInfo> listDisplays() {
    return {{L"\\\\.\\DISPLAY1", L"LG ULTRAGEAR", false, true, 70, 2560, 1440, 144},
            {L"\\\\.\\DISPLAY2", L"Dell P2419H", false, false, -1, 1920, 1080, 60}};
  }
  bool setDisplayBrightness(const std::wstring&, int) { return true; }

} // namespace platform
