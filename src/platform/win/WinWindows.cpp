// Windows implementation: top-level windows, focus, global hotkeys, paste.

#include "platform/Platform.h"
#include "platform/win/WinInput.h"
#include "platform/win/WinWindowUtil.h"

#include <windows.h>
#include <propidl.h>
#include <propsys.h>
#include <shellapi.h>
#include <shobjidl.h>

#include <algorithm>
#include <cwctype>
#include <iterator>
#include <map>
#include <string>

namespace platform {

  namespace {

    // PKEY_AppUserModel_ID, defined locally (see WinAudio.cpp for why).
    const PROPERTYKEY kAppUserModelId = {
        {0x9F4C2855, 0x9F79, 0x4B39, {0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3}}, 5};

    std::wstring windowAppId(HWND hwnd) {
      std::wstring id;
      IPropertyStore* store = nullptr;
      if (SUCCEEDED(SHGetPropertyStoreForWindow(hwnd, IID_PPV_ARGS(&store)))) {
        PROPVARIANT pv;
        PropVariantInit(&pv);
        if (SUCCEEDED(store->GetValue(kAppUserModelId, &pv)) && pv.vt == VT_LPWSTR && pv.pwszVal != nullptr) {
          id = pv.pwszVal;
        }
        PropVariantClear(&pv);
        store->Release();
      }
      return id;
    }

    std::wstring processPath(HWND hwnd) {
      DWORD pid = 0;
      GetWindowThreadProcessId(hwnd, &pid);
      std::wstring path;
      HANDLE proc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
      if (proc != nullptr) {
        wchar_t buf[MAX_PATH * 2] = {};
        DWORD size = static_cast<DWORD>(std::size(buf));
        if (QueryFullProcessImageNameW(proc, 0, buf, &size)) {
          path.assign(buf, size);
        }
        CloseHandle(proc);
      }
      return path;
    }

    struct EnumCtx {
      std::vector<WindowEntry>* out = nullptr;
      HWND foreground = nullptr;
    };

    BOOL CALLBACK enumWindow(HWND hwnd, LPARAM param) {
      auto* ctx = reinterpret_cast<EnumCtx*>(param);
      if (!win::isUserWindow(hwnd)) {
        return TRUE;
      }
      WindowEntry e;
      e.handle = reinterpret_cast<std::uintptr_t>(hwnd);
      const int len = GetWindowTextLengthW(hwnd);
      e.title.resize(static_cast<size_t>(len) + 1);
      GetWindowTextW(hwnd, e.title.data(), len + 1);
      e.title.resize(static_cast<size_t>(len));
      e.exePath = processPath(hwnd);
      e.appId = windowAppId(hwnd);
      e.minimized = IsIconic(hwnd) != FALSE;
      e.focused = hwnd == ctx->foreground;
      e.onCurrentDesktop = !win::isCloakedByShell(hwnd);
      ctx->out->push_back(std::move(e));
      return TRUE;
    }

    // ── hotkey parsing ─────────────────────────────────────────────────────

    std::wstring lowerTrim(std::wstring s) {
      while (!s.empty() && std::iswspace(s.front())) {
        s.erase(s.begin());
      }
      while (!s.empty() && std::iswspace(s.back())) {
        s.pop_back();
      }
      std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
      return s;
    }

    UINT keyFromName(const std::wstring& k) {
      if (k.size() == 1) {
        const wchar_t c = k[0];
        if ((c >= L'a' && c <= L'z') || (c >= L'0' && c <= L'9')) {
          return static_cast<UINT>(std::towupper(c));
        }
        switch (c) {
        case L'`':
        case L'\'':
          return VK_OEM_3;
        case L'-':
          return VK_OEM_MINUS;
        case L'=':
          return VK_OEM_PLUS;
        case L',':
          return VK_OEM_COMMA;
        case L'.':
          return VK_OEM_PERIOD;
        case L'/':
          return VK_OEM_2;
        case L';':
          return VK_OEM_1;
        case L'[':
          return VK_OEM_4;
        case L']':
          return VK_OEM_6;
        case L'\\':
          return VK_OEM_5;
        default:
          return 0;
        }
      }
      if (k.size() >= 2 && k[0] == L'f') {
        const int n = _wtoi(k.c_str() + 1);
        if (n >= 1 && n <= 24) {
          return static_cast<UINT>(VK_F1 + n - 1);
        }
      }
      static const std::map<std::wstring, UINT> named{
          {L"space", VK_SPACE},     {L"enter", VK_RETURN},   {L"return", VK_RETURN}, {L"tab", VK_TAB},
          {L"esc", VK_ESCAPE},      {L"escape", VK_ESCAPE},  {L"backspace", VK_BACK}, {L"delete", VK_DELETE},
          {L"insert", VK_INSERT},   {L"home", VK_HOME},      {L"end", VK_END},        {L"pageup", VK_PRIOR},
          {L"pagedown", VK_NEXT},   {L"left", VK_LEFT},      {L"right", VK_RIGHT},    {L"up", VK_UP},
          {L"down", VK_DOWN},       {L"print", VK_SNAPSHOT}, {L"pause", VK_PAUSE},    {L"grave", VK_OEM_3},
          {L"slash", VK_OEM_2},     {L"comma", VK_OEM_COMMA}, {L"period", VK_OEM_PERIOD},
      };
      const auto it = named.find(k);
      return it == named.end() ? 0 : it->second;
    }

