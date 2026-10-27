import QtQuick
import Noctalia
import "../Commons"

// Segmented control (Noctalia ui::segmented, compact): a row of pill tabs.
//   NSegmented { model: [{key: "recent", text: "Recente", icon: "clock"}, ...]; current: "recent" }
Rectangle {
  id: root

  property var model: []
  property string current: ""
  signal selected(string key)

  implicitHeight: 32
  implicitWidth: row.implicitWidth + 6
  radius: Style.radiusMd + 3
  color: Theme.colors.surfaceContainer
  border.width: Style.borderWidth
  border.color: Theme.colors.outlineVariant

  Row {
    id: row
    anchors.centerIn: parent
    spacing: 2

    Repeater {
      model: root.model

      delegate: Rectangle {
        id: seg
        required property var modelData
        readonly property bool isCurrent: modelData.key === root.current
        readonly property bool hovered: segMouse.containsMouse

        height: root.height - 6
        width: segRow.implicitWidth + 2 * Style.spaceMd
        radius: Style.radiusMd
        color: isCurrent ? Theme.colors.primary : hovered ? Theme.colors.hover : "transparent"

        Behavior on color {
          ColorAnimation { duration: Style.animFast; easing.type: Easing.OutCubic }
        }

        Row {
          id: segRow
          anchors.centerIn: parent
          spacing: Style.spaceXs + 2

          NGlyph {
            visible: (seg.modelData.icon ?? "").length > 0
            icon: seg.modelData.icon ?? ""
            size: Style.fontSizeBody
            anchors.verticalCenter: parent.verticalCenter
            color: seg.isCurrent ? Theme.colors.onPrimary : seg.hovered ? Theme.colors.onHover : Theme.colors.onSurface
          }
          NText {
            text: seg.modelData.text ?? ""
            visible: text.length > 0
            anchors.verticalCenter: parent.verticalCenter
            font.pixelSize: Style.fontSizeCaption
            font.weight: Font.DemiBold
            color: seg.isCurrent ? Theme.colors.onPrimary : seg.hovered ? Theme.colors.onHover : Theme.colors.onSurface
          }
        }

        MouseArea {
          id: segMouse
          anchors.fill: parent
          hoverEnabled: true
          cursorShape: Qt.PointingHandCursor
          onClicked: root.selected(seg.modelData.key)
        }
      }
    }
  }
}
