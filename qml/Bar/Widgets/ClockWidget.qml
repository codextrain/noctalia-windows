import QtQuick
import Noctalia
import ".."
import "../../Commons"
import "../../Widgets"

// Noctalia: left click = control center > calendar. Right click = Windows notification center.
BarWidget {
  id: root

  property date now: new Date()
  readonly property string timeFormat: Config.shell.time_format ?? "HH:mm"
  readonly property string dateFormat: Config.shell.date_format ?? "dddd, d 'de' MMMM"

  tooltipText: {
    const d = now.toLocaleDateString(Qt.locale(), dateFormat);
    return d.charAt(0).toUpperCase() + d.slice(1);
  }
  onClicked: Panels.toggle("control-center:calendar")
  onRightClicked: Shell.notificationCenter()

  Timer {
    // Tick exactly on the next second boundary, then every second.
    interval: 1000 - (new Date()).getMilliseconds()
    running: true
    repeat: true
    onTriggered: {
      root.now = new Date();
      interval = 1000 - root.now.getMilliseconds();
    }
  }

  NText {
    text: Qt.formatDateTime(root.now, root.timeFormat)
    color: root.foreground
    font.pixelSize: Style.fontSizeBody
    font.weight: Font.DemiBold
  }
}
