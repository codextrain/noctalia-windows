// Windows implementation: Wi-Fi / Bluetooth radio on-off and paired Bluetooth devices.
// Uses C++/WinRT (Windows.Devices.Radios / Windows.Devices.Bluetooth), MSVC only.
// Every function blocks: call from a worker thread (multithreaded apartment).

#include "platform/Platform.h"

#include <windows.h>

#include <winrt/Windows.Devices.Bluetooth.h>
#include <winrt/Windows.Devices.Enumeration.h>
#include <winrt/Windows.Devices.Radios.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>

namespace platform {

  namespace {

    namespace wdr = winrt::Windows::Devices::Radios;
    namespace wdb = winrt::Windows::Devices::Bluetooth;
    namespace wde = winrt::Windows::Devices::Enumeration;

    void ensureApartment() {
      thread_local bool done = false;
      if (!done) {
        try {
          winrt::init_apartment(winrt::apartment_type::multi_threaded);
        } catch (...) {
          // already initialised (maybe as STA by someone else): fine
        }
        done = true;
      }
    }

    wdr::RadioKind toKind(RadioKind k) { return k == RadioKind::WiFi ? wdr::RadioKind::WiFi : wdr::RadioKind::Bluetooth; }

    wdr::Radio findRadio(RadioKind kind) {
      for (const auto& r : wdr::Radio::GetRadiosAsync().get()) {
        if (r.Kind() == toKind(kind)) {
          return r;
        }
      }
      return nullptr;
    }

    std::wstring kindFromClass(const wdb::BluetoothClassOfDevice& cod) {
      using Major = wdb::BluetoothMajorClass;
      switch (cod.MajorClass()) {
      case Major::AudioVideo: {
        const auto minor = cod.MinorClass();
        if (minor == wdb::BluetoothMinorClass::AudioVideoWearableHeadset ||
            minor == wdb::BluetoothMinorClass::AudioVideoHandsFree) {
          return L"headset";
        }
        return L"audio";
      }
      case Major::Peripheral: {
        const auto raw = static_cast<int>(cod.RawValue());
        // bits 6-7 of the minor class: 01 = keyboard, 10 = pointing device
        const int sub = (raw >> 6) & 0x3;
        if (sub == 1) return L"keyboard";
        if (sub == 2) return L"mouse";
        return L"gamepad";
      }
      case Major::Phone:
        return L"phone";
      case Major::Computer:
        return L"computer";
      case Major::Wearable:
        return L"watch";
      default:
        return L"";
      }
    }

  } // namespace

  RadioState queryRadio(RadioKind kind) {
    ensureApartment();
    RadioState st;
    try {
      if (const auto r = findRadio(kind)) {
        st.present = true;
        st.on = r.State() == wdr::RadioState::On;
      }
    } catch (...) {
    }
    return st;
  }

  bool setRadio(RadioKind kind, bool on) {
    ensureApartment();
    try {
      if (wdr::Radio::RequestAccessAsync().get() != wdr::RadioAccessStatus::Allowed) {
        return false;
      }
      if (const auto r = findRadio(kind)) {
        return r.SetStateAsync(on ? wdr::RadioState::On : wdr::RadioState::Off).get() == wdr::RadioAccessStatus::Allowed;
      }
    } catch (...) {
    }
    return false;
  }

  std::vector<BluetoothDevice> listBluetoothDevices() {
    ensureApartment();
    std::vector<BluetoothDevice> out;
    try {
      const auto selector = wdb::BluetoothDevice::GetDeviceSelectorFromPairingState(true);
      for (const auto& info : wde::DeviceInformation::FindAllAsync(selector).get()) {
        BluetoothDevice d;
        d.id = info.Id().c_str();
        d.name = info.Name().c_str();
        try {
          if (const auto dev = wdb::BluetoothDevice::FromIdAsync(info.Id()).get()) {
            d.connected = dev.ConnectionStatus() == wdb::BluetoothConnectionStatus::Connected;
            d.kind = kindFromClass(dev.ClassOfDevice());
          }
        } catch (...) {
        }
        out.push_back(std::move(d));
      }
      // Bluetooth Low Energy devices (many earbuds, mice, controllers) are a separate list.
      const auto leSelector = wdb::BluetoothLEDevice::GetDeviceSelectorFromPairingState(true);
      for (const auto& info : wde::DeviceInformation::FindAllAsync(leSelector).get()) {
        const std::wstring name = info.Name().c_str();
        bool duplicate = false;
        for (const auto& existing : out) {
          duplicate = duplicate || existing.name == name;
        }
        if (duplicate || name.empty()) {
          continue;
        }
        BluetoothDevice d;
        d.id = info.Id().c_str();
        d.name = name;
        try {
          if (const auto dev = wdb::BluetoothLEDevice::FromIdAsync(info.Id()).get()) {
            d.connected = dev.ConnectionStatus() == wdb::BluetoothConnectionStatus::Connected;
          }
        } catch (...) {
        }
        out.push_back(std::move(d));
      }
    } catch (...) {
    }
    return out;
  }

} // namespace platform
