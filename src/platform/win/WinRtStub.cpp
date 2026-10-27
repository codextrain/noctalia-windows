// Fallback for Windows builds without C++/WinRT (e.g. MinGW): media, radios and Bluetooth
// report "not available". The MSVC build uses WinMedia.cpp / WinRadios.cpp instead.

#include "platform/Platform.h"

namespace platform {

  namespace {
    class NoMedia final : public MediaController {
    public:
      explicit NoMedia(std::function<void(const MediaState&)> fn) {
        if (fn) {
          fn(MediaState{});
        }
      }
      void playPause() override {}
      void next() override {}
      void previous() override {}
      void seek(double) override {}
    };
  } // namespace

  std::unique_ptr<MediaController> MediaController::create(std::function<void(const MediaState&)> onChange) {
    return std::make_unique<NoMedia>(std::move(onChange));
  }

  RadioState queryRadio(RadioKind) { return {}; }
  bool setRadio(RadioKind, bool) { return false; }
  std::vector<BluetoothDevice> listBluetoothDevices() { return {}; }

} // namespace platform
