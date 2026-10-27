import QtQuick
import Noctalia
import "../Commons"
import "../Widgets"

// One dock icon with magnification, running dot and window count.
Item {
  id: item

  property var dockWindow
  property var entry: ({})
  property bool isLauncher: false

  readonly property int base: dockWindow.iconSize
  property int slot: 0
  readonly property real centerX: dockWindow.slotCenter(slot)
  readonly property real distance: Math.abs(dockWindow.mouseX - centerX)
  readonly property real zoom: dockWindow.hovering ? 1 + (dockWindow.magScale - 1) * Math.max(0, 1 - distance / (base * 2.2)) : 1
  readonly property int count: (entry.windows ?? []).length

  width: Math.round(base * zoom)
  height: base
  opacity: entry.focused || isLauncher || mouse.containsMouse ? 1 : (dockWindow.cfg.inactive_opacity ?? 0.85)

  Behavior on width {
    NumberAnimation { duration: 60 }
  }

  Item {
    id: iconBox
    width: item.width
    height: item.width
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.bottom: dockWindow.atTop ? undefined : parent.bottom
    anchors.top: dockWindow.atTop ? parent.top : undefined

    Rectangle {
      anchors.fill: parent
      visible: item.isLauncher
      radius: width * 0.28
      color: mouse.containsMouse ? Theme.colors.hover : Theme.colors.surfaceContainerHigh
      NGlyph {
        anchors.centerIn: parent
        icon: "grid-dots"
        size: parent.width * 0.5
        color: mouse.containsMouse ? Theme.colors.onHover : Theme.colors.primary
      }
    }

    Image {
      id: img
      anchors.fill: parent
      visible: !item.isLauncher && status === Image.Ready
      source: item.isLauncher ? "" : (item.entry.icon ?? "")
      sourceSize: Qt.size(item.base * 2, item.base * 2)
      asynchronous: true
      mipmap: true
      fillMode: Image.PreserveAspectFit
    }
    NGlyph {
      anchors.centerIn: parent
      visible: !item.isLauncher && img.status !== Image.Ready
      icon: "app-window"
      size: parent.width * 0.7
      color: Theme.colors.onSurface
    }

    // window count badge
    Rectangle {
      visible: item.count > 1 && (dockWindow.cfg.show_instance_count ?? true)
      width: 16
      height: 16
      radius: 8
      anchors.right: parent.right
      anchors.top: parent.top
      color: Theme.colors.primary
      NText {
        anchors.centerIn: parent
        text: item.count
        font.pixelSize: 10
        font.weight: Font.Bold
        color: Theme.colors.onPrimary
      }
    }
  }

  // running indicator
  Rectangle {
    visible: item.entry.running === true
    width: item.entry.focused ? 14 : 5
    height: 4
    radius: 2
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.top: dockWindow.atTop ? undefined : parent.bottom
    anchors.topMargin: 3
    anchors.bottom: dockWindow.atTop ? parent.top : undefined
    anchors.bottomMargin: 3
    color: item.entry.focused ? Theme.colors.primary : Theme.colors.onSurfaceVariant
    Behavior on width {
      NumberAnimation { duration: Style.animFast }
    }
  }

  MouseArea {
    id: mouse
    anchors.fill: parent
    hoverEnabled: true
    acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
    cursorShape: Qt.PointingHandCursor
    onContainsMouseChanged: containsMouse ? dockWindow.showTip(item, item.entry.name) : dockWindow.hideTip()
    onClicked: event => {
      dockWindow.hideTip();
      if (item.isLauncher) {
        Panels.toggle("launcher");
      } else if (event.button === Qt.RightButton) {
        dockWindow.openMenu(item, item.entry);
      } else if (event.button === Qt.MiddleButton) {
        Dock.launchNew(item.entry.key);
      } else {
        Dock.activate(item.entry.key);
      }
    }
    onWheel: event => {
      if (!item.isLauncher && item.count > 1)
        Dock.activate(item.entry.key);
    }
  }
}
