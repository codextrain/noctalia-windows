// Windows implementation: displays and brightness.
//
//  - Laptop panels: WMI (root\WMI WmiMonitorBrightness), the same thing the brightness keys use.
//  - External monitors: DDC/CI through dxva2 (Get/SetMonitorBrightness). Most monitors support it;
//    a few need "DDC/CI" turned on in their own on-screen menu.

#include "platform/Platform.h"

#include <windows.h>
#include <highlevelmonitorconfigurationapi.h>
#include <physicalmonitorenumerationapi.h>
#include <oleauto.h>
#include <wbemidl.h>

#include <string>
#include <vector>

namespace platform {

  namespace {

    struct Monitor {
      HMONITOR handle = nullptr;
      std::wstring device; // \\.\DISPLAY1
    };

    BOOL CALLBACK collectMonitor(HMONITOR mon, HDC, LPRECT, LPARAM param) {
      auto* list = reinterpret_cast<std::vector<Monitor>*>(param);
      MONITORINFOEXW mi{};
      mi.cbSize = sizeof(mi);
      if (GetMonitorInfoW(mon, &mi)) {
        list->push_back({mon, mi.szDevice});
      }
      return TRUE;
    }

    std::vector<Monitor> monitors() {
      std::vector<Monitor> list;
      EnumDisplayMonitors(nullptr, nullptr, collectMonitor, reinterpret_cast<LPARAM>(&list));
      return list;
    }

    struct TargetInfo {
      std::wstring friendlyName;
      bool internal = false;
    };