    bool parseHotkey(const std::wstring& spec, UINT& mods, UINT& vk) {
      mods = MOD_NOREPEAT;
      vk = 0;
      size_t start = 0;
      while (start <= spec.size()) {
        size_t plus = spec.find(L'+', start);
        // "Ctrl++" style: a trailing '+' means the plus key itself.
        if (plus == start && plus + 1 == spec.size()) {
          plus = std::wstring::npos;
        }
        const std::wstring part = lowerTrim(spec.substr(start, plus == std::wstring::npos ? std::wstring::npos : plus - start));
        if (part == L"ctrl" || part == L"control") {
          mods |= MOD_CONTROL;
        } else if (part == L"alt") {
          mods |= MOD_ALT;
        } else if (part == L"shift") {
          mods |= MOD_SHIFT;
        } else if (part == L"win" || part == L"super" || part == L"meta" || part == L"mod4") {
          mods |= MOD_WIN;
        } else if (!part.empty()) {
          if (vk != 0) {
            return false; // two non-modifier keys
          }
          vk = keyFromName(part);
          if (vk == 0) {
            return false;
          }
        }
        if (plus == std::wstring::npos) {
          break;
        }
        start = plus + 1;
      }
      return vk != 0;
    }

  } // namespace

  std::vector<WindowEntry> enumerateWindows() {
    std::vector<WindowEntry> out;
    EnumCtx ctx{&out, GetForegroundWindow()};
    EnumWindows(enumWindow, reinterpret_cast<LPARAM>(&ctx));
    return out;
  }

  void bringToFront(void* hwndPtr) {
    HWND hwnd = static_cast<HWND>(hwndPtr);
    if (hwnd == nullptr) {
      return;
    }
    if (IsIconic(hwnd)) {
      ShowWindow(hwnd, SW_RESTORE);
    }
    if (SetForegroundWindow(hwnd)) {
      return;
    }
    // Windows refuses SetForegroundWindow from background processes. Briefly attaching our
    // input queue to the foreground thread's is the standard, documented-behaviour workaround.
    const HWND fg = GetForegroundWindow();
    const DWORD fgThread = GetWindowThreadProcessId(fg, nullptr);
    const DWORD self = GetCurrentThreadId();
    if (fgThread != 0 && fgThread != self) {
      AttachThreadInput(fgThread, self, TRUE);
      BringWindowToTop(hwnd);
      SetForegroundWindow(hwnd);
      SetFocus(hwnd);
      AttachThreadInput(fgThread, self, FALSE);
    } else {
      BringWindowToTop(hwnd);
      SetForegroundWindow(hwnd);
    }
  }

  void activateWindow(std::uintptr_t handle) { bringToFront(reinterpret_cast<void*>(handle)); }

  void closeWindow(std::uintptr_t handle) {
    PostMessageW(reinterpret_cast<HWND>(handle), WM_CLOSE, 0, 0);
  }

  void minimizeWindow(std::uintptr_t handle) {
    ShowWindowAsync(reinterpret_cast<HWND>(handle), SW_MINIMIZE);
  }

  std::uintptr_t foregroundWindow() { return reinterpret_cast<std::uintptr_t>(GetForegroundWindow()); }

  bool registerHotkey(int id, const std::wstring& spec) {
    UINT mods = 0;
    UINT vk = 0;
    if (!parseHotkey(spec, mods, vk)) {
      return false;
    }
    // hwnd = nullptr: WM_HOTKEY is posted to this (the GUI) thread's message queue.
    return RegisterHotKey(nullptr, id, mods, vk) != FALSE;
  }

  void unregisterHotkey(int id) { UnregisterHotKey(nullptr, id); }

  int hotkeyFromMessage(const void* nativeMessage) {
    const MSG* msg = static_cast<const MSG*>(nativeMessage);
    if (msg == nullptr || msg->message != WM_HOTKEY) {
      return -1;
    }
    return static_cast<int>(msg->wParam);
  }

  void sendPaste() { win::sendChord({VK_LCONTROL, 'V'}); }

} // namespace platform
