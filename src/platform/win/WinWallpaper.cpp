// Windows implementation: desktop wallpaper through IDesktopWallpaper (documented, Windows 8+).

#include "platform/Platform.h"

#include <windows.h>
#include <objbase.h>
#include <shobjidl.h>

namespace platform {

  namespace {
    IDesktopWallpaper* wallpaperApi() {
      IDesktopWallpaper* api = nullptr;
      CoCreateInstance(__uuidof(DesktopWallpaper), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&api));
      return api;
    }
  } // namespace

  std::wstring currentWallpaper() {
    std::wstring out;
    IDesktopWallpaper* api = wallpaperApi();
    if (api == nullptr) {
      return out;
    }
    LPWSTR monitor = nullptr;
    if (SUCCEEDED(api->GetMonitorDevicePathAt(0, &monitor)) && monitor != nullptr) {
      LPWSTR path = nullptr;
      if (SUCCEEDED(api->GetWallpaper(monitor, &path)) && path != nullptr) {
        out = path;
        CoTaskMemFree(path);
      }
      CoTaskMemFree(monitor);
    }
    api->Release();
    return out;
  }

  bool setWallpaper(const std::wstring& path, const std::wstring& fillMode) {
    IDesktopWallpaper* api = wallpaperApi();
    if (api == nullptr) {
      return false;
    }
    DESKTOP_WALLPAPER_POSITION pos = DWPOS_FILL;
    if (fillMode == L"fit") pos = DWPOS_FIT;
    else if (fillMode == L"stretch") pos = DWPOS_STRETCH;
    else if (fillMode == L"center") pos = DWPOS_CENTER;
    else if (fillMode == L"repeat") pos = DWPOS_TILE;
    else if (fillMode == L"span") pos = DWPOS_SPAN;
    api->SetPosition(pos);
    const bool ok = SUCCEEDED(api->SetWallpaper(nullptr, path.c_str())); // nullptr = every monitor
    api->Release();
    return ok;
  }

} // namespace platform
