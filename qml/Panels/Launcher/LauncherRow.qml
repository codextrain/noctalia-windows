import QtQuick
import Noctalia
import "../../Commons"
import "../../Widgets"

// One launcher result in list mode (Noctalia LauncherResultRow):
// [icon 40] Title (body, semibold)            [pin]
//           subtitle (caption, on_surface_variant)
// selected = primary / on_primary, hovered = hover / on_hover.
Rectangle {
  id: row

  property var item: ({})
  property bool selected: false
  property int iconSize: 40
  property bool compact: false
  property bool showIcon: true
  readonly property bool hovered: mouse.containsMouse
  readonly property color fg: selected ? Theme.colors.onPrimary : hovered ? Theme.colors.onHover : Theme.colors.onSurface
  readonly property color fgDim: selected ? Style.alpha(Theme.colors.onPrimary, 0.8)
                               : hovered ? Style.alpha(Theme.colors.onHover, 0.8) : Theme.colors.onSurfaceVariant

  signal activated
  signal menuRequested(point windowPos)

  height: iconSize + 2 * (compact ? 2 : Style.spaceXs) + 4
  radius: Style.radiusMd
  color: selected ? Theme.colors.primary : hovered ? Theme.colors.hover : "transparent"

  Behavior on color {
    ColorAnimation { duration: Style.animFast; easing.type: Easing.OutCubic }
  }

  Item {
    id: iconBox
    width: row.showIcon || (row.item.badge ?? "").length > 0 ? row.iconSize : 0
    height: row.iconSize
    anchors.left: parent.left
    anchors.leftMargin: Style.spaceSm
    anchors.verticalCenter: parent.verticalCenter
    visible: width > 0

    // Emoji / "=" badge
    Text {
      anchors.centerIn: parent
      visible: (row.item.badge ?? "").length > 0
      text: row.item.badge ?? ""
      font.pixelSize: row.iconSize * 0.72
      font.family: (row.item.badge ?? "") === "=" ? Style.fontFamily : "Segoe UI Emoji"
      font.weight: Font.DemiBold
      color: row.fg
    }

    Image {
      id: img
      anchors.fill: parent
      visible: (row.item.badge ?? "").length === 0 && status === Image.Ready
      source: row.showIcon && (row.item.icon ?? "").length > 0 ? row.item.icon : ""
      sourceSize: Qt.size(row.iconSize * 2, row.iconSize * 2)
      asynchronous: true
      smooth: true
      mipmap: true
      fillMode: Image.PreserveAspectFit
    }

    NGlyph {
      anchors.centerIn: parent
      visible: (row.item.badge ?? "").length === 0 && img.status !== Image.Ready
      icon: row.item.glyph ?? "apps"
      size: row.iconSize * 0.7
      color: row.fg
    }
  }

  Column {
    anchors.left: iconBox.right
    anchors.leftMargin: row.compact ? Style.spaceSm : Style.spaceMd
    anchors.right: pinGlyph.visible ? pinGlyph.left : parent.right
    anchors.rightMargin: Style.spaceSm
    anchors.verticalCenter: parent.verticalCenter
    spacing: 0

    NText {
      width: parent.width
      text: row.item.title ?? ""
      font.pixelSize: Style.fontSizeBody
      font.weight: Font.DemiBold
      color: row.fg
    }
    NText {
      width: parent.width
      visible: !row.compact && text.length > 0
      text: row.item.subtitle ?? ""
      font.pixelSize: Style.fontSizeCaption
      font.weight: Font.Normal
      color: row.fgDim
    }
  }

  NGlyph {
    id: pinGlyph
    visible: row.item.pinned === true
    icon: "pinned"
    size: Style.fontSizeBody
    color: row.fgDim
    anchors.right: parent.right
    anchors.rightMargin: Style.spaceMd
    anchors.verticalCenter: parent.verticalCenter
  }

  MouseArea {
    id: mouse
    anchors.fill: parent
    hoverEnabled: true
    acceptedButtons: Qt.LeftButton | Qt.RightButton
    cursorShape: Qt.PointingHandCursor
    onClicked: event => {
      if (event.button === Qt.RightButton)
        row.menuRequested(mapToItem(null, event.x, event.y));
      else
        row.activated();
    }
  }
}
