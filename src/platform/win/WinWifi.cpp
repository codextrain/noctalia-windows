// Windows implementation: Wi-Fi networks through the Native Wifi API (wlanapi).

#include "platform/Platform.h"

#include <windows.h>
#include <wlanapi.h>

#include <algorithm>
#include <map>
#include <string>

namespace platform {

  namespace {

    // One client handle for the whole process (opening one is slow).
    HANDLE wlanClient() {
      static HANDLE client = [] {
        HANDLE h = nullptr;
        DWORD negotiated = 0;
        if (WlanOpenHandle(2, nullptr, &negotiated, &h) != ERROR_SUCCESS) {
          return static_cast<HANDLE>(nullptr);
        }
        return h;
      }();
      return client;
    }

    // First wireless adapter (that's what the taskbar uses too).
    bool firstInterface(GUID& out) {
      HANDLE client = wlanClient();
      if (client == nullptr) {
        return false;
      }
      PWLAN_INTERFACE_INFO_LIST list = nullptr;
      if (WlanEnumInterfaces(client, nullptr, &list) != ERROR_SUCCESS || list == nullptr) {
        return false;
      }
      const bool ok = list->dwNumberOfItems > 0;
      if (ok) {
        out = list->InterfaceInfo[0].InterfaceGuid;
      }
      WlanFreeMemory(list);
      return ok;
    }

