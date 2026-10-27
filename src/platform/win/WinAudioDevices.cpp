// Windows implementation: audio device list, default-device switching, per-app volume.

#include "platform/Platform.h"

#include <windows.h>
#include <audiopolicy.h>
#include <endpointvolume.h>
#include <mmdeviceapi.h>
#include <propidl.h>

#include <string>

namespace platform {

  namespace {

    const PROPERTYKEY kDeviceFriendlyName = {
        {0xa45c254e, 0xdf1c, 0x4efd, {0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0}}, 14};

    template <typename T> void safeRelease(T*& p) {
      if (p != nullptr) {
        p->Release();
        p = nullptr;
      }
    }

    // Undocumented but stable since Windows 7: the interface the Sound control panel uses to
    // change the default device. Every "audio switcher" app relies on it.
    // {f8679f50-850a-41cf-9c72-430f290290c8}
    struct IPolicyConfig : public IUnknown {
      virtual HRESULT STDMETHODCALLTYPE GetMixFormat(PCWSTR, void**) = 0;
      virtual HRESULT STDMETHODCALLTYPE GetDeviceFormat(PCWSTR, INT, void**) = 0;
      virtual HRESULT STDMETHODCALLTYPE ResetDeviceFormat(PCWSTR) = 0;
      virtual HRESULT STDMETHODCALLTYPE SetDeviceFormat(PCWSTR, void*, void*) = 0;
      virtual HRESULT STDMETHODCALLTYPE GetProcessingPeriod(PCWSTR, INT, void*, void*) = 0;
      virtual HRESULT STDMETHODCALLTYPE SetProcessingPeriod(PCWSTR, void*) = 0;
      virtual HRESULT STDMETHODCALLTYPE GetShareMode(PCWSTR, void*) = 0;
      virtual HRESULT STDMETHODCALLTYPE SetShareMode(PCWSTR, void*) = 0;
      virtual HRESULT STDMETHODCALLTYPE GetPropertyValue(PCWSTR, const PROPERTYKEY&, PROPVARIANT*) = 0;
      virtual HRESULT STDMETHODCALLTYPE SetPropertyValue(PCWSTR, const PROPERTYKEY&, PROPVARIANT*) = 0;
      virtual HRESULT STDMETHODCALLTYPE SetDefaultEndpoint(PCWSTR deviceId, ERole role) = 0;
      virtual HRESULT STDMETHODCALLTYPE SetEndpointVisibility(PCWSTR, INT) = 0;
    };
    // {870af99c-171d-4f9e-af0d-e63df40c2bc9}
    const CLSID kPolicyConfigClient = {0x870af99c, 0x171d, 0x4f9e, {0xaf, 0x0d, 0xe6, 0x3d, 0xf4, 0x0c, 0x2b, 0xc9}};
    const IID kIPolicyConfig = {0xf8679f50, 0x850a, 0x41cf, {0x9c, 0x72, 0x43, 0x0f, 0x29, 0x02, 0x90, 0xc8}};

