import QtQuick
import Noctalia
import "../../Commons"
import "../../Widgets"

// CPU, memory, disk and uptime.
Column {
  id: tab
  spacing: Style.spaceMd

  Component.onCompleted: SystemMonitor.acquire()
  Component.onDestruction: SystemMonitor.release()

  function gb(bytes) {
    return (bytes / 1073741824).toLocaleString(Qt.locale(), "f", 1) + " GB";
  }

  component Stat: NCard {
    id: stat
    property string icon
    property string label
    property string valueText
    property real fraction
    width: tab.width
    height: 86

    NGlyph {
      id: statIcon
      icon: stat.icon
      size: 22
      color: Theme.colors.primary
      anchors.left: parent.left
      anchors.top: parent.top
    }
    NText {
      anchors.left: statIcon.right
      anchors.leftMargin: Style.spaceSm
      anchors.verticalCenter: statIcon.verticalCenter
      text: stat.label
      font.weight: Font.Bold
    }
    NText {
      anchors.right: parent.right
      anchors.verticalCenter: statIcon.verticalCenter
      text: stat.valueText
      font.weight: Font.Normal
      color: Theme.colors.onSurfaceVariant
    }
    NProgress {
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.bottom: parent.bottom
      value: stat.fraction
      accent: stat.fraction > 0.9 ? Theme.colors.error : Theme.colors.primary
    }
  }

  // CPU with a small history graph
  NCard {
    width: tab.width
    height: 140

    NGlyph {
      id: cpuIcon
      icon: "cpu-usage"
      size: 22
      color: Theme.colors.primary
      anchors.left: parent.left
      anchors.top: parent.top
    }
    NText {
      anchors.left: cpuIcon.right
      anchors.leftMargin: Style.spaceSm
      anchors.verticalCenter: cpuIcon.verticalCenter
      text: "Processador"
      font.weight: Font.Bold
    }
    NText {
      anchors.right: parent.right
      anchors.verticalCenter: cpuIcon.verticalCenter
      text: Math.round(SystemMonitor.cpu) + "%"
      font.weight: Font.Normal
      color: Theme.colors.onSurfaceVariant
    }
    Row {
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.bottom: parent.bottom
      height: 70
      spacing: 2
      Repeater {
        model: SystemMonitor.cpuHistory
        delegate: Rectangle {
          required property var modelData
          width: (tab.width - 2 * Style.panelPadding - 39 * 2) / 40
          height: Math.max(2, 70 * modelData / 100)
          anchors.bottom: parent.bottom
          radius: 2
          color: Style.alpha(Theme.colors.primary, 0.35 + 0.65 * modelData / 100)
        }
      }
    }
  }

  Stat {
    icon: "memory"
    label: "Memória"
    valueText: tab.gb(SystemMonitor.memUsed) + " de " + tab.gb(SystemMonitor.memTotal)
    fraction: SystemMonitor.memTotal > 0 ? SystemMonitor.memUsed / SystemMonitor.memTotal : 0
  }
  Stat {
    icon: "storage"
    label: "Disco do sistema"
    valueText: tab.gb(SystemMonitor.diskUsed) + " de " + tab.gb(SystemMonitor.diskTotal)
    fraction: SystemMonitor.diskTotal > 0 ? SystemMonitor.diskUsed / SystemMonitor.diskTotal : 0
  }
  NCard {
    width: tab.width
    height: 56
    NGlyph {
      id: upIcon
      icon: "clock"
      size: 22
      color: Theme.colors.primary
      anchors.left: parent.left
      anchors.verticalCenter: parent.verticalCenter
    }
    NText {
      anchors.left: upIcon.right
      anchors.leftMargin: Style.spaceSm
      anchors.verticalCenter: parent.verticalCenter
      text: "Ligado há"
      font.weight: Font.Bold
    }
    NText {
      anchors.right: parent.right
      anchors.verticalCenter: parent.verticalCenter
      color: Theme.colors.onSurfaceVariant
      font.weight: Font.Normal
      text: {
        const s = SystemMonitor.uptime;
        const d = Math.floor(s / 86400), h = Math.floor((s % 86400) / 3600), m = Math.floor((s % 3600) / 60);
        return (d > 0 ? d + " dias, " : "") + h + "h " + m + "min";
      }
    }
  }
}
