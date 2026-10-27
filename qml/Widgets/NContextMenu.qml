import QtQuick
import QtQuick.Window
import Noctalia
import "../Commons"

// Simple floating menu: NContextMenu { model: [{icon, text, action}] } then open(item, below).
Window {
  id: root

  property var model: []
  property Item anchorItem: null
  property bool below: true

  flags: Qt.Popup | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.NoDropShadowWindowHint
  color: "transparent"
  width: 240
  height: column.implicitHeight + 2 * Style.spaceXs + 2
  visible: false

  function open(item, placeBelow) {
    anchorItem = item;
    below = placeBelow;
    const p = item.mapToGlobal(item.width, placeBelow ? item.height : 0);
    x = Math.round(p.x - width);
    y = Math.round(placeBelow ? p.y + Style.spaceSm + 6 : p.y - height - Style.spaceSm - 6);
    visible = true;
    requestActivate();
    leaveTimer.restart();
  }

  function close() {
    visible = false;
  }

  onActiveChanged: if (!active && visible) close()

  // Safety net in case Windows refused to give us focus (then activeChanged never fires).
  Timer {
    id: leaveTimer
    interval: 2500
    onTriggered: if (root.visible && !hoverArea.containsMouse) root.close()
  }

  Shortcut {
    sequence: "Escape"
    onActivated: root.close()
  }

  Rectangle {
    anchors.fill: parent
    radius: Style.radiusLg
    color: Style.glassColor(Theme.colors.surface, 1)
    border.color: Theme.colors.outlineVariant
    border.width: Style.borderWidth

    MouseArea {
      id: hoverArea
      anchors.fill: parent
      hoverEnabled: true
      acceptedButtons: Qt.NoButton
      onExited: leaveTimer.restart()
    }

    Column {
      id: column
      anchors.fill: parent
      anchors.margins: Style.spaceXs + 1

      Repeater {
        model: root.model

        delegate: Rectangle {
          id: entry
          required property var modelData
          width: column.width
          height: modelData.separator ? 9 : 34
          radius: Style.radiusMd
          color: !modelData.separator && itemMouse.containsMouse ? Theme.colors.hover : "transparent"

          Rectangle {
            visible: entry.modelData.separator === true
            anchors.centerIn: parent
            width: parent.width - 2 * Style.spaceSm
            height: 1
            color: Theme.colors.outlineVariant
          }

          Row {
            visible: !entry.modelData.separator
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: Style.spaceSm + 2
            spacing: Style.spaceSm + 2

            NGlyph {
              icon: entry.modelData.icon ?? ""
              anchors.verticalCenter: parent.verticalCenter
              color: itemMouse.containsMouse ? Theme.colors.onHover : Theme.colors.onSurface
            }
            NText {
              text: entry.modelData.text ?? ""
              anchors.verticalCenter: parent.verticalCenter
              font.weight: Font.Normal
              color: itemMouse.containsMouse ? Theme.colors.onHover : Theme.colors.onSurface
            }
          }

          MouseArea {
            id: itemMouse
            anchors.fill: parent
            enabled: !entry.modelData.separator
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
              root.close();
              if (entry.modelData.action)
                entry.modelData.action();
            }
          }
        }
      }
    }
  }
}
