import QtQuick
import QtQuick.Window
import Noctalia
import "../Commons"

// Small floating tooltip window shared by all widgets of a bar.
// show(item, text, below) places it centered under (or above) `item`.
Window {
  id: root

  property string text: ""
  property Item target: null
  property bool below: true

  flags: Qt.ToolTip | Qt.FramelessWindowHint | Qt.WindowTransparentForInput | Qt.WindowDoesNotAcceptFocus
         | Qt.WindowStaysOnTopHint | Qt.NoDropShadowWindowHint
  color: "transparent"
  width: Math.ceil(label.implicitWidth + 2 * Style.spaceMd)
  height: Math.ceil(label.implicitHeight + 2 * Style.spaceSm)
  visible: false

  function show(item, txt, placeBelow) {
    if (!item || !txt) {
      hide();
      return;
    }
    target = item;
    text = txt;
    below = placeBelow;
    delay.restart();
  }

  function hide() {
    delay.stop();
    target = null;
    visible = false;
  }

  function place() {
    if (!target)
      return;
    const p = target.mapToGlobal(target.width / 2, below ? target.height : 0);
    x = Math.round(p.x - width / 2);
    y = Math.round(below ? p.y + Style.spaceSm + 6 : p.y - height - Style.spaceSm - 6);
  }

  Timer {
    id: delay
    interval: 450
    onTriggered: {
      root.place();
      root.visible = true;
    }
  }

  Rectangle {
    anchors.fill: parent
    radius: Style.radiusMd
    color: Style.glassColor(Theme.colors.surfaceContainer, 1)
    border.color: Theme.colors.outlineVariant
    border.width: Style.borderWidth

    NText {
      id: label
      anchors.centerIn: parent
      text: root.text
      font.pixelSize: Style.fontSizeCaption
      font.weight: Font.Normal
    }
  }
}