    // Friendly monitor names ("LG ULTRAGEAR") and whether a display is the built-in panel.
    TargetInfo targetFor(const std::wstring& gdiDevice) {
      TargetInfo out;
      UINT32 pathCount = 0, modeCount = 0;
      if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &pathCount, &modeCount) != ERROR_SUCCESS) {
        return out;
      }
      std::vector<DISPLAYCONFIG_PATH_INFO> paths(pathCount);
      std::vector<DISPLAYCONFIG_MODE_INFO> modes(modeCount);
      if (QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &pathCount, paths.data(), &modeCount, modes.data(), nullptr) !=
          ERROR_SUCCESS) {
        return out;
      }
      for (UINT32 i = 0; i < pathCount; ++i) {
        DISPLAYCONFIG_SOURCE_DEVICE_NAME source{};
        source.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
        source.header.size = sizeof(source);
        source.header.adapterId = paths[i].sourceInfo.adapterId;
        source.header.id = paths[i].sourceInfo.id;
        if (DisplayConfigGetDeviceInfo(&source.header) != ERROR_SUCCESS || gdiDevice != source.viewGdiDeviceName) {
          continue;
        }
        DISPLAYCONFIG_TARGET_DEVICE_NAME target{};
        target.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
        target.header.size = sizeof(target);
        target.header.adapterId = paths[i].targetInfo.adapterId;
        target.header.id = paths[i].targetInfo.id;
        if (DisplayConfigGetDeviceInfo(&target.header) == ERROR_SUCCESS) {
          out.friendlyName = target.monitorFriendlyDeviceName;
        }
        const auto tech = paths[i].targetInfo.outputTechnology;
        out.internal = tech == DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INTERNAL ||
                       tech == DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DISPLAYPORT_EMBEDDED ||
                       tech == DISPLAYCONFIG_OUTPUT_TECHNOLOGY_UDI_EMBEDDED;
        break;
      }
      return out;
    }

    // ── DDC/CI ─────────────────────────────────────────────────────────────

    int ddcGet(HMONITOR mon) {
      DWORD count = 0;
      if (!GetNumberOfPhysicalMonitorsFromHMONITOR(mon, &count) || count == 0) {
        return -1;
      }
      std::vector<PHYSICAL_MONITOR> phys(count);
      if (!GetPhysicalMonitorsFromHMONITOR(mon, count, phys.data())) {
        return -1;
      }
      int result = -1;
      DWORD minV = 0, cur = 0, maxV = 0;
      if (GetMonitorBrightness(phys[0].hPhysicalMonitor, &minV, &cur, &maxV) && maxV > minV) {
        result = static_cast<int>((cur - minV) * 100 / (maxV - minV));
      }
      DestroyPhysicalMonitors(count, phys.data());
      return result;
    }

    bool ddcSet(HMONITOR mon, int percent) {
      DWORD count = 0;
      if (!GetNumberOfPhysicalMonitorsFromHMONITOR(mon, &count) || count == 0) {
        return false;
      }
      std::vector<PHYSICAL_MONITOR> phys(count);
      if (!GetPhysicalMonitorsFromHMONITOR(mon, count, phys.data())) {
        return false;
      }
      bool ok = false;
      DWORD minV = 0, cur = 0, maxV = 0;
      for (DWORD i = 0; i < count; ++i) {
        if (GetMonitorBrightness(phys[i].hPhysicalMonitor, &minV, &cur, &maxV) && maxV > minV) {
          const DWORD value = minV + static_cast<DWORD>((maxV - minV) * percent / 100);
          ok = SetMonitorBrightness(phys[i].hPhysicalMonitor, value) || ok;
        }
      }
      DestroyPhysicalMonitors(count, phys.data());
      return ok;
    }

    // ── WMI (laptop panel) ───────────────────────────────────────────────────

    IWbemServices* wmi() {
      IWbemLocator* locator = nullptr;
      if (FAILED(CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator,
                                  reinterpret_cast<void**>(&locator)))) {
        return nullptr;
      }
      IWbemServices* services = nullptr;
      BSTR ns = SysAllocString(L"ROOT\\WMI");
      const HRESULT hr = locator->ConnectServer(ns, nullptr, nullptr, nullptr, 0, nullptr, nullptr, &services);
      SysFreeString(ns);
      locator->Release();
      if (FAILED(hr)) {
        return nullptr;
      }
      CoSetProxyBlanket(services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL,
                        RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);
      return services;
    }

    // Runs `query` and returns the first object (caller releases).
    IWbemClassObject* wmiFirst(IWbemServices* svc, const wchar_t* query) {
      BSTR lang = SysAllocString(L"WQL");
      BSTR q = SysAllocString(query);
      IEnumWbemClassObject* e = nullptr;
      const HRESULT hr =
          svc->ExecQuery(lang, q, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &e);
      SysFreeString(lang);
      SysFreeString(q);
      if (FAILED(hr) || e == nullptr) {
        return nullptr;
      }
      IWbemClassObject* obj = nullptr;
      ULONG got = 0;
      e->Next(WBEM_INFINITE, 1, &obj, &got);
      e->Release();
      return got == 1 ? obj : nullptr;
    }

    int wmiGet() {
      IWbemServices* svc = wmi();
      if (svc == nullptr) {
        return -1;
      }
      int result = -1;
      if (IWbemClassObject* obj = wmiFirst(svc, L"SELECT CurrentBrightness FROM WmiMonitorBrightness WHERE Active=TRUE")) {
        VARIANT v;
        VariantInit(&v);
        if (SUCCEEDED(obj->Get(L"CurrentBrightness", 0, &v, nullptr, nullptr))) {
          result = v.vt == VT_UI1 ? v.bVal : (v.vt == VT_I4 ? v.lVal : -1);
        }
        VariantClear(&v);
        obj->Release();
      }
      svc->Release();
      return result;
    }

    bool wmiSet(int percent) {
      IWbemServices* svc = wmi();
      if (svc == nullptr) {
        return false;
      }
      bool ok = false;
      IWbemClassObject* inst = wmiFirst(svc, L"SELECT * FROM WmiMonitorBrightnessMethods WHERE Active=TRUE");
      if (inst != nullptr) {
        VARIANT path;
        VariantInit(&path);
        inst->Get(L"__PATH", 0, &path, nullptr, nullptr);

        IWbemClassObject* cls = nullptr;
        BSTR clsName = SysAllocString(L"WmiMonitorBrightnessMethods");
        IWbemClassObject* inSig = nullptr;
        IWbemClassObject* inParams = nullptr;
        if (SUCCEEDED(svc->GetObject(clsName, 0, nullptr, &cls, nullptr)) &&
            SUCCEEDED(cls->GetMethod(L"WmiSetBrightness", 0, &inSig, nullptr)) && inSig != nullptr &&
            SUCCEEDED(inSig->SpawnInstance(0, &inParams))) {
          VARIANT timeout;
          VariantInit(&timeout);
          timeout.vt = VT_I4;
          timeout.lVal = 1;
          VARIANT level;
          VariantInit(&level);
          level.vt = VT_UI1;
          level.bVal = static_cast<BYTE>(percent);
          inParams->Put(L"Timeout", 0, &timeout, 0);
          inParams->Put(L"Brightness", 0, &level, 0);
          BSTR method = SysAllocString(L"WmiSetBrightness");
          ok = SUCCEEDED(svc->ExecMethod(path.bstrVal, method, 0, nullptr, inParams, nullptr, nullptr));
          SysFreeString(method);
        }
        if (inParams) inParams->Release();
        if (inSig) inSig->Release();
        if (cls) cls->Release();
        SysFreeString(clsName);
        VariantClear(&path);
        inst->Release();
      }
      svc->Release();
      return ok;
    }

  } // namespace

  std::vector<DisplayInfo> listDisplays() {
    std::vector<DisplayInfo> out;
    int index = 1;
    for (const Monitor& m : monitors()) {
      DisplayInfo d;
      d.id = m.device;
      const TargetInfo t = targetFor(m.device);
      d.internal = t.internal;
      d.name = !t.friendlyName.empty() ? t.friendlyName : (t.internal ? L"Tela integrada" : L"Monitor " + std::to_wstring(index));
      DEVMODEW dm{};
      dm.dmSize = sizeof(dm);
      if (EnumDisplaySettingsW(m.device.c_str(), ENUM_CURRENT_SETTINGS, &dm)) {
        d.width = static_cast<int>(dm.dmPelsWidth);
        d.height = static_cast<int>(dm.dmPelsHeight);
        d.refreshRate = static_cast<int>(dm.dmDisplayFrequency);
      }
      d.brightness = d.internal ? wmiGet() : ddcGet(m.handle);
      d.brightnessSupported = d.brightness >= 0;
      out.push_back(std::move(d));
      ++index;
    }
    return out;
  }

  bool setDisplayBrightness(const std::wstring& id, int percent) {
    percent = percent < 0 ? 0 : (percent > 100 ? 100 : percent);
    for (const Monitor& m : monitors()) {
      if (m.device != id) {
        continue;
      }
      return targetFor(m.device).internal ? wmiSet(percent) : ddcSet(m.handle, percent);
    }
    return false;
  }

} // namespace platform
