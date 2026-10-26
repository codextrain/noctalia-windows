// Windows implementation of virtual desktops ("workspaces" in Noctalia).
//
// Windows has no public API to list or switch virtual desktops, but:
//  - Explorer keeps the desktop list and the current desktop in the registry (stable
//    since Windows 10, still true on Windows 11);
//  - IVirtualDesktopManager (documented) tells which desktop a window lives on, which
//    gives us "occupied" workspaces like Noctalia shows;
//  - switching uses the official shortcuts Ctrl+Win+Left/Right and Ctrl+Win+D.

#include "platform/Platform.h"
#include "platform/win/WinInput.h"

#include <windows.h>
#include <dwmapi.h>
#include <shobjidl.h>

#include <algorithm>
#include <string>

namespace platform {

  namespace {

    constexpr const wchar_t* kDesktopsKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VirtualDesktops";

    std::vector<BYTE> readBinary(const std::wstring& key, const wchar_t* value) {
      DWORD size = 0;
      if (RegGetValueW(HKEY_CURRENT_USER, key.c_str(), value, RRF_RT_REG_BINARY, nullptr, nullptr, &size) !=
              ERROR_SUCCESS ||
          size == 0) {
        return {};
      }
      std::vector<BYTE> buf(size);
      if (RegGetValueW(HKEY_CURRENT_USER, key.c_str(), value, RRF_RT_REG_BINARY, nullptr, buf.data(), &size) !=
          ERROR_SUCCESS) {
        return {};
      }
      buf.resize(size);
      return buf;
    }

    std::wstring readString(const std::wstring& key, const wchar_t* value) {
      DWORD size = 0;
      if (RegGetValueW(HKEY_CURRENT_USER, key.c_str(), value, RRF_RT_REG_SZ, nullptr, nullptr, &size) !=
              ERROR_SUCCESS ||
          size < sizeof(wchar_t)) {
        return {};
      }
      std::wstring s(size / sizeof(wchar_t), L'\0');
      if (RegGetValueW(HKEY_CURRENT_USER, key.c_str(), value, RRF_RT_REG_SZ, nullptr, s.data(), &size) !=
          ERROR_SUCCESS) {
        return {};
      }
      s.resize(wcsnlen(s.c_str(), s.size()));
      return s;
    }

    std::vector<GUID> desktopIds() {
      const auto raw = readBinary(kDesktopsKey, L"VirtualDesktopIDs");
      std::vector<GUID> ids(raw.size() / sizeof(GUID));
      if (!ids.empty()) {
        memcpy(ids.data(), raw.data(), ids.size() * sizeof(GUID));
      }
      return ids;
    }

    bool currentDesktopId(GUID& out) {
      // Windows 11 keeps it directly under VirtualDesktops...
      auto raw = readBinary(kDesktopsKey, L"CurrentVirtualDesktop");
      if (raw.size() < sizeof(GUID)) {
        // ...Windows 10 keeps it per logon session.
        DWORD session = 0;
        ProcessIdToSessionId(GetCurrentProcessId(), &session);
        const std::wstring key = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\SessionInfo\\" +
                                 std::to_wstring(session) + L"\\VirtualDesktops";
        raw = readBinary(key, L"CurrentVirtualDesktop");
      }
      if (raw.size() < sizeof(GUID)) {
        return false;
      }
      memcpy(&out, raw.data(), sizeof(GUID));
      return true;
    }

    std::wstring guidString(const GUID& id) {
      wchar_t buf[64] = {};
      StringFromGUID2(id, buf, 64);
      return buf;
    }

    // A window counts as "on a desktop" if a user would see it in Alt+Tab.
    bool isUserWindow(HWND hwnd) {
      if (!IsWindowVisible(hwnd) || GetWindow(hwnd, GW_OWNER) != nullptr) {
        return false;
      }
      const LONG_PTR ex = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
      if ((ex & WS_EX_TOOLWINDOW) != 0) {
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
      // Windows on other virtual desktops are cloaked by the shell; those still count.
      // Windows cloaked by their own app (suspended UWP apps etc.) do not.
      DWORD cloaked = 0;
      if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked)))) {
        if ((cloaked & (DWM_CLOAKED_APP | DWM_CLOAKED_INHERITED)) != 0) {
          return false;
        }
      }
      return true;
    }

    struct EnumContext {
      IVirtualDesktopManager* manager = nullptr;
      std::vector<GUID> occupied;
    };

    BOOL CALLBACK enumWindow(HWND hwnd, LPARAM param) {
      auto* ctx = reinterpret_cast<EnumContext*>(param);
      if (!isUserWindow(hwnd)) {
        return TRUE;
      }
      GUID id{};
      if (SUCCEEDED(ctx->manager->GetWindowDesktopId(hwnd, &id)) && id != GUID_NULL) {
        if (std::find(ctx->occupied.begin(), ctx->occupied.end(), id) == ctx->occupied.end()) {
          ctx->occupied.push_back(id);
        }
      }
      return TRUE;
    }

  } // namespace

  DesktopState queryDesktops(bool withOccupancy) {
    DesktopState state;
    const std::vector<GUID> ids = desktopIds();
    if (ids.empty()) {
      // Never created a second desktop: Windows doesn't write the list yet.
      state.count = 1;
      state.current = 0;
      state.names = {L""};
      state.occupied = {true};
      return state;
    }

    state.count = static_cast<int>(ids.size());
    GUID current{};
    if (currentDesktopId(current)) {
      const auto it = std::find(ids.begin(), ids.end(), current);
      state.current = it == ids.end() ? 0 : static_cast<int>(it - ids.begin());
    }

    state.names.reserve(ids.size());
    for (const GUID& id : ids) {
      state.names.push_back(readString(std::wstring(kDesktopsKey) + L"\\Desktops\\" + guidString(id), L"Name"));
    }

    state.occupied.assign(ids.size(), false);
    if (withOccupancy) {
      EnumContext ctx;
      if (SUCCEEDED(CoCreateInstance(__uuidof(VirtualDesktopManager), nullptr, CLSCTX_ALL,
                                     IID_PPV_ARGS(&ctx.manager)))) {
        EnumWindows(enumWindow, reinterpret_cast<LPARAM>(&ctx));
        ctx.manager->Release();
        for (size_t i = 0; i < ids.size(); ++i) {
          state.occupied[i] = std::find(ctx.occupied.begin(), ctx.occupied.end(), ids[i]) != ctx.occupied.end();
        }
      }
    }
    return state;
  }

  void switchDesktop(int fromIndex, int toIndex) {
    const int steps = toIndex - fromIndex;
    if (steps == 0) {
      return;
    }
    win::sendChord({VK_LCONTROL, VK_LWIN, static_cast<WORD>(steps > 0 ? VK_RIGHT : VK_LEFT)}, steps > 0 ? steps : -steps);
  }

  void createDesktop() { win::sendChord({VK_LCONTROL, VK_LWIN, 'D'}); }

} // namespace platform
