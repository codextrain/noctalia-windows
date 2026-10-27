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
    // capture = false: default speakers/headphones; true: default microphone.
    static std::unique_ptr<AudioEndpoint> create(std::function<void(float, bool)> onChange,
                                                 std::function<void()> onDefaultDeviceChanged, bool capture = false);
    virtual ~AudioEndpoint() = default;

    virtual bool rebind() = 0; // (re)attach to the current default output device
    virtual bool valid() const = 0;
    virtual float volume() const = 0;
    virtual bool muted() const = 0;
    virtual void setVolume(float value) = 0;
    virtual void setMuted(bool muted) = 0;
    virtual std::wstring deviceName() const = 0;
  };

  struct AudioDevice {
    std::wstring id;
    std::wstring name;
    bool isDefault = false;
  };
  std::vector<AudioDevice> listAudioDevices(bool capture);
  bool setDefaultAudioDevice(const std::wstring& id); // all roles (console, multimedia, communications)

  // Per-application volume (the Windows "volume mixer").
  struct AudioSession {
    std::wstring id; // session instance identifier
    std::wstring name;
    std::wstring exePath;
    float volume = 1.0F;
    bool muted = false;
    bool active = false; // currently playing sound
  };
  std::vector<AudioSession> listAudioSessions();
  void setAudioSessionVolume(const std::wstring& id, float volume);
  void setAudioSessionMuted(const std::wstring& id, bool muted);

  // ── Power ────────────────────────────────────────────────────────────────
  struct PowerStatus {
    bool hasBattery = false;
    int percent = -1;      // -1 = unknown
    bool acOnline = false; // plugged in
    bool charging = false;
    int secondsLeft = -1;  // -1 = unknown
  };
  PowerStatus queryPower();

  struct PowerPlan {
    std::wstring guid;
    std::wstring name;
    bool active = false;
  };
  std::vector<PowerPlan> listPowerPlans();
  bool setPowerPlan(const std::wstring& guid);

  // ── System monitor ───────────────────────────────────────────────────────
  struct SystemStats {
    double cpuPercent = 0;          // since the previous call
    std::uint64_t memTotal = 0;     // bytes
    std::uint64_t memUsed = 0;
    std::uint64_t uptimeSeconds = 0;
    std::uint64_t diskTotal = 0;    // system drive
    std::uint64_t diskFree = 0;
    std::wstring computerName;
  };
  SystemStats querySystem();

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

  // ── Threads ──────────────────────────────────────────────────────────────
  // Call at the start of any worker thread that uses the functions below (COM on Windows).
  void initializeThread();

  // ── Images (icons) ───────────────────────────────────────────────────────
  struct Image {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> bgra; // premultiplied BGRA, top-down (= QImage::Format_ARGB32_Premultiplied)
    bool valid() const { return width > 0 && height > 0; }
  };

  // ── Installed apps (Noctalia launcher "Applications" provider) ───────────
  struct AppEntry {
    std::wstring id;   // shell:AppsFolder parsing name (AUMID for Store apps, path-like for desktop apps)
    std::wstring name; // display name
    bool storeApp = false;
  };
  std::vector<AppEntry> enumerateApps(); // slow (100-500 ms): call from a worker thread
  Image appIcon(const std::wstring& id, int size);
  Image fileIcon(const std::wstring& path, int size);
  bool launchApp(const std::wstring& id);

  // ── Top-level windows (launcher "Windows" provider, dock) ────────────────
  struct WindowEntry {
    std::uintptr_t handle = 0;
    std::wstring title;
    std::wstring exePath;
    std::wstring appId; // AppUserModelID when the window has one (Store apps, pinned apps)
    bool minimized = false;
    bool focused = false;
    bool onCurrentDesktop = true;
  };
  std::vector<WindowEntry> enumerateWindows();
  void activateWindow(std::uintptr_t handle);
  void closeWindow(std::uintptr_t handle);
  void minimizeWindow(std::uintptr_t handle);
  std::uintptr_t foregroundWindow();
  // Bring one of OUR windows to the front with keyboard focus (works around focus-stealing rules).
  void bringToFront(void* hwnd);

  // ── Global hotkeys ───────────────────────────────────────────────────────
  // spec: "Alt+Space", "Ctrl+Shift+V", "Win+Alt+L", "F12"... Returns false if the spec is invalid
  // or the combination is already taken by Windows / another app.
  bool registerHotkey(int id, const std::wstring& spec);
  void unregisterHotkey(int id);
  // Pass a native message (Windows: MSG*). Returns the hotkey id, or -1 if it isn't a hotkey.
  int hotkeyFromMessage(const void* nativeMessage);

  // ── Media ("now playing", Windows' system media controls) ────────────────
  struct MediaState {
    bool available = false;
    std::wstring title;
    std::wstring artist;
    std::wstring album;
    std::wstring appId; // AUMID of the player (Spotify, Chrome...)
    bool playing = false;
    bool canNext = false;
    bool canPrevious = false;
    bool canSeek = false;
    double position = 0; // seconds, as of `positionTimestampMs`
    double duration = 0; // seconds (0 = unknown / live)
    std::int64_t positionTimestampMs = 0;
    std::uint64_t artVersion = 0;  // bumps when the cover changes
    std::vector<std::uint8_t> art; // encoded image (PNG/JPEG), may be empty
  };
  class MediaController {
  public:
    // onChange is called from a worker thread whenever anything changes.
    static std::unique_ptr<MediaController> create(std::function<void(const MediaState&)> onChange);
    virtual ~MediaController() = default;
    virtual void playPause() = 0;
    virtual void next() = 0;
    virtual void previous() = 0;
    virtual void seek(double seconds) = 0;
  };

  // ── Radios (Wi-Fi / Bluetooth on-off) ────────────────────────────────────
  enum class RadioKind { WiFi, Bluetooth };
  struct RadioState {
    bool present = false;
    bool on = false;
  };
  RadioState queryRadio(RadioKind kind); // may block ~50ms: call from a worker thread
  bool setRadio(RadioKind kind, bool on);

  // ── Bluetooth devices ────────────────────────────────────────────────────
  struct BluetoothDevice {
    std::wstring id;
    std::wstring name;
    bool connected = false;
    std::wstring kind; // "audio", "headset", "mouse", "keyboard", "gamepad", "phone", "computer", ""
  };
  std::vector<BluetoothDevice> listBluetoothDevices(); // paired devices; blocking, worker thread

  // ── Wi-Fi ────────────────────────────────────────────────────────────────
  struct WifiNetwork {
    std::wstring ssid;
    int signal = 0; // 0..100
    bool secured = false;
    bool connected = false;
    bool saved = false; // Windows already has a profile (password) for it
  };
  struct WifiState {
    bool available = false; // there is a Wi-Fi adapter
    bool connected = false;
    std::wstring ssid;
    int signal = 0;
    std::vector<WifiNetwork> networks;
  };
  WifiState queryWifi();
  void scanWifi(); // asks the adapter to refresh the list (results arrive within a few seconds)
  // password: ignored when the network is open or already saved. Returns false on immediate failure.
  bool connectWifi(const std::wstring& ssid, const std::wstring& password);
  bool disconnectWifi();

  // ── Displays & brightness ────────────────────────────────────────────────
  struct DisplayInfo {
    std::wstring id;   // stable for this session
    std::wstring name; // "LG ULTRAGEAR", "Tela integrada"
    bool internal = false;
    bool brightnessSupported = false;
    int brightness = -1; // 0..100
    int width = 0, height = 0, refreshRate = 0;
  };
  std::vector<DisplayInfo> listDisplays();                        // slow (DDC/CI): worker thread
  bool setDisplayBrightness(const std::wstring& id, int percent); // slow (DDC/CI): worker thread

  // ── Input helpers ────────────────────────────────────────────────────────
  void sendPaste(); // Ctrl+V into the focused window

} // namespace platform
