import QtQuick
import Noctalia
import "../../Commons"
import "../../Widgets"
import ".."

// Noctalia's launcher ("full" layout): a toolbar card (avatar · search · power), a navigation
// row (Recent / Pinned / All / Providers + sort, grid and settings buttons) and the results.
//
// Keys: ↑ ↓ (← → in grid) select · Enter opens · Tab switches view · Esc closes
//       Menu key / Shift+F10 opens the context menu (pin / unpin)
PanelWindow {
  id: launcher

  panelName: "launcher"
  panelWidth: Math.max(560, Math.min(1200, Config.launcher.width ?? 760))
  panelHeight: 620

  readonly property bool compact: Config.launcher.compact ?? false
  readonly property bool showIcons: Config.launcher.show_icons ?? true
  readonly property bool gridSetting: Config.launcher.app_grid ?? false
  readonly property int iconSize: compact ? 28 : 40
  readonly property int gridColumns: 5
  readonly property string pfx: Launcher.prefix

  property string view: "recent"
  property var results: []
  property int currentIndex: 0
  // The grid only makes sense for plain app lists.
  readonly property bool useGrid: gridSetting && results.length > 0 && results.every(r => r.kind === "app")

  readonly property var views: [
    { key: "recent", text: "Recente" },
    { key: "pinned", text: "Fixado" },
    { key: "all", text: "Todos" },
    { key: "providers", text: "Provedores" },
  ]

  function refresh(keepIndex) {
    let r;
    if (search.text.length === 0 && view === "providers")
      r = Launcher.query(pfx, "all");
    else
      r = Launcher.query(search.text, view);
    results = r;
    if (!keepIndex || currentIndex >= results.length)
      currentIndex = 0;
    resultsView.positionAt(currentIndex);
  }

  function setView(v) {
    view = v;
    refresh(false);
  }

  function cycleView(step) {
    const i = views.findIndex(v => v.key === view);
    setView(views[(i + step + views.length) % views.length].key);
  }

  function activate(item) {
    if (!item)
      return;
    const next = Launcher.activate(item);
    if (next === "close")
      Panels.close();
    else if (next === "close-paste")
      Panels.closeAndRestoreFocus(true);
    else if (next.startsWith("set:")) {
      search.text = next.slice(4);
      search.input.cursorPosition = search.text.length;
    }
  }

  function moveSelection(step) {
    if (results.length === 0)
      return;
    currentIndex = Math.max(0, Math.min(results.length - 1, currentIndex + step));
    resultsView.positionAt(currentIndex);
  }

  onOpened: {
    contextMenu.close();
    search.text = panelArg === "session" ? pfx + "session " : Shell.env("NOCTALIA_QUERY");
    view = Config.launcher.default_view ?? "recent";
    refresh(false);
    // Nothing used yet? Show everything instead of an empty "Recent".
    if (results.length === 0 && search.text.length === 0 && view === "recent")
      setView("all");
    search.input.forceActiveFocus();
  }

  Connections {
    target: Launcher
    function onAppsChanged() {
      if (launcher.shown)
        launcher.refresh(true);
    }
  }
  Connections {
    target: Config
    function onChanged() {
      if (launcher.shown)
        launcher.refresh(true);
    }
  }

  Item {
    id: body
    anchors.fill: parent
    anchors.margins: Style.spaceSm

    // ── toolbar card: avatar · search · power ────────────────────────────────
    Rectangle {
      id: toolbar
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.top: parent.top
      height: 44 + 2 * Style.spaceMd
      radius: Style.radiusXl
      color: Theme.colors.surfaceContainer
      border.width: Style.borderWidth
      border.color: Theme.colors.outlineVariant

      Item {
        id: avatar
        width: 44
        height: 44
        anchors.left: parent.left
        anchors.leftMargin: Style.spaceMd
        anchors.verticalCenter: parent.verticalCenter

        NGlyph {
          anchors.centerIn: parent
          icon: "user-circle"
          size: 38
          color: Theme.colors.primary
        }
        MouseArea {
          id: avatarMouse
          anchors.fill: parent
          hoverEnabled: true
        }
      }

      NTextInput {
        id: search
        anchors.left: avatar.right
        anchors.leftMargin: Style.spaceSm
        anchors.right: powerButton.left
        anchors.rightMargin: Style.spaceSm
        anchors.verticalCenter: parent.verticalCenter
        height: 44
        placeholder: "Busque por aplicativos..."
        onTextChanged: launcher.refresh(false)

        onKeyPressed: event => {
          const shift = (event.modifiers & Qt.ShiftModifier) !== 0;
          if (contextMenu.visible) {
            if (event.key === Qt.Key_Escape) {
              contextMenu.close();
              event.accepted = true;
            }
            return;
          }
          switch (event.key) {
          case Qt.Key_Down:
            launcher.moveSelection(launcher.useGrid ? launcher.gridColumns : 1);
            break;
          case Qt.Key_Up:
            launcher.moveSelection(launcher.useGrid ? -launcher.gridColumns : -1);
            break;
          case Qt.Key_Right:
            if (!launcher.useGrid || search.input.cursorPosition < search.text.length)
              return;
            launcher.moveSelection(1);
            break;
          case Qt.Key_Left:
            if (!launcher.useGrid || search.text.length > 0)
              return;
            launcher.moveSelection(-1);
            break;
          case Qt.Key_PageDown:
            launcher.moveSelection(8);
            break;
          case Qt.Key_PageUp:
            launcher.moveSelection(-8);
            break;
          case Qt.Key_Return:
          case Qt.Key_Enter:
            launcher.activate(launcher.results[launcher.currentIndex]);
            break;
          case Qt.Key_Tab:
            launcher.cycleView(1);
            break;
          case Qt.Key_Backtab:
            launcher.cycleView(-1);
            break;
          case Qt.Key_Menu:
            contextMenu.openFor(launcher.currentIndex, null);
            break;
          case Qt.Key_F10:
            if (!shift)
              return;
            contextMenu.openFor(launcher.currentIndex, null);
            break;
          default:
            return;
          }
          event.accepted = true;
        }
      }

      NIconButton {
        id: powerButton
        width: 44
        height: 44
        anchors.right: parent.right
        anchors.rightMargin: Style.spaceMd
        anchors.verticalCenter: parent.verticalCenter
        icon: "shutdown"
        glyphSize: Style.fontSizeTitle
        onClicked: {
          search.text = launcher.pfx + "session ";
          search.input.forceActiveFocus();
        }
      }
    }

    // ── navigation row ───────────────────────────────────────────────────────
    Item {
      id: navRow
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.top: toolbar.bottom
      anchors.topMargin: Style.spaceSm
      height: 32

      NSegmented {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        model: launcher.views
        current: launcher.view
        onSelected: key => {
          launcher.setView(key);
          search.input.forceActiveFocus();
        }
      }

      Row {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        spacing: Style.spaceXs

        NIconButton {
          icon: (Config.launcher.sort_by_usage ?? true) ? "sort-ascending-letters" : "sort-descending-small-big"
          onClicked: Config.setValue("launcher.sort_by_usage", !(Config.launcher.sort_by_usage ?? true))
        }
        NIconButton {
          icon: launcher.gridSetting ? "list" : "layout-grid"
          onClicked: Config.setValue("launcher.app_grid", !launcher.gridSetting)
        }
        NIconButton {
          icon: "settings"
          onClicked: {
            Panels.close();
            Config.openInEditor();
          }
        }
      }
    }

    // ── results ──────────────────────────────────────────────────────────────
    Item {
      id: resultsArea
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.top: navRow.bottom
      anchors.topMargin: Style.spaceSm
      anchors.bottom: parent.bottom
      clip: true

      Item {
        id: resultsView
        anchors.fill: parent

        function positionAt(i) {
          if (launcher.useGrid)
            grid.positionViewAtIndex(i, GridView.Contain);
          else
            list.positionViewAtIndex(i, ListView.Contain);
        }

        ListView {
          id: list
          anchors.fill: parent
          visible: !launcher.useGrid
          model: launcher.useGrid ? [] : launcher.results
          currentIndex: launcher.currentIndex
          boundsBehavior: Flickable.StopAtBounds
          spacing: 2
          reuseItems: true

          delegate: LauncherRow {
            required property var modelData
            required property int index
            width: list.width
            item: modelData
            selected: index === launcher.currentIndex
            iconSize: launcher.iconSize
            compact: launcher.compact
            showIcon: launcher.showIcons
            onActivated: launcher.activate(modelData)
            onMenuRequested: pos => contextMenu.openFor(index, pos)
          }
        }

        GridView {
          id: grid
          anchors.fill: parent
          visible: launcher.useGrid
          model: launcher.useGrid ? launcher.results : []
          cellWidth: Math.floor(width / launcher.gridColumns)
          cellHeight: 112
          currentIndex: launcher.currentIndex
          boundsBehavior: Flickable.StopAtBounds

          delegate: LauncherGridCell {
            required property var modelData
            required property int index
            width: grid.cellWidth
            height: grid.cellHeight
            item: modelData
            selected: index === launcher.currentIndex
            onActivated: launcher.activate(modelData)
            onMenuRequested: pos => contextMenu.openFor(index, pos)
          }
        }
      }

      NText {
        anchors.centerIn: parent
        visible: launcher.results.length === 0
        color: Theme.colors.onSurfaceVariant
        font.weight: Font.Normal
        text: Launcher.loading ? "Carregando resultados..."
            : search.text.length === 0 && launcher.view === "pinned"
              ? "Nenhum app fixado. Clique com o botão direito em um app para fixar."
            : search.text.length === 0 ? "Digite para buscar..." : "Nenhum resultado encontrado"
      }
    }

    // ── context menu (inside the panel so it never steals focus) ─────────────
    MouseArea {
      anchors.fill: parent
      visible: contextMenu.visible
      acceptedButtons: Qt.AllButtons
      onPressed: contextMenu.close()
    }

    Rectangle {
      id: contextMenu

      property int index: -1
      readonly property var item: launcher.results[index]
      readonly property bool isApp: item ? item.kind === "app" : false

      function openFor(i, pos) {
        if (i < 0 || i >= launcher.results.length)
          return;
        index = i;
        launcher.currentIndex = i;
        // pos comes in window coordinates (or null for keyboard: center of the results)
        const p = pos ? body.mapFromItem(null, pos.x, pos.y) : Qt.point(body.width / 2 - width / 2, resultsArea.y + 40);
        x = Math.min(Math.max(4, p.x), body.width - width - 4);
        y = Math.min(Math.max(4, p.y), body.height - height - 4);
        visible = true;
      }
      function close() {
        visible = false;
      }

      visible: false
      width: 240
      height: menuColumn.implicitHeight + 2 * Style.spaceXs
      radius: Style.radiusLg
      color: Theme.colors.surface
      border.width: Style.borderWidth
      border.color: Theme.colors.outlineVariant
      z: 10

      Column {
        id: menuColumn
        anchors.fill: parent
        anchors.margins: Style.spaceXs

        Repeater {
          model: {
            const it = contextMenu.item;
            if (!it)
              return [];
            const entries = [{ icon: "player-play", text: "Abrir", action: () => launcher.activate(it) }];
            if (contextMenu.isApp) {
              entries.push(it.pinned
                ? { icon: "pinned-off", text: "Desfixar do Launcher", action: () => Launcher.setPinned(it.id, false) }
                : { icon: "pin", text: "Fixar no Launcher", action: () => Launcher.setPinned(it.id, true) });
            }
            return entries;
          }

          delegate: Rectangle {
            id: menuEntry
            required property var modelData
            width: menuColumn.width
            height: 34
            radius: Style.radiusMd
            color: entryMouse.containsMouse ? Theme.colors.hover : "transparent"

            Row {
              anchors.left: parent.left
              anchors.leftMargin: Style.spaceSm + 2
              anchors.verticalCenter: parent.verticalCenter
              spacing: Style.spaceSm + 2
              NGlyph {
                icon: menuEntry.modelData.icon
                anchors.verticalCenter: parent.verticalCenter
                color: entryMouse.containsMouse ? Theme.colors.onHover : Theme.colors.onSurface
              }
              NText {
                text: menuEntry.modelData.text
                anchors.verticalCenter: parent.verticalCenter
                font.weight: Font.Normal
                color: entryMouse.containsMouse ? Theme.colors.onHover : Theme.colors.onSurface
              }
            }
            MouseArea {
              id: entryMouse
              anchors.fill: parent
              hoverEnabled: true
              cursorShape: Qt.PointingHandCursor
              onClicked: {
                contextMenu.close();
                menuEntry.modelData.action();
                search.input.forceActiveFocus();
              }
            }
          }
        }
      }
    }
  }
}
