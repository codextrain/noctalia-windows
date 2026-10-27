import QtQuick
import Noctalia
import "../../Commons"
import "../../Widgets"
import ".."

// Noctalia's clipboard history: search, pinned entries on top, Enter / click pastes into the
// window you were using. Keys: ↑ ↓ select · Enter paste · Ctrl+P pin · Delete remove · Esc close
PanelWindow {
  id: clipPanel

  panelName: "clipboard"
  panelWidth: 620
  panelHeight: 560

  property int currentIndex: 0
  readonly property var filtered: {
    const f = search.text.toLowerCase();
    const all = Clipboard.entries;
    return f.length === 0 ? all : all.filter(e => e.kind === "text" && e.text.toLowerCase().indexOf(f) >= 0);
  }

  function paste(entry) {
    if (!entry)
      return;
    if (Clipboard.activate(entry.id))
      Panels.closeAndRestoreFocus(Config.launcher.auto_paste ?? true);
  }

  onOpened: {
    search.text = "";
    currentIndex = 0;
    search.input.forceActiveFocus();
  }

  Item {
    anchors.fill: parent
    anchors.margins: Style.panelPadding

    Item {
      id: header
      width: parent.width
      height: 32
      NText {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        text: "Área de transferência"
        font.pixelSize: Style.fontSizeHeader
        font.weight: Font.Bold
      }
      Row {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        spacing: Style.spaceXs
        NIconButton { icon: "trash"; onClicked: Clipboard.clear() }
        NIconButton { icon: "x"; onClicked: Panels.close() }
      }
    }

    NTextInput {
      id: search
      anchors.top: header.bottom
      anchors.topMargin: Style.spaceMd
      width: parent.width
      height: 40
      placeholder: "Buscar no histórico..."
      onTextChanged: clipPanel.currentIndex = 0
      onKeyPressed: event => {
        const items = clipPanel.filtered;
        switch (event.key) {
        case Qt.Key_Down:
          clipPanel.currentIndex = Math.min(items.length - 1, clipPanel.currentIndex + 1);
          break;
        case Qt.Key_Up:
          clipPanel.currentIndex = Math.max(0, clipPanel.currentIndex - 1);
          break;
        case Qt.Key_Return:
        case Qt.Key_Enter:
          clipPanel.paste(items[clipPanel.currentIndex]);
          break;
        case Qt.Key_Delete:
          if (search.text.length > 0)
            return;
          if (items[clipPanel.currentIndex])
            Clipboard.remove(items[clipPanel.currentIndex].id);
          break;
        case Qt.Key_P:
          if (!(event.modifiers & Qt.ControlModifier))
            return;
          if (items[clipPanel.currentIndex])
            Clipboard.togglePin(items[clipPanel.currentIndex].id);
          break;
        default:
          return;
        }
        list.positionViewAtIndex(clipPanel.currentIndex, ListView.Contain);
        event.accepted = true;
      }
    }

    ListView {
      id: list
      anchors.top: search.bottom
      anchors.topMargin: Style.spaceMd
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.bottom: parent.bottom
      clip: true
      spacing: Style.spaceXs
      boundsBehavior: Flickable.StopAtBounds
      model: clipPanel.filtered

      delegate: Rectangle {
        id: row
        required property var modelData
        required property int index
        readonly property bool selected: index === clipPanel.currentIndex
        readonly property bool hovered: rowMouse.containsMouse
        readonly property color fg: selected ? Theme.colors.onPrimary : hovered ? Theme.colors.onHover : Theme.colors.onSurface
        width: ListView.view.width
        height: modelData.kind === "image" ? 96 : 56
        radius: Style.radiusLg
        color: selected ? Theme.colors.primary : hovered ? Theme.colors.hover : Theme.colors.surfaceContainer
        border.width: selected || hovered ? 0 : Style.borderWidth
        border.color: Theme.colors.outlineVariant

        MouseArea {
          id: rowMouse
          anchors.fill: parent
          hoverEnabled: true
          cursorShape: Qt.PointingHandCursor
          onClicked: clipPanel.paste(row.modelData)
        }

        NGlyph {
          id: kindIcon
          anchors.left: parent.left
          anchors.leftMargin: Style.spaceMd
          anchors.verticalCenter: parent.verticalCenter
          icon: row.modelData.pinned ? "pinned" : row.modelData.kind === "image" ? "photo" : "clipboard"
          color: row.fg
        }

        Image {
          visible: row.modelData.kind === "image"
          anchors.left: kindIcon.right
          anchors.leftMargin: Style.spaceMd
          anchors.verticalCenter: parent.verticalCenter
          height: parent.height - 16
          width: height * 1.6
          source: row.modelData.image
          sourceSize: Qt.size(240, 150)
          fillMode: Image.PreserveAspectFit
          asynchronous: true
        }

        Column {
          visible: row.modelData.kind === "text"
          anchors.left: kindIcon.right
          anchors.leftMargin: Style.spaceMd
          anchors.right: actions.left
          anchors.rightMargin: Style.spaceSm
          anchors.verticalCenter: parent.verticalCenter
          NText {
            width: parent.width
            text: row.modelData.preview
            maximumLineCount: 1
            font.weight: Font.Normal
            color: row.fg
          }
          NText {
            width: parent.width
            text: row.modelData.time + " · " + row.modelData.text.length + " caracteres"
            font.pixelSize: Style.fontSizeMini
            font.weight: Font.Normal
            color: row.selected ? Style.alpha(Theme.colors.onPrimary, 0.75) : Theme.colors.onSurfaceVariant
          }
        }

        Row {
          id: actions
          anchors.right: parent.right
          anchors.rightMargin: Style.spaceSm
          anchors.verticalCenter: parent.verticalCenter
          spacing: 2
          opacity: row.hovered || row.selected ? 1 : 0
          NIconButton { icon: row.modelData.pinned ? "pinned-off" : "pin"; onClicked: Clipboard.togglePin(row.modelData.id) }
          NIconButton { icon: "trash"; onClicked: Clipboard.remove(row.modelData.id) }
        }
      }

      NText {
        anchors.centerIn: parent
        visible: clipPanel.filtered.length === 0
        color: Theme.colors.onSurfaceVariant
        text: search.text.length > 0 ? "Nada encontrado" : "Copie algo (Ctrl+C) e ele aparece aqui."
      }
    }
  }
}
