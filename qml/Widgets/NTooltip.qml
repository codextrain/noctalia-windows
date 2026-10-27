import QtQuick
import QtQuick.Window
import Noctalia
import "../Commons"
import "../Commons/TooltipPosition.js" as TooltipPosition

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
  width: Math.min(Math.ceil(label.implicitWidth + 2 * Style.spaceMd), Math.max(1, screen.width - 16))
  height: Math.ceil(label.implicitHeight + 2 * Style.spaceSm)
  visible: false

  function show(item, txt, placeBelow) {
    if (!item || !txt) {
      hide();
      return;
    }
    target = item;
    if (item.Window.window && item.Window.window.screen)
      screen = item.Window.window.screen;
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
    const pos = TooltipPosition.position(p.x, p.y, width, height, below,
        Style.spaceSm + 6, screen.virtualX, screen.virtualY, screen.width, screen.height, 8);
    x = pos.x;
    y = pos.y;
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
      width: root.width - 2 * Style.spaceMd
      wrapMode: Text.Wrap
      horizontalAlignment: Text.AlignHCenter
      font.pixelSize: Style.fontSizeCaption
      font.weight: Font.Normal
    }
  }
}
