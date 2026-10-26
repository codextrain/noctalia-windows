import QtQuick
import QtQml.Models
import Noctalia
import "Bar"

// Entry point: one bar per connected monitor (follows hot-plugging automatically).
QtObject {
  id: root

  property Instantiator bars: Instantiator {
    model: Qt.application.screens
    delegate: BarWindow {}
  }

  property Connections configErrors: Connections {
    target: Config
    function onChanged() {
      if (Config.lastError.length > 0)
        console.warn("noctalia-win:", Config.lastError);
    }
  }
}
