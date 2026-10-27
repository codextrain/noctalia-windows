import QtQuick
import Noctalia
import ".."
import "../../Commons"
import "../../Widgets"

// Noctalia "sysmon": CPU and RAM usage; left = control center > system.
BarWidget {
  id: root
  tooltipText: "CPU " + Math.round(SystemMonitor.cpu) + "% · Memória "
               + (SystemMonitor.memTotal > 0 ? Math.round(100 * SystemMonitor.memUsed / SystemMonitor.memTotal) : 0) + "%"
  onClicked: Panels.toggle("control-center:system")

  Component.onCompleted: SystemMonitor.acquire()
  Component.onDestruction: SystemMonitor.release()

  NGlyph { icon: "cpu-usage"; color: root.foreground; anchors.verticalCenter: parent.verticalCenter }
  NText {
    anchors.verticalCenter: parent.verticalCenter
    text: Math.round(SystemMonitor.cpu) + "%"
    font.pixelSize: Style.fontSizeCaption
    color: root.foreground
  }
  NGlyph { icon: "memory"; color: root.foreground; anchors.verticalCenter: parent.verticalCenter }
  NText {
    anchors.verticalCenter: parent.verticalCenter
    text: (SystemMonitor.memTotal > 0 ? Math.round(100 * SystemMonitor.memUsed / SystemMonitor.memTotal) : 0) + "%"
    font.pixelSize: Style.fontSizeCaption
    color: root.foreground
  }
}