    IMMDeviceEnumerator* enumerator() {
      IMMDeviceEnumerator* e = nullptr;
      CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&e));
      return e;
    }

    std::wstring deviceName(IMMDevice* dev) {
      std::wstring name;
      IPropertyStore* store = nullptr;
      if (SUCCEEDED(dev->OpenPropertyStore(STGM_READ, &store))) {
        PROPVARIANT pv;
        PropVariantInit(&pv);
        if (SUCCEEDED(store->GetValue(kDeviceFriendlyName, &pv)) && pv.vt == VT_LPWSTR) {
          name = pv.pwszVal;
        }
        PropVariantClear(&pv);
        store->Release();
      }
      return name;
    }

    std::wstring processPath(DWORD pid) {
      std::wstring path;
      HANDLE proc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
      if (proc != nullptr) {
        wchar_t buf[MAX_PATH * 2] = {};
        DWORD size = MAX_PATH * 2;
        if (QueryFullProcessImageNameW(proc, 0, buf, &size)) {
          path.assign(buf, size);
        }
        CloseHandle(proc);
      }
      return path;
    }

    std::wstring fileStem(const std::wstring& path) {
      const size_t slash = path.find_last_of(L"\\/");
      std::wstring name = slash == std::wstring::npos ? path : path.substr(slash + 1);
      const size_t dot = name.rfind(L'.');
      if (dot != std::wstring::npos) {
        name.resize(dot);
      }
      if (!name.empty()) {
        name[0] = static_cast<wchar_t>(towupper(name[0]));
      }
      return name;
    }

    // Calls fn(control2, simpleVolume) for every live session on the default output device.
    template <typename Fn> void forEachSession(Fn&& fn) {
      IMMDeviceEnumerator* e = enumerator();
      if (e == nullptr) {
        return;
      }
      IMMDevice* dev = nullptr;
      IAudioSessionManager2* mgr = nullptr;
      IAudioSessionEnumerator* sessions = nullptr;
      if (SUCCEEDED(e->GetDefaultAudioEndpoint(eRender, eConsole, &dev)) &&
          SUCCEEDED(dev->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&mgr))) &&
          SUCCEEDED(mgr->GetSessionEnumerator(&sessions))) {
        int count = 0;
        sessions->GetCount(&count);
        for (int i = 0; i < count; ++i) {
          IAudioSessionControl* ctl = nullptr;
          IAudioSessionControl2* ctl2 = nullptr;
          ISimpleAudioVolume* vol = nullptr;
          if (SUCCEEDED(sessions->GetSession(i, &ctl)) &&
              SUCCEEDED(ctl->QueryInterface(IID_PPV_ARGS(&ctl2))) &&
              SUCCEEDED(ctl->QueryInterface(IID_PPV_ARGS(&vol)))) {
            AudioSessionState state = AudioSessionStateInactive;
            ctl2->GetState(&state);
            if (state != AudioSessionStateExpired) {
              const bool stop = fn(ctl2, vol, state);
              if (stop) {
                safeRelease(vol);
                safeRelease(ctl2);
                safeRelease(ctl);
                break;
              }
            }
          }
          safeRelease(vol);
          safeRelease(ctl2);
          safeRelease(ctl);
        }
      }
      safeRelease(sessions);
      safeRelease(mgr);
      safeRelease(dev);
      e->Release();
    }

    std::wstring sessionId(IAudioSessionControl2* ctl2) {
      std::wstring id;
      LPWSTR s = nullptr;
      if (SUCCEEDED(ctl2->GetSessionInstanceIdentifier(&s)) && s != nullptr) {
        id = s;
        CoTaskMemFree(s);
      }
      return id;
    }

  } // namespace

  std::vector<AudioDevice> listAudioDevices(bool capture) {
    std::vector<AudioDevice> out;
    IMMDeviceEnumerator* e = enumerator();
    if (e == nullptr) {
      return out;
    }
    const EDataFlow flow = capture ? eCapture : eRender;
    std::wstring defaultId;
    IMMDevice* def = nullptr;
    if (SUCCEEDED(e->GetDefaultAudioEndpoint(flow, eConsole, &def))) {
      LPWSTR id = nullptr;
      if (SUCCEEDED(def->GetId(&id))) {
        defaultId = id;
        CoTaskMemFree(id);
      }
      def->Release();
    }
    IMMDeviceCollection* devices = nullptr;
    if (SUCCEEDED(e->EnumAudioEndpoints(flow, DEVICE_STATE_ACTIVE, &devices))) {
      UINT count = 0;
      devices->GetCount(&count);
      for (UINT i = 0; i < count; ++i) {
        IMMDevice* dev = nullptr;
        if (SUCCEEDED(devices->Item(i, &dev))) {
          LPWSTR id = nullptr;
          if (SUCCEEDED(dev->GetId(&id))) {
            AudioDevice d;
            d.id = id;
            d.name = deviceName(dev);
            d.isDefault = d.id == defaultId;
            out.push_back(std::move(d));
            CoTaskMemFree(id);
          }
          dev->Release();
        }
      }
      devices->Release();
    }
    e->Release();
    return out;
  }

  bool setDefaultAudioDevice(const std::wstring& id) {
    IPolicyConfig* policy = nullptr;
    if (FAILED(CoCreateInstance(kPolicyConfigClient, nullptr, CLSCTX_ALL, kIPolicyConfig, reinterpret_cast<void**>(&policy)))) {
      return false;
    }
    bool ok = true;
    for (ERole role : {eConsole, eMultimedia, eCommunications}) {
      ok = SUCCEEDED(policy->SetDefaultEndpoint(id.c_str(), role)) && ok;
    }
    policy->Release();
    return ok;
  }

  std::vector<AudioSession> listAudioSessions() {
    std::vector<AudioSession> out;
    forEachSession([&](IAudioSessionControl2* ctl2, ISimpleAudioVolume* vol, AudioSessionState state) {
      if (ctl2->IsSystemSoundsSession() == S_OK) {
        return false; // "System sounds" has no app to show
      }
      DWORD pid = 0;
      ctl2->GetProcessId(&pid);
      AudioSession s;
      s.id = sessionId(ctl2);
      s.exePath = processPath(pid);
      LPWSTR display = nullptr;
      if (SUCCEEDED(ctl2->GetDisplayName(&display)) && display != nullptr) {
        // Some apps set "@%SystemRoot%\..." resource strings: not human readable, ignore.
        if (display[0] != L'\0' && display[0] != L'@') {
          s.name = display;
        }
        CoTaskMemFree(display);
      }
      if (s.name.empty()) {
        s.name = fileStem(s.exePath);
      }
      float v = 1.0F;
      vol->GetMasterVolume(&v);
      BOOL m = FALSE;
      vol->GetMute(&m);
      s.volume = v;
      s.muted = m != FALSE;
      s.active = state == AudioSessionStateActive;
      if (!s.name.empty()) {
        out.push_back(std::move(s));
      }
      return false;
    });
    return out;
  }

  void setAudioSessionVolume(const std::wstring& id, float volume) {
    forEachSession([&](IAudioSessionControl2* ctl2, ISimpleAudioVolume* vol, AudioSessionState) {
      if (sessionId(ctl2) != id) {
        return false;
      }
      vol->SetMasterVolume(volume < 0 ? 0 : (volume > 1 ? 1 : volume), nullptr);
      return true;
    });
  }

  void setAudioSessionMuted(const std::wstring& id, bool muted) {
    forEachSession([&](IAudioSessionControl2* ctl2, ISimpleAudioVolume* vol, AudioSessionState) {
      if (sessionId(ctl2) != id) {
        return false;
      }
      vol->SetMute(muted ? TRUE : FALSE, nullptr);
      return true;
    });
  }

} // namespace platform
