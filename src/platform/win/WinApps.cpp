// Windows implementation: installed apps, icons and launching.
//
// The shell's virtual "Applications" folder (shell:AppsFolder) is the same list the Start menu
// uses: classic desktop programs AND Microsoft Store apps, with their real names and icons.

#include "platform/Platform.h"

#include <windows.h>
#include <objbase.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shobjidl.h>

#include <algorithm>
#include <cwctype>

namespace platform {

  namespace {

    std::wstring lower(std::wstring s) {
      std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
      return s;
    }

    // Entries the Start menu also lists but nobody wants in a launcher.
    bool isNoise(const std::wstring& name) {
      const std::wstring n = lower(name);
      static const wchar_t* noise[] = {L"uninstall", L"desinstalar", L"remover ", L"readme", L"leia-me",
                                       L"release notes", L"notas de vers", L"help", L"ajuda do"};
      for (const wchar_t* w : noise) {
        if (n.rfind(w, 0) == 0) {
          return true;
        }
      }
      return false;
    }

    Image fromHBitmap(HBITMAP bmp) {
      Image img;
      BITMAP bm{};
      if (bmp == nullptr || GetObjectW(bmp, sizeof(bm), &bm) == 0 || bm.bmWidth <= 0 || bm.bmHeight == 0) {
        return img;
      }
      const int w = bm.bmWidth;
      const int h = bm.bmHeight < 0 ? -bm.bmHeight : bm.bmHeight;

      BITMAPINFO bi{};
      bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
      bi.bmiHeader.biWidth = w;
      bi.bmiHeader.biHeight = -h; // top-down rows
      bi.bmiHeader.biPlanes = 1;
      bi.bmiHeader.biBitCount = 32;
      bi.bmiHeader.biCompression = BI_RGB;

      img.bgra.resize(static_cast<size_t>(w) * h * 4);
      HDC dc = GetDC(nullptr);
      const int lines = GetDIBits(dc, bmp, 0, static_cast<UINT>(h), img.bgra.data(), &bi, DIB_RGB_COLORS);
      ReleaseDC(nullptr, dc);
      if (lines == 0) {
        img.bgra.clear();
        return img;
      }
      // Some icons come without an alpha channel at all: treat them as opaque.
      bool anyAlpha = false;
      for (size_t i = 3; i < img.bgra.size(); i += 4) {
        if (img.bgra[i] != 0) {
          anyAlpha = true;
          break;
        }
      }
      if (!anyAlpha) {
        for (size_t i = 3; i < img.bgra.size(); i += 4) {
          img.bgra[i] = 255;
        }
      }
      img.width = w;
      img.height = h;
      return img;
    }

    Image imageFromShellItem(IShellItem* item, int size) {
      Image img;
      IShellItemImageFactory* factory = nullptr;
      if (item == nullptr || FAILED(item->QueryInterface(IID_PPV_ARGS(&factory)))) {
        return img;
      }
      HBITMAP bmp = nullptr;
      const SIZE sz{size, size};
      if (SUCCEEDED(factory->GetImage(sz, SIIGBF_ICONONLY | SIIGBF_BIGGERSIZEOK, &bmp)) && bmp != nullptr) {
        img = fromHBitmap(bmp);
        DeleteObject(bmp);
      }
      factory->Release();
      return img;
    }

    Image imageFromParsingName(const std::wstring& name, int size) {
      IShellItem* item = nullptr;
      if (FAILED(SHCreateItemFromParsingName(name.c_str(), nullptr, IID_PPV_ARGS(&item)))) {
        return {};
      }
      Image img = imageFromShellItem(item, size);
      item->Release();
      return img;
    }

  } // namespace

  void initializeThread() {
    // Worker threads use the multithreaded apartment; harmless if already initialised.
    const HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    (void)hr;
  }

  std::vector<AppEntry> enumerateApps() {
    std::vector<AppEntry> out;
    IShellItem* folder = nullptr;
    if (FAILED(SHCreateItemFromParsingName(L"shell:AppsFolder", nullptr, IID_PPV_ARGS(&folder)))) {
      return out;
    }
    IEnumShellItems* items = nullptr;
    if (SUCCEEDED(folder->BindToHandler(nullptr, BHID_EnumItems, IID_PPV_ARGS(&items)))) {
      IShellItem* item = nullptr;
      while (items->Next(1, &item, nullptr) == S_OK) {
        PWSTR name = nullptr;
        PWSTR parsing = nullptr;
        if (SUCCEEDED(item->GetDisplayName(SIGDN_NORMALDISPLAY, &name)) &&
            SUCCEEDED(item->GetDisplayName(SIGDN_PARENTRELATIVEPARSING, &parsing))) {
          AppEntry e;
          e.id = parsing;
          e.name = name;
          // Store apps are addressed by AUMID: "Publisher.App_hash!AppId".
          e.storeApp = e.id.find(L'!') != std::wstring::npos;
          // Plain web links / documents pinned in the Start menu are not apps.
          const std::wstring idLower = lower(e.id);
          const bool isUrl = idLower.rfind(L"http", 0) == 0;
          if (!e.name.empty() && !isUrl && !isNoise(e.name)) {
            out.push_back(std::move(e));
          }
        }
        CoTaskMemFree(name);
        CoTaskMemFree(parsing);
        item->Release();
      }
      items->Release();
    }
    folder->Release();

    // Same name twice (e.g. per-user and per-machine shortcut): keep the first.
    std::sort(out.begin(), out.end(), [](const AppEntry& a, const AppEntry& b) { return lower(a.name) < lower(b.name); });
    out.erase(std::unique(out.begin(), out.end(), [](const AppEntry& a, const AppEntry& b) { return lower(a.name) == lower(b.name); }),
              out.end());
    return out;
  }

  Image appIcon(const std::wstring& id, int size) { return imageFromParsingName(L"shell:AppsFolder\\" + id, size); }

  Image fileIcon(const std::wstring& path, int size) { return imageFromParsingName(path, size); }

  bool launchApp(const std::wstring& id) {
    const std::wstring target = L"shell:AppsFolder\\" + id;
    const auto result = reinterpret_cast<INT_PTR>(
        ShellExecuteW(nullptr, L"open", target.c_str(), nullptr, nullptr, SW_SHOWNORMAL));
    return result > 32;
  }

} // namespace platform
