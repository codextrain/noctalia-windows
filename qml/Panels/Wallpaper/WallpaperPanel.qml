import QtQuick
import Noctalia
import "../../Commons"
import "../../Widgets"
import ".."

// Noctalia's wallpaper panel with three tabs:
//   Meus          thumbnails of the wallpaper folder, search, random, colors from wallpaper
//   Wallhaven     search wallhaven.cc, download one or thousands
//   Coleções rice GitHub repositories of "rice" wallpapers (Catppuccin, Gruvbox, Nord...)
// Open a tab directly with Panels.toggle("wallpaper:wallhaven") / "wallpaper:collections".
PanelWindow {
  id: wp

  panelName: "wallpaper"
  placement: "bar"
  panelWidth: 940
  panelHeight: 660

  property string tab: "local"
  property string filter: ""
  readonly property var filtered: {
    const f = filter.toLowerCase();
    return f.length === 0 ? Wallpaper.images : Wallpaper.images.filter(i => i.name.toLowerCase().indexOf(f) >= 0);
  }
  readonly property bool fromWallpaper: Theme.source === "wallpaper"

  // Wallhaven search state
  property string whQuery: ""
  property string whSorting: "toplist"
  property string whResolution: "1920x1080"
  property int bulkCount: 500
  property string showing: "" // what WallStore.results currently holds: "wallhaven" | collection repo
  property string collection: ""

  function whSearch() {
    showing = "wallhaven";
    WallStore.searchWallhaven(whQuery, whSorting, whResolution, 1);
  }
  function openCollection(repo) {
    collection = repo;
    showing = repo;
    WallStore.openCollection(repo);
  }
  function selectTab(key) {
    tab = key;
    if (key === "wallhaven" && showing !== "wallhaven")
      whSearch();
    else if (key === "collections" && (showing === "" || showing === "wallhaven")) {
      const list = WallStore.collections;
      if (list.length > 0)
        openCollection(collection.length > 0 ? collection : list[0].repo);
    } else if (key === "local")
      search.input.forceActiveFocus();
  }

  onPanelArgChanged: if (panelArg === "wallhaven" || panelArg === "collections") selectTab(panelArg)
  onOpened: {
    Wallpaper.refresh();
    search.text = "";
    // (panelArg may not be updated yet when opened() fires)
    const arg = Panels.current.indexOf(":") > 0 ? Panels.current.slice(Panels.current.indexOf(":") + 1) : "";
    selectTab(arg === "wallhaven" || arg === "collections" ? arg : tab);
    if (tab === "local")
      search.input.forceActiveFocus();
  }

  Item {
    anchors.fill: parent
    anchors.margins: Style.panelPadding

    // ── header ───────────────────────────────────────────────────────────────
    Item {
      id: header
      width: parent.width
      height: 34
      NText {
        id: title
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        text: "Papel de parede"
        font.pixelSize: Style.fontSizeHeader
        font.weight: Font.Bold
      }
      NSegmented {
        anchors.left: title.right
        anchors.leftMargin: Style.spaceLg
        anchors.verticalCenter: parent.verticalCenter
        model: [
          { key: "local", text: "Meus", icon: "photo" },
          { key: "wallhaven", text: "Wallhaven", icon: "world-search" },
          { key: "collections", text: "Coleções rice", icon: "brush" },
        ]
        current: wp.tab
        onSelected: key => wp.selectTab(key)
      }
      Row {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        spacing: Style.spaceXs
        NIconButton { icon: "arrows-shuffle"; tooltip: "Aleatório"; visible: wp.tab === "local"; onClicked: Wallpaper.random() }
        NIconButton {
          icon: "folder"
          tooltip: "Abrir pasta"
          onClicked: { Panels.close(); wp.tab === "local" ? Wallpaper.openFolder() : WallStore.openDownloadFolder(); }
        }
        NIconButton { icon: "refresh"; visible: wp.tab === "local"; onClicked: Wallpaper.refresh(true) }
        NIconButton { icon: "x"; onClicked: Panels.close() }
      }
    }

    // ── tab: my wallpapers ───────────────────────────────────────────────────
    Item {
      id: localTab
      anchors.top: header.bottom
      anchors.topMargin: Style.spaceMd
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.bottom: statusBar.top
      visible: wp.tab === "local"

      Item {
        id: tools
        width: parent.width
        height: 38

        NTextInput {
          id: search
          anchors.left: parent.left
          anchors.right: themeBox.left
          anchors.rightMargin: Style.spaceMd
          height: 38
          placeholder: "Buscar em " + Wallpaper.directory + " (" + Wallpaper.images.length + " imagens)"
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
          delegate: Chip {
            required property var modelData
            text: modelData.text
            selected: (Config.theme.wallpaper_scheme ?? "m3-content") === modelData.key
            onClicked: Theme.useWallpaper(modelData.key)
          }
        }
      }

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
        cacheBuffer: 400

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
            elide: Text.ElideMiddle
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

        EmptyText {
          visible: wp.filtered.length === 0
          text: Wallpaper.loading ? "Procurando imagens..."
              : wp.filter.length > 0 ? "Nenhuma imagem encontrada"
              : "Nenhuma imagem em " + Wallpaper.directory + ". Baixe alguns nas abas Wallhaven e Coleções rice, ou coloque seus papéis de parede nessa pasta."
        }
      }
    }

    // ── tab: Wallhaven ───────────────────────────────────────────────────────
    Item {
      id: whTab
      anchors.top: header.bottom
      anchors.topMargin: Style.spaceMd
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.bottom: statusBar.top
      visible: wp.tab === "wallhaven"

      Row {
        id: whTools
        width: parent.width
        spacing: Style.spaceSm
        NTextInput {
          id: whInput
          width: parent.width - sortSeg.width - resSeg.width - 2 * Style.spaceSm
          height: 34
          placeholder: "Buscar no Wallhaven (em inglês: minimalism, pixel art...)"
          onKeyPressed: event => {
            if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
              wp.whQuery = text;
              wp.whSearch();
              event.accepted = true;
            }
          }
        }
        NSegmented {
          id: sortSeg
          model: [
            { key: "toplist", text: "Em alta" }, { key: "date_added", text: "Novos" },
            { key: "random", text: "Aleatório" }, { key: "favorites", text: "Favoritos" },
          ]
          current: wp.whSorting
          onSelected: key => { wp.whSorting = key; wp.whSearch(); }
        }
        NSegmented {
          id: resSeg
          model: [
            { key: "", text: "Todas" }, { key: "1920x1080", text: "1080p" },
            { key: "2560x1440", text: "1440p" }, { key: "3840x2160", text: "4K" },
          ]
          current: wp.whResolution
          onSelected: key => { wp.whResolution = key; wp.whSearch(); }
        }
      }

      Flow {
        id: tags
        anchors.top: whTools.bottom
        anchors.topMargin: Style.spaceSm
        width: parent.width
        spacing: Style.spaceXs
        Repeater {
          model: ["minimalism", "pixel art", "anime", "landscape", "abstract", "space", "cyberpunk", "nature",
                  "dark", "city", "digital art", "lofi", "mountains", "catppuccin", "gruvbox", "nord"]
          delegate: Chip {
            required property string modelData
            text: modelData
            selected: wp.whQuery === modelData
            onClicked: {
              wp.whQuery = selected ? "" : modelData;
              whInput.text = wp.whQuery;
              wp.whSearch();
            }
          }
        }
      }

      OnlineGrid {
        id: whGrid
        anchors.top: tags.bottom
        anchors.topMargin: Style.spaceSm
        anchors.bottom: whFooter.top
        anchors.bottomMargin: Style.spaceSm
        width: parent.width
        active: wp.showing === "wallhaven"
        onAtYEndChanged: if (atYEnd && active) WallStore.loadMore()
      }

      Row {
        id: whFooter
        anchors.bottom: parent.bottom
        width: parent.width
        height: 34
        spacing: Style.spaceSm
        NText {
          anchors.verticalCenter: parent.verticalCenter
          width: parent.width - bulkLabel.width - bulkSeg.width - bulkButton.width - 3 * Style.spaceSm
          elide: Text.ElideRight
          color: Theme.colors.onSurfaceVariant
          font.pixelSize: Style.fontSizeCaption
          text: WallStore.total > 0 ? WallStore.total.toLocaleString(Qt.locale(), "f", 0) + " wallpapers nessa busca · role pra carregar mais" : ""
        }
        NText {
          id: bulkLabel
          anchors.verticalCenter: parent.verticalCenter
          text: "Baixar desta busca:"
          font.pixelSize: Style.fontSizeCaption
        }
        NSegmented {
          id: bulkSeg
          anchors.verticalCenter: parent.verticalCenter
          model: [{ key: "100", text: "100" }, { key: "500", text: "500" }, { key: "1000", text: "1.000" }, { key: "5000", text: "5.000" }]
          current: String(wp.bulkCount)
          onSelected: key => wp.bulkCount = parseInt(key)
        }
        ActionButton {
          id: bulkButton
          anchors.verticalCenter: parent.verticalCenter
          icon: "cloud-download"
          text: "Baixar"
          onClicked: WallStore.downloadWallhaven(wp.whQuery, wp.whSorting, wp.whResolution, wp.bulkCount)
        }
      }
    }

    // ── tab: rice collections (GitHub) ───────────────────────────────────────
    Item {
      id: colTab
      anchors.top: header.bottom
      anchors.topMargin: Style.spaceMd
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.bottom: statusBar.top
      visible: wp.tab === "collections"

      Flow {
        id: colChips
        width: parent.width
        spacing: Style.spaceXs
        Repeater {
          model: WallStore.collections
          delegate: Chip {
            required property var modelData
            text: modelData.name
            selected: wp.collection === modelData.repo
            onClicked: wp.openCollection(modelData.repo)
            onRightClicked: WallStore.removeCollection(modelData.repo)
          }
        }
        NTextInput {
          id: addInput
          width: 230
          height: 26
          icon: "plus"
          placeholder: "dono/repositório + Enter"
          onKeyPressed: event => {
            if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
              const repo = text.trim();
              if (WallStore.addCollection(repo)) {
                text = "";
                wp.openCollection(repo.replace(/^https?:\/\/(www\.)?github\.com\//, "").replace(/\/tree\/[^/]+/, ""));
              }
              event.accepted = true;
            }
          }
        }
      }

      Row {
        id: colInfo
        anchors.top: colChips.bottom
        anchors.topMargin: Style.spaceSm
        width: parent.width
        height: 34
        spacing: Style.spaceSm
        readonly property var current: WallStore.collections.find(c => c.repo === wp.collection) ?? null
        NText {
          anchors.verticalCenter: parent.verticalCenter
          width: parent.width - colDownload.width - colOpen.width - 2 * Style.spaceSm
          elide: Text.ElideRight
          color: Theme.colors.onSurfaceVariant
          font.pixelSize: Style.fontSizeCaption
          text: !colInfo.current ? "" : (colInfo.current.description.length ? colInfo.current.description + " · " : "")
                + (wp.showing === wp.collection && !WallStore.busy ? WallStore.total + " imagens" : "carregando...")
                + " · botão direito no nome remove da lista"
        }
        NIconButton {
          id: colOpen
          anchors.verticalCenter: parent.verticalCenter
          icon: "brand-github"
          tooltip: "Abrir no GitHub"
          onClicked: { Panels.close(); Shell.openUri("https://github.com/" + wp.collection); }
        }
        ActionButton {
          id: colDownload
          anchors.verticalCenter: parent.verticalCenter
          icon: "cloud-download"
          text: "Baixar coleção inteira"
          enabled: wp.showing === wp.collection && WallStore.results.length > 0
          onClicked: WallStore.downloadResults()
        }
      }

      OnlineGrid {
        anchors.top: colInfo.bottom
        anchors.topMargin: Style.spaceSm
        anchors.bottom: parent.bottom
        width: parent.width
        active: wp.showing === wp.collection && wp.collection.length > 0
      }
    }

    // ── download progress / errors (all tabs) ────────────────────────────────
    Column {
      id: statusBar
      anchors.bottom: parent.bottom
      width: parent.width
      spacing: Style.spaceXs
      topPadding: visible ? Style.spaceSm : 0
      readonly property bool hasQueue: WallStore.downloading || WallStore.queueStatus.length > 0
      visible: hasQueue || (WallStore.error.length > 0 && wp.tab !== "local")
      height: visible ? implicitHeight : 0

      NText {
        width: parent.width
        visible: WallStore.error.length > 0 && wp.tab !== "local"
        text: WallStore.error
        color: Theme.colors.error
        font.pixelSize: Style.fontSizeCaption
        wrapMode: Text.Wrap
      }
      Row {
        width: parent.width
        height: 28
        spacing: Style.spaceSm
        visible: statusBar.hasQueue
        NGlyph {
          anchors.verticalCenter: parent.verticalCenter
          icon: WallStore.downloading ? "cloud-download" : "circle-check"
          color: Theme.colors.primary
        }
        NText {
          anchors.verticalCenter: parent.verticalCenter
          width: 360
          elide: Text.ElideMiddle
          text: WallStore.queueStatus
          font.pixelSize: Style.fontSizeCaption
        }
        NProgress {
          anchors.verticalCenter: parent.verticalCenter
          width: parent.width - 360 - 28 - cancelButton.width - 3 * Style.spaceSm
          height: 6
          value: WallStore.queueTotal > 0 ? (WallStore.queueDone + WallStore.queueFailed) / WallStore.queueTotal : 1
        }
        NIconButton {
          id: cancelButton
          anchors.verticalCenter: parent.verticalCenter
          icon: WallStore.downloading ? "player-stop" : "folder"
          tooltip: WallStore.downloading ? "Cancelar downloads" : "Abrir pasta"
          onClicked: WallStore.downloading ? WallStore.cancelDownloads() : WallStore.openDownloadFolder()
        }
      }
    }
  }

  // ── small reusable pieces ──────────────────────────────────────────────────

  component Chip: Rectangle {
    id: chip
    property string text: ""
    property bool selected: false
    signal clicked
    signal rightClicked
    height: 26
    width: chipText.implicitWidth + 2 * Style.spaceMd
    radius: 13
    color: selected ? Theme.colors.primary : chipMouse.containsMouse ? Theme.colors.hover : Theme.colors.surfaceContainer
    border.width: selected ? 0 : Style.borderWidth
    border.color: Theme.colors.outlineVariant
    NText {
      id: chipText
      anchors.centerIn: parent
      text: chip.text
      font.pixelSize: Style.fontSizeCaption
      color: chip.selected ? Theme.colors.onPrimary : chipMouse.containsMouse ? Theme.colors.onHover : Theme.colors.onSurface
    }
    MouseArea {
      id: chipMouse
      anchors.fill: parent
      hoverEnabled: true
      acceptedButtons: Qt.LeftButton | Qt.RightButton
      cursorShape: Qt.PointingHandCursor
      onClicked: mouse => mouse.button === Qt.RightButton ? chip.rightClicked() : chip.clicked()
    }
  }

  component ActionButton: Rectangle {
    id: btn
    property string icon: ""
    property string text: ""
    signal clicked
    height: 30
    width: btnRow.implicitWidth + 2 * Style.spaceMd
    radius: 15
    opacity: enabled ? 1 : 0.45
    color: btnMouse.containsMouse && enabled ? Theme.colors.hover : Theme.colors.primary
    Row {
      id: btnRow
      anchors.centerIn: parent
      spacing: Style.spaceXs
      NGlyph { icon: btn.icon; size: 16; anchors.verticalCenter: parent.verticalCenter; color: btnMouse.containsMouse ? Theme.colors.onHover : Theme.colors.onPrimary }
      NText {
        anchors.verticalCenter: parent.verticalCenter
        text: btn.text
        font.pixelSize: Style.fontSizeCaption
        font.weight: Font.DemiBold
        color: btnMouse.containsMouse ? Theme.colors.onHover : Theme.colors.onPrimary
      }
    }
    MouseArea {
      id: btnMouse
      anchors.fill: parent
      hoverEnabled: true
      enabled: btn.enabled
      cursorShape: Qt.PointingHandCursor
      onClicked: btn.clicked()
    }
  }

  component EmptyText: NText {
    anchors.centerIn: parent
    width: parent.width * 0.8
    horizontalAlignment: Text.AlignHCenter
    wrapMode: Text.Wrap
    color: Theme.colors.onSurfaceVariant
  }

  // Grid of online results: click = download and use; the download button only saves it.
  component OnlineGrid: GridView {
    id: og
    property bool active: true
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    model: active ? WallStore.results : []
    cellWidth: Math.floor(width / 4)
    cellHeight: Math.round(cellWidth * 0.62) + 8
    cacheBuffer: 600

    // (explicit Component: Qt 6.4 rejects an inline delegate inside an inline component)
    delegate: Component { Item {
      id: cell
      required property var modelData
      readonly property string localPath: WallStore.downloaded[modelData.key] ?? ""
      readonly property bool isCurrent: localPath.length > 0 && localPath.toLowerCase() === Wallpaper.current.toLowerCase()
      width: og.cellWidth
      height: og.cellHeight

      Rectangle {
        id: card
        anchors.fill: parent
        anchors.margins: 4
        radius: Style.radiusLg
        color: Theme.colors.surfaceContainer
        border.width: cell.isCurrent ? 3 : cellMouse.containsMouse ? 2 : 0
        border.color: cell.isCurrent ? Theme.colors.primary : Theme.colors.hover
        clip: true

        Image {
          id: img
          anchors.fill: parent
          anchors.margins: card.border.width
          source: cell.modelData.thumb
          sourceSize: Qt.size(400, 250)
          fillMode: Image.PreserveAspectCrop
          asynchronous: true
          cache: true
        }
        NGlyph {
          anchors.centerIn: parent
          visible: img.status !== Image.Ready
          icon: img.status === Image.Error ? "photo" : "loader"
          color: Theme.colors.onSurfaceVariant
        }

        // info strip
        Rectangle {
          anchors.left: parent.left
          anchors.right: parent.right
          anchors.bottom: parent.bottom
          anchors.margins: card.border.width
          height: 22
          visible: cellMouse.containsMouse
          color: Style.alpha(Theme.colors.surface, 0.82)
          NText {
            anchors.left: parent.left
            anchors.leftMargin: Style.spaceSm
            anchors.right: parent.right
            anchors.rightMargin: 34
            anchors.verticalCenter: parent.verticalCenter
            elide: Text.ElideMiddle
            font.pixelSize: Style.fontSizeMini
            text: [cell.modelData.resolution, cell.modelData.fileSize, cell.modelData.source === "github" ? cell.modelData.name : ""]
                  .filter(s => s && s.length).join(" · ")
          }
        }

        // downloaded badge / download button
        Rectangle {
          anchors.right: parent.right
          anchors.top: parent.top
          anchors.margins: 6
          width: 26
          height: 26
          radius: 13
          visible: cell.localPath.length > 0 || cellMouse.containsMouse || dlMouse.containsMouse
          color: cell.localPath.length > 0 ? Theme.colors.primary
               : dlMouse.containsMouse ? Theme.colors.hover : Style.alpha(Theme.colors.surface, 0.85)
          NGlyph {
            anchors.centerIn: parent
            size: 15
            icon: cell.localPath.length > 0 ? "check" : "download"
            color: cell.localPath.length > 0 ? Theme.colors.onPrimary : Theme.colors.onSurface
          }
          MouseArea {
            id: dlMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: WallStore.download(cell.modelData, false)
          }
        }
      }
      MouseArea {
        id: cellMouse
        anchors.fill: parent
        z: -1
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: WallStore.download(cell.modelData, true)
      }
    } }

    EmptyText {
      visible: og.count === 0
      text: WallStore.busy ? "Carregando..." : WallStore.error.length > 0 ? "" : "Nada por aqui ainda."
    }
  }
}
