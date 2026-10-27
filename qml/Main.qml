import QtQuick
import QtQml.Models
import Noctalia
import "Bar"
import "Panels/Launcher"
import "Panels/ControlCenter"
import "Osd"
import "Panels/Wallpaper"
import "Panels/Clipboard"

// Entry point: one bar per connected monitor (follows hot-plugging automatically) and the
// panels, which open on the monitor under the mouse.
QtObject {
  id: root

  property Instantiator bars: Instantiator {
    model: Qt.application.screens
    delegate: BarWindow {}
  }

  property LauncherPanel launcher: LauncherPanel {}
  property ControlCenterPanel controlCenter: ControlCenterPanel {}
  property OsdWindow osd: OsdWindow {}
  property WallpaperPanel wallpaper: WallpaperPanel {}
  property ClipboardPanel clipboard: ClipboardPanel {}

  property Connections configErrors: Connections {
    target: Config
    function onChanged() {
      if (Config.lastError.length > 0)
        console.warn("noctalia-win:", Config.lastError);
    }
  }

  property Connections hotkeyErrors: Connections {
    target: Panels
    function onHotkeysChanged() {
      for (const e of Panels.hotkeyErrors)
        console.warn("noctalia-win: atalho não registrado:", e);
    }
  }
}
