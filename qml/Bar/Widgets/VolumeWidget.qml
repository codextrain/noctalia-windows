import QtQuick
import Noctalia
import ".."
import "../../Widgets"

// Noctalia: left = control center > audio, right = mute, scroll = volume ±1%.
BarWidget {
  id: root

  readonly property int percent: Math.round(Audio.volume * 100)
  // Same rule as noctalia/src/pipewire/audio_glyphs.cpp
  readonly property string glyph: (Audio.muted || percent === 0) ? "volume-mute"
                                : Audio.volume < 0.4 ? "volume-low" : "volume-high"

  shown: Audio.available
  tooltipText: Audio.muted ? "Volume: mudo" : "Volume: " + percent + "%"

  onClicked: Panels.toggle("control-center:audio")
  onRightClicked: Audio.toggleMute()
  onWheelUp: Audio.changeVolume(0.01)
  onWheelDown: Audio.changeVolume(-0.01)

  SculptedIcon { symbol: "volume"; disabled: Audio.muted || root.percent === 0 }
}
