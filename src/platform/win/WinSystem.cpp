// Windows implementation: power plans and system statistics.

#include "platform/Platform.h"

#include <windows.h>
#include <objbase.h>
#include <powrprof.h>

namespace platform {

  namespace {

    std::wstring guidToString(const GUID& g) {
      wchar_t buf[64] = {};
      StringFromGUID2(g, buf, 64);
      return buf;
    }

    std::uint64_t fileTimeToU64(const FILETIME& ft) {
      return (static_cast<std::uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    }

  } // namespace

  std::vector<PowerPlan> listPowerPlans() {
    std::vector<PowerPlan> out;
    GUID* active = nullptr;
    GUID activeGuid{};
    if (PowerGetActiveScheme(nullptr, &active) == ERROR_SUCCESS && active != nullptr) {
      activeGuid = *active;
      LocalFree(active);
    }
    for (ULONG i = 0;; ++i) {
      GUID scheme{};
      DWORD size = sizeof(scheme);
      if (PowerEnumerate(nullptr, nullptr, nullptr, ACCESS_SCHEME, i, reinterpret_cast<UCHAR*>(&scheme), &size) !=
          ERROR_SUCCESS) {
        break;
      }
      DWORD nameSize = 0;
      PowerReadFriendlyName(nullptr, &scheme, nullptr, nullptr, nullptr, &nameSize);
      std::wstring name(nameSize / sizeof(wchar_t) + 1, L'\0');
      if (nameSize > 0 &&
          PowerReadFriendlyName(nullptr, &scheme, nullptr, nullptr, reinterpret_cast<UCHAR*>(name.data()), &nameSize) ==
              ERROR_SUCCESS) {
        name.resize(wcsnlen(name.c_str(), name.size()));
      } else {
        name.clear();
      }
      out.push_back({guidToString(scheme), name, IsEqualGUID(scheme, activeGuid) != FALSE});
    }
    return out;
  }

  bool setPowerPlan(const std::wstring& guid) {
    GUID g{};
    if (FAILED(CLSIDFromString(guid.c_str(), &g))) {
      return false;
    }
    return PowerSetActiveScheme(nullptr, &g) == ERROR_SUCCESS;
  }

  SystemStats querySystem() {
    SystemStats s;

    // CPU: busy share of the time since the previous call.
    static std::uint64_t prevIdle = 0, prevTotal = 0;
    FILETIME idle{}, kernel{}, user{};
    if (GetSystemTimes(&idle, &kernel, &user)) {
      const std::uint64_t i = fileTimeToU64(idle);
      const std::uint64_t total = fileTimeToU64(kernel) + fileTimeToU64(user); // kernel includes idle
      if (prevTotal != 0 && total > prevTotal) {
        const double dTotal = static_cast<double>(total - prevTotal);
        const double dIdle = static_cast<double>(i - prevIdle);
        s.cpuPercent = 100.0 * (1.0 - dIdle / dTotal);
      }
      prevIdle = i;
      prevTotal = total;
    }

    MEMORYSTATUSEX mem{};
    mem.dwLength = sizeof(mem);
    if (GlobalMemoryStatusEx(&mem)) {
      s.memTotal = mem.ullTotalPhys;
      s.memUsed = mem.ullTotalPhys - mem.ullAvailPhys;
    }

    s.uptimeSeconds = GetTickCount64() / 1000;

    wchar_t sysDir[MAX_PATH] = {};
    GetSystemWindowsDirectoryW(sysDir, MAX_PATH);
    wchar_t root[4] = {sysDir[0] ? sysDir[0] : L'C', L':', L'\\', 0};
    ULARGE_INTEGER freeBytes{}, totalBytes{};
    if (GetDiskFreeSpaceExW(root, &freeBytes, &totalBytes, nullptr)) {
      s.diskTotal = totalBytes.QuadPart;
      s.diskFree = freeBytes.QuadPart;
    }

    wchar_t name[MAX_COMPUTERNAME_LENGTH + 1] = {};
    DWORD len = MAX_COMPUTERNAME_LENGTH + 1;
    if (GetComputerNameW(name, &len)) {
      s.computerName.assign(name, len);
    }
    return s;
  }

} // namespace platform
