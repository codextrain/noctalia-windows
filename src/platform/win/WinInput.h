#pragma once

// Tiny helper to send keyboard shortcuts with SendInput.

#include <windows.h>

#include <initializer_list>
#include <vector>

namespace platform::win {

  inline bool isExtendedKey(WORD vk) {
    switch (vk) {
    case VK_LEFT:
    case VK_RIGHT:
    case VK_UP:
    case VK_DOWN:
    case VK_LWIN:
    case VK_RWIN:
    case VK_INSERT:
    case VK_DELETE:
    case VK_HOME:
    case VK_END:
    case VK_PRIOR:
    case VK_NEXT:
      return true;
    default:
      return false;
    }
  }

  inline INPUT keyInput(WORD vk, bool down) {
    INPUT in{};
    in.type = INPUT_KEYBOARD;
    in.ki.wVk = vk;
    in.ki.dwFlags = (down ? 0u : static_cast<DWORD>(KEYEVENTF_KEYUP)) | (isExtendedKey(vk) ? KEYEVENTF_EXTENDEDKEY : 0u);
    return in;
  }

  // Press modifiers + `repeat` taps of the last key, then release modifiers in reverse order.
  // sendChord({VK_LWIN, 'A'})                      -> Win+A
  // sendChord({VK_LCONTROL, VK_LWIN, VK_RIGHT}, 3) -> Ctrl+Win+Right three times
  inline void sendChord(std::initializer_list<WORD> keys, int repeat = 1) {
    std::vector<WORD> list(keys);
    if (list.empty()) {
      return;
    }
    const WORD last = list.back();
    list.pop_back();

    std::vector<INPUT> inputs;
    for (WORD vk : list) {
      inputs.push_back(keyInput(vk, true));
    }
    for (int i = 0; i < (repeat < 1 ? 1 : repeat); ++i) {
      inputs.push_back(keyInput(last, true));
      inputs.push_back(keyInput(last, false));
    }
    for (auto it = list.rbegin(); it != list.rend(); ++it) {
      inputs.push_back(keyInput(*it, false));
    }
    SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
  }

} // namespace platform::win
