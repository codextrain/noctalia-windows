import QtQuick
import QtQuick.Window
import Noctalia
import "../Commons"
import "../Widgets"

// Settings window: every option of config.json with a UI. Changes are saved immediately.
Window {
  id: win

  property string page: "appearance"
  readonly property var pages: [
    { key: "appearance", title: "Aparência", icon: "palette", source: "AppearancePage.qml" },
    { key: "bar", title: "Barra", icon: "layout-navbar", source: "BarPage.qml" },
    { key: "launcher", title: "Launcher e atalhos", icon: "search", source: "LauncherPage.qml" },
    { key: "dock", title: "Dock e widgets", icon: "layout-bottombar", source: "DockPage.qml" },
    { key: "panels", title: "Painéis", icon: "layout-sidebar-right", source: "PanelsPage.qml" },
    { key: "system", title: "Sistema", icon: "settings", source: "SystemPage.qml" },
  ]

  function openPage(p) {
    if (p && pages.some(x => x.key === p))
      page = p;
    visible = true;
    raise();
    requestActivate();
    Panels.focusWindow(win);
  }

  title: "Configurações do Vitória Shell"
  objectName: "settings"
  flags: Qt.Window | Qt.FramelessWindowHint
  color: "transparent"
  width: 960
  height: 680
  minimumWidth: 760
  minimumHeight: 480
  visible: Shell.env("NOCTALIA_OPEN") === "settings"
  Component.onCompleted: if (Shell.env("NOCTALIA_SETTINGS_PAGE").length > 0) page = Shell.env("NOCTALIA_SETTINGS_PAGE")

  Connections {
    target: Shell
    function onSettingsRequested(p) {
      Panels.close();
      win.openPage(p);
    }
  }

  Shortcut { sequence: "Escape"; onActivated: win.close() }

  Rectangle {
    anchors.fill: parent
    radius: Style.radiusXl
    color: Style.glassColor(Theme.colors.surface, 1)
    border.width: Style.borderWidth
    border.color: Theme.colors.outlineVariant

    // title bar (drag to move)
    Item {
      id: titleBar
      width: parent.width
      height: 48
      MouseArea {
        anchors.fill: parent
        onPressed: win.startSystemMove()
        onDoubleClicked: win.visibility === Window.Maximized ? win.showNormal() : win.showMaximized()
      }
      Row {
        anchors.left: parent.left
        anchors.leftMargin: Style.panelPadding
        anchors.verticalCenter: parent.verticalCenter
        spacing: Style.spaceSm
        NGlyph { icon: "vitoria-cat"; size: 20; color: Theme.colors.primary; anchors.verticalCenter: parent.verticalCenter }
        NText { text: "Configurações"; font.pixelSize: Style.fontSizeTitle; font.weight: Font.Bold; anchors.verticalCenter: parent.verticalCenter }
      }
      Row {
        anchors.right: parent.right
        anchors.rightMargin: Style.spaceSm
        anchors.verticalCenter: parent.verticalCenter
        NIconButton { icon: "minus"; onClicked: win.showMinimized() }
        NIconButton { icon: "x"; onClicked: win.close() }
      }
    }

    // navigation
    Rectangle {
      id: nav
      anchors.top: titleBar.bottom
      anchors.left: parent.left
      anchors.bottom: parent.bottom
      anchors.margins: Style.panelPadding
      anchors.topMargin: 0
      width: 220
      radius: Style.radiusXl
      color: Theme.colors.surfaceContainer
      Column {
        anchors.fill: parent
        anchors.margins: Style.spaceSm
        spacing: Style.spaceXs
        Repeater {
          model: win.pages
          delegate: NListItem {
            required property var modelData
            width: parent.width
            icon: modelData.icon
            title: modelData.title
            checked: win.page === modelData.key
            onClicked: win.page = modelData.key
          }
        }
      }
    }

    Flickable {
      id: body
      anchors.top: titleBar.bottom
      anchors.left: nav.right
      anchors.right: parent.right
      anchors.bottom: parent.bottom
      anchors.leftMargin: Style.panelPadding
      anchors.rightMargin: Style.panelPadding
      anchors.bottomMargin: Style.panelPadding
      clip: true
      contentHeight: pageLoader.item ? pageLoader.item.implicitHeight + Style.spaceLg : 0
      boundsBehavior: Flickable.StopAtBounds

      Loader {
        id: pageLoader
        width: body.width
        source: (win.pages.find(p => p.key === win.page) ?? win.pages[0]).source
        onLoaded: body.contentY = 0
      }
    }
  }
}
