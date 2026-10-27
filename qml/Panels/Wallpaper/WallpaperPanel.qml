import QtQuick
import Noctalia
import "../../Commons"
import "../../Widgets"
import ".."

// Noctalia's wallpaper panel: thumbnails of the wallpaper folder, search, random, and the
// "colors from wallpaper" switch (Material You palette generated from the image).
PanelWindow {
  id: wp

  panelName: "wallpaper"
  placement: "bar"
  panelWidth: 820
  panelHeight: 600

  property string filter: ""
  readonly property var filtered: {
    const f = filter.toLowerCase();
    return f.length === 0 ? Wallpaper.images : Wallpaper.images.filter(i => i.name.toLowerCase().indexOf(f) >= 0);
  }
  readonly property bool fromWallpaper: Theme.source === "wallpaper"

  onOpened: {
    Wallpaper.refresh();
    search.text = "";
    search.input.forceActiveFocus();
  }

  Item {
    anchors.fill: parent
    anchors.margins: Style.panelPadding

    // ── header ───────────────────────────────────────────────────────────────
    Item {
      id: header
      width: parent.width
      height: 32
      NText {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        text: "Papel de parede"
        font.pixelSize: Style.fontSizeHeader
        font.weight: Font.Bold
      }
      Row {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        spacing: Style.spaceXs
        NIconButton { icon: "arrows-shuffle"; onClicked: Wallpaper.random() }
        NIconButton { icon: "folder"; onClicked: { Panels.close(); Wallpaper.openFolder(); } }
        NIconButton { icon: "refresh"; onClicked: Wallpaper.refresh() }
        NIconButton { icon: "x"; onClicked: Panels.close() }
      }
    }

    // ── search + colors-from-wallpaper ───────────────────────────────────────
    Item {
      id: tools
      anchors.top: header.bottom
      anchors.topMargin: Style.spaceMd
      width: parent.width
      height: 38

      NTextInput {
        id: search
        anchors.left: parent.left
        anchors.right: themeBox.left
        anchors.rightMargin: Style.spaceMd
        height: 38
        placeholder: "Buscar em " + Wallpaper.directory
        onTextChanged: wp.filter = text
      }

      Rectangle {
        id: themeBox
        anchors.right: parent.right
        height: 38
        width: themeRow.implicitWidth + 2 * Style.spaceMd
        radius: Style.radiusMd
        color: Theme.colors.surfaceContainer
        border.width: Style.borderWidth
        border.color: Theme.colors.outlineVariant
        Row {
          id: themeRow
          anchors.centerIn: parent
          spacing: Style.spaceSm
          NGlyph { icon: "palette"; anchors.verticalCenter: parent.verticalCenter; color: Theme.colors.primary }
          NText {
            anchors.verticalCenter: parent.verticalCenter
            text: Theme.generating ? "Gerando cores..." : "Cores do papel de parede"
            font.pixelSize: Style.fontSizeCaption
          }
          NToggle {
            anchors.verticalCenter: parent.verticalCenter
            checked: wp.fromWallpaper
            onToggled: on => on ? Theme.useWallpaper(Config.theme.wallpaper_scheme ?? "m3-content")
                                : Theme.setBuiltin(Config.theme.builtin ?? "Noctalia")
          }
        }
      }
    }

    // scheme chips (only when colors come from the wallpaper)
    Flow {
      id: schemes
      anchors.top: tools.bottom
      anchors.topMargin: wp.fromWallpaper ? Style.spaceSm : 0
      width: parent.width
      spacing: Style.spaceXs
      visible: wp.fromWallpaper
      height: visible ? implicitHeight : 0
      Repeater {
        model: [
          { key: "m3-content", text: "Conteúdo" }, { key: "m3-tonal-spot", text: "Tonal" },
          { key: "m3-fruit-salad", text: "Salada de frutas" }, { key: "m3-rainbow", text: "Arco-íris" },
          { key: "m3-monochrome", text: "Monocromático" }, { key: "vibrant", text: "Vibrante" },
          { key: "faithful", text: "Fiel" }, { key: "soft", text: "Suave" }, { key: "muted", text: "Discreto" },
        ]
        delegate: Rectangle {
          id: chip
          required property var modelData
          readonly property bool isCurrent: (Config.theme.wallpaper_scheme ?? "m3-content") === modelData.key
          height: 26
          width: chipText.implicitWidth + 2 * Style.spaceMd
          radius: 13
          color: isCurrent ? Theme.colors.primary : chipMouse.containsMouse ? Theme.colors.hover : Theme.colors.surfaceContainer
          border.width: isCurrent ? 0 : Style.borderWidth
          border.color: Theme.colors.outlineVariant
          NText {
            id: chipText
            anchors.centerIn: parent
            text: chip.modelData.text
            font.pixelSize: Style.fontSizeCaption
            color: chip.isCurrent ? Theme.colors.onPrimary : chipMouse.containsMouse ? Theme.colors.onHover : Theme.colors.onSurface
          }
          MouseArea {
            id: chipMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: Theme.useWallpaper(chip.modelData.key)
          }
        }
      }
    }

    // ── thumbnails ───────────────────────────────────────────────────────────
    GridView {
      id: grid
      anchors.top: schemes.bottom
      anchors.topMargin: Style.spaceMd
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.bottom: parent.bottom
      clip: true
      boundsBehavior: Flickable.StopAtBounds
      model: wp.filtered
      cellWidth: Math.floor(width / 4)
      cellHeight: Math.round(cellWidth * 0.62) + 22

      delegate: Item {
        id: thumb
        required property var modelData
        readonly property bool isCurrent: modelData.path.toLowerCase() === Wallpaper.current.toLowerCase()
        width: grid.cellWidth
        height: grid.cellHeight

        Rectangle {
          id: frame
          anchors.left: parent.left
          anchors.right: parent.right
          anchors.top: parent.top
          anchors.margins: 4
          height: parent.height - 26
          radius: Style.radiusLg
          color: Theme.colors.surfaceContainer
          border.width: thumb.isCurrent ? 3 : thumbMouse.containsMouse ? 2 : 0
          border.color: thumb.isCurrent ? Theme.colors.primary : Theme.colors.hover
          clip: true

          Image {
            anchors.fill: parent
            anchors.margins: frame.border.width
            source: thumb.modelData.url
            sourceSize: Qt.size(320, 200)
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            cache: true
          }
        }
        NText {
          anchors.top: frame.bottom
          anchors.topMargin: 3
          anchors.left: frame.left
          anchors.right: frame.right
          horizontalAlignment: Text.AlignHCenter
          text: thumb.modelData.name
          font.pixelSize: Style.fontSizeMini
          font.weight: thumb.isCurrent ? Font.Bold : Font.Normal
          color: thumb.isCurrent ? Theme.colors.primary : Theme.colors.onSurfaceVariant
        }
        MouseArea {
          id: thumbMouse
          anchors.fill: parent
          hoverEnabled: true
          cursorShape: Qt.PointingHandCursor
          onClicked: Wallpaper.set(thumb.modelData.path)
        }
      }

      NText {
        anchors.centerIn: parent
        visible: wp.filtered.length === 0
        width: parent.width * 0.8
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        color: Theme.colors.onSurfaceVariant
        text: Wallpaper.loading ? "Procurando imagens..."
            : wp.filter.length > 0 ? "Nenhuma imagem encontrada"
            : "Nenhuma imagem em " + Wallpaper.directory + ". Coloque seus papéis de parede nessa pasta ou mude \"wallpaper.directory\" no config."
      }
    }
  }
}
