import QtQuick
import Noctalia
import "../../Commons"
import "../../Widgets"

// [mute button] ───────slider─────── 45%
Item {
  id: root

  property string icon: "volume"
  property string mutedIcon: "volume-mute"
  property real value: 0
  property bool muted: false
  property bool available: true
  property string label: ""
  signal moved(real value)
  signal muteToggled

  implicitHeight: 36

  NIconButton {
    id: muteButton
    anchors.left: parent.left
    anchors.verticalCenter: parent.verticalCenter
    icon: root.muted || root.value <= 0 ? root.mutedIcon : root.icon
    active: !root.muted
    onClicked: root.muteToggled()
  }

  NSlider {
    anchors.left: muteButton.right
    anchors.leftMargin: Style.spaceMd
    anchors.right: pct.left
    anchors.rightMargin: Style.spaceMd
    anchors.verticalCenter: parent.verticalCenter
    value: root.value
    wheelStep: 0.01
    enabled: root.available
    accent: root.muted ? Theme.colors.onSurfaceVariant : Theme.colors.primary
    onMoved: v => root.moved(Math.round(v * 100) / 100)
  }

  NText {
    id: pct
    width: 44
    anchors.right: parent.right
    anchors.verticalCenter: parent.verticalCenter
    horizontalAlignment: Text.AlignRight
    text: root.muted ? "mudo" : Math.round(root.value * 100) + "%"
    font.pixelSize: Style.fontSizeCaption
    color: Theme.colors.onSurfaceVariant
  }
}
