// Windows implementation of the default-output volume control (Core Audio API).

#include "platform/Platform.h"

#include <windows.h>
#include <endpointvolume.h>
#include <initguid.h> // defines PKEY_Device_FriendlyName in this translation unit
#include <functiondiscoverykeys_devpkey.h>
#include <mmdeviceapi.h>
#include <propidl.h>

#include <atomic>
#include <utility>

namespace platform {

  namespace {

    template <typename T> void safeRelease(T*& p) {
      if (p != nullptr) {
        p->Release();
        p = nullptr;
      }
    }

    // Receives volume / mute changes of one endpoint (called on a COM worker thread).
    class VolumeCallback final : public IAudioEndpointVolumeCallback {
    public:
      explicit VolumeCallback(std::function<void(float, bool)> fn) : m_fn(std::move(fn)) {}

      ULONG STDMETHODCALLTYPE AddRef() override { return ++m_refs; }
      ULONG STDMETHODCALLTYPE Release() override {
        const ULONG r = --m_refs;
        if (r == 0) {
          delete this;
        }
        return r;
      }
      HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** out) override {
        if (riid == __uuidof(IUnknown) || riid == __uuidof(IAudioEndpointVolumeCallback)) {
          *out = static_cast<IAudioEndpointVolumeCallback*>(this);
          AddRef();
          return S_OK;
        }
        *out = nullptr;
        return E_NOINTERFACE;
      }
      HRESULT STDMETHODCALLTYPE OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA data) override {
        if (data != nullptr && m_fn) {
          m_fn(data->fMasterVolume, data->bMuted != FALSE);
        }
        return S_OK;
      }

    private:
      std::atomic<ULONG> m_refs{1};
      std::function<void(float, bool)> m_fn;
    };

    // Receives "default device changed" notifications (called on a COM worker thread).
    class DeviceNotifier final : public IMMNotificationClient {
    public:
      explicit DeviceNotifier(std::function<void()> fn) : m_fn(std::move(fn)) {}

      ULONG STDMETHODCALLTYPE AddRef() override { return ++m_refs; }
      ULONG STDMETHODCALLTYPE Release() override {
        const ULONG r = --m_refs;
        if (r == 0) {
          delete this;
        }
        return r;
      }
      HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** out) override {
        if (riid == __uuidof(IUnknown) || riid == __uuidof(IMMNotificationClient)) {
          *out = static_cast<IMMNotificationClient*>(this);
          AddRef();
          return S_OK;
        }
        *out = nullptr;
        return E_NOINTERFACE;
      }
      HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR) override {
        if (flow == eRender && role == eConsole && m_fn) {
          m_fn();
        }
        return S_OK;
      }
      HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR, DWORD) override { return S_OK; }
      HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR) override { return S_OK; }
      HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR) override { return S_OK; }
      HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY) override { return S_OK; }

    private:
      std::atomic<ULONG> m_refs{1};
      std::function<void()> m_fn;
    };

    class WinAudioEndpoint final : public AudioEndpoint {
    public:
      WinAudioEndpoint(std::function<void(float, bool)> onChange, std::function<void()> onDeviceChanged) {
        m_volumeCallback = new VolumeCallback(std::move(onChange));
        m_deviceNotifier = new DeviceNotifier(std::move(onDeviceChanged));
        if (SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                       IID_PPV_ARGS(&m_enumerator)))) {
          m_enumerator->RegisterEndpointNotificationCallback(m_deviceNotifier);
        }
        rebind();
      }

      ~WinAudioEndpoint() override {
        detach();
        if (m_enumerator != nullptr) {
          m_enumerator->UnregisterEndpointNotificationCallback(m_deviceNotifier);
        }
        safeRelease(m_enumerator);
        safeRelease(m_volumeCallback);
        safeRelease(m_deviceNotifier);
      }

      bool rebind() override {
        detach();
        if (m_enumerator == nullptr) {
          return false;
        }
        if (FAILED(m_enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &m_device))) {
          return false;
        }
        if (FAILED(m_device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr,
                                      reinterpret_cast<void**>(&m_volume)))) {
          safeRelease(m_device);
          return false;
        }
        m_volume->RegisterControlChangeNotify(m_volumeCallback);
        return true;
      }

      bool valid() const override { return m_volume != nullptr; }

      float volume() const override {
        float v = 0.0F;
        if (m_volume != nullptr) {
          m_volume->GetMasterVolumeLevelScalar(&v);
        }
        return v;
      }

      bool muted() const override {
        BOOL m = FALSE;
        if (m_volume != nullptr) {
          m_volume->GetMute(&m);
        }
        return m != FALSE;
      }

      void setVolume(float value) override {
        if (m_volume == nullptr) {
          return;
        }
        if (value < 0.0F) {
          value = 0.0F;
        }
        if (value > 1.0F) {
          value = 1.0F;
        }
        m_volume->SetMasterVolumeLevelScalar(value, nullptr);
      }

      void setMuted(bool muted) override {
        if (m_volume != nullptr) {
          m_volume->SetMute(muted ? TRUE : FALSE, nullptr);
        }
      }

      std::wstring deviceName() const override {
        std::wstring name;
        if (m_device == nullptr) {
          return name;
        }
        IPropertyStore* store = nullptr;
        if (SUCCEEDED(m_device->OpenPropertyStore(STGM_READ, &store))) {
          PROPVARIANT pv;
          PropVariantInit(&pv);
          if (SUCCEEDED(store->GetValue(PKEY_Device_FriendlyName, &pv)) && pv.vt == VT_LPWSTR) {
            name = pv.pwszVal;
          }
          PropVariantClear(&pv);
          store->Release();
        }
        return name;
      }

    private:
      void detach() {
        if (m_volume != nullptr) {
          m_volume->UnregisterControlChangeNotify(m_volumeCallback);
        }
        safeRelease(m_volume);
        safeRelease(m_device);
      }

      IMMDeviceEnumerator* m_enumerator = nullptr;
      IMMDevice* m_device = nullptr;
      IAudioEndpointVolume* m_volume = nullptr;
      VolumeCallback* m_volumeCallback = nullptr;
      DeviceNotifier* m_deviceNotifier = nullptr;
    };

  } // namespace

  std::unique_ptr<AudioEndpoint> AudioEndpoint::create(std::function<void(float, bool)> onChange,
                                                       std::function<void()> onDefaultDeviceChanged) {
    return std::make_unique<WinAudioEndpoint>(std::move(onChange), std::move(onDefaultDeviceChanged));
  }

} // namespace platform