    std::wstring ssidToString(const DOT11_SSID& ssid) {
      // SSIDs are raw bytes, almost always UTF-8.
      const int len = static_cast<int>(ssid.uSSIDLength);
      if (len <= 0) {
        return {};
      }
      const int wlen = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(ssid.ucSSID), len, nullptr, 0);
      std::wstring out(static_cast<size_t>(wlen), L'\0');
      MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(ssid.ucSSID), len, out.data(), wlen);
      return out;
    }

    std::wstring xmlEscape(const std::wstring& s) {
      std::wstring out;
      for (const wchar_t c : s) {
        switch (c) {
        case L'&': out += L"&amp;"; break;
        case L'<': out += L"&lt;"; break;
        case L'>': out += L"&gt;"; break;
        case L'"': out += L"&quot;"; break;
        case L'\'': out += L"&apos;"; break;
        default: out += c;
        }
      }
      return out;
    }

    struct Found {
      WLAN_AVAILABLE_NETWORK net{};
      bool ok = false;
    };

    Found findNetwork(const GUID& iface, const std::wstring& ssid) {
      Found f;
      PWLAN_AVAILABLE_NETWORK_LIST list = nullptr;
      if (WlanGetAvailableNetworkList(wlanClient(), &iface, 0, nullptr, &list) != ERROR_SUCCESS || list == nullptr) {
        return f;
      }
      int best = -1;
      for (DWORD i = 0; i < list->dwNumberOfItems; ++i) {
        const WLAN_AVAILABLE_NETWORK& n = list->Network[i];
        if (ssidToString(n.dot11Ssid) == ssid && static_cast<int>(n.wlanSignalQuality) > best) {
          best = static_cast<int>(n.wlanSignalQuality);
          f.net = n;
          f.ok = true;
        }
      }
      WlanFreeMemory(list);
      return f;
    }

  } // namespace

  WifiState queryWifi() {
    WifiState st;
    GUID iface{};
    if (!firstInterface(iface)) {
      return st;
    }
    st.available = true;

    PWLAN_AVAILABLE_NETWORK_LIST list = nullptr;
    if (WlanGetAvailableNetworkList(wlanClient(), &iface, 0, nullptr, &list) == ERROR_SUCCESS && list != nullptr) {
      std::map<std::wstring, WifiNetwork> bySsid; // the same SSID shows up once per band / profile
      for (DWORD i = 0; i < list->dwNumberOfItems; ++i) {
        const WLAN_AVAILABLE_NETWORK& n = list->Network[i];
        WifiNetwork w;
        w.ssid = ssidToString(n.dot11Ssid);
        if (w.ssid.empty()) {
          continue; // hidden network
        }
        w.signal = static_cast<int>(n.wlanSignalQuality);
        w.secured = n.bSecurityEnabled != FALSE;
        w.connected = (n.dwFlags & WLAN_AVAILABLE_NETWORK_CONNECTED) != 0;
        w.saved = (n.dwFlags & WLAN_AVAILABLE_NETWORK_HAS_PROFILE) != 0;
        auto it = bySsid.find(w.ssid);
        if (it == bySsid.end()) {
          bySsid.emplace(w.ssid, w);
        } else {
          it->second.signal = std::max(it->second.signal, w.signal);
          it->second.connected = it->second.connected || w.connected;
          it->second.saved = it->second.saved || w.saved;
        }
      }
      WlanFreeMemory(list);
      for (auto& [ssid, w] : bySsid) {
        if (w.connected) {
          st.connected = true;
          st.ssid = w.ssid;
          st.signal = w.signal;
        }
        st.networks.push_back(w);
      }
      std::sort(st.networks.begin(), st.networks.end(), [](const WifiNetwork& a, const WifiNetwork& b) {
        if (a.connected != b.connected) return a.connected;
        if (a.saved != b.saved) return a.saved;
        return a.signal > b.signal;
      });
    }
    return st;
  }

  void scanWifi() {
    GUID iface{};
    if (firstInterface(iface)) {
      WlanScan(wlanClient(), &iface, nullptr, nullptr, nullptr);
    }
  }

  bool connectWifi(const std::wstring& ssid, const std::wstring& password) {
    GUID iface{};
    if (!firstInterface(iface)) {
      return false;
    }
    const Found found = findNetwork(iface, ssid);
    if (!found.ok) {
      return false;
    }
    const WLAN_AVAILABLE_NETWORK& n = found.net;

    std::wstring profileName;
    if ((n.dwFlags & WLAN_AVAILABLE_NETWORK_HAS_PROFILE) != 0 && n.strProfileName[0] != L'\0') {
      profileName = n.strProfileName;
    } else {
      // New network: write a Windows Wi-Fi profile, exactly like the taskbar does when you type a password.
      std::wstring auth = L"open";
      std::wstring enc = L"none";
      switch (n.dot11DefaultAuthAlgorithm) {
      case DOT11_AUTH_ALGO_RSNA_PSK:
        auth = L"WPA2PSK";
        enc = L"AES";
        break;
      case DOT11_AUTH_ALGO_WPA_PSK:
        auth = L"WPAPSK";
        enc = n.dot11DefaultCipherAlgorithm == DOT11_CIPHER_ALGO_TKIP ? L"TKIP" : L"AES";
        break;
      case static_cast<DOT11_AUTH_ALGORITHM>(9): // DOT11_AUTH_ALGO_WPA3_SAE (newer SDKs)
        auth = L"WPA3SAE";
        enc = L"AES";
        break;
      case DOT11_AUTH_ALGO_80211_OPEN:
        if (n.bSecurityEnabled) {
          auth = L"open";
          enc = L"WEP";
        }
        break;
      default:
        if (n.bSecurityEnabled) {
          auth = L"WPA2PSK";
          enc = L"AES";
        }
        break;
      }
      if (n.bSecurityEnabled && password.empty()) {
        return false;
      }
      std::wstring xml = L"<?xml version=\"1.0\"?><WLANProfile xmlns=\"http://www.microsoft.com/networking/WLAN/profile/v1\">";
      xml += L"<name>" + xmlEscape(ssid) + L"</name><SSIDConfig><SSID><name>" + xmlEscape(ssid) + L"</name></SSID></SSIDConfig>";
      xml += L"<connectionType>ESS</connectionType><connectionMode>auto</connectionMode><MSM><security>";
      xml += L"<authEncryption><authentication>" + auth + L"</authentication><encryption>" + enc +
             L"</encryption><useOneX>false</useOneX></authEncryption>";
      if (n.bSecurityEnabled) {
        xml += L"<sharedKey><keyType>" + std::wstring(enc == L"WEP" ? L"networkKey" : L"passPhrase") +
               L"</keyType><protected>false</protected><keyMaterial>" + xmlEscape(password) + L"</keyMaterial></sharedKey>";
      }
      xml += L"</security></MSM></WLANProfile>";
      DWORD reason = 0;
      if (WlanSetProfile(wlanClient(), &iface, 0, xml.c_str(), nullptr, TRUE, nullptr, &reason) != ERROR_SUCCESS) {
        return false;
      }
      profileName = ssid;
    }

    WLAN_CONNECTION_PARAMETERS params{};
    params.wlanConnectionMode = wlan_connection_mode_profile;
    params.strProfile = profileName.c_str();
    params.pDot11Ssid = nullptr;
    params.pDesiredBssidList = nullptr;
    params.dot11BssType = dot11_BSS_type_infrastructure;
    params.dwFlags = 0;
    return WlanConnect(wlanClient(), &iface, &params, nullptr) == ERROR_SUCCESS;
  }

  bool disconnectWifi() {
    GUID iface{};
    if (!firstInterface(iface)) {
      return false;
    }
    return WlanDisconnect(wlanClient(), &iface, nullptr) == ERROR_SUCCESS;
  }

} // namespace platform
