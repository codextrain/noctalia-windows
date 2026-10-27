import QtQuick
import QtQuick.Window
import Noctalia
import "../Commons"
import "../Widgets"

// Noctalia's OSD: a small card with an icon, a progress bar and the value, shown when the
// volume / brightness changes or Wi-Fi / Bluetooth are toggled. Click-through, never focused.
Window {
  id: osd

  readonly property var cfg: Config.osd
  readonly property string position: cfg.position ?? "top_right"
  readonly property int offsetX: cfg.offset_x ?? 20
  readonly property int offsetY: cfg.offset_y ?? 8
  readonly property int barReserve: (Config.bar.margin_edge ?? 10) + (Config.bar.thickness ?? 34) + (Config.bar.margin_opposite_edge ?? 6)
  readonly property bool barTop: Config.bar.position !== "bottom"
  readonly property var targetScreen: Qt.application.screens[Panels.screenIndex] ?? Qt.application.screens[0]

  property string icon: "volume"
  property real value: 0
  property bool showProgress: true
  property string label: ""
  property bool muted: false

  readonly property int cardW: 38 * 6 + Style.spaceMd + Style.spaceSm + Style.spaceXs
  readonly property int cardH: 38 + Style.spaceSm
  readonly property int pad: 16

  // Ignore the first seconds: services report their initial state then, not user changes.
  property bool armed: false
  Timer {
    interval: 4000
    running: true
    onTriggered: {
      osd.armed = true;
      if (Shell.env("NOCTALIA_OSD_TEST").length > 0)
        osd.show("volume_output", "volume-high", 0.45, "45%");
    }
  }

  function show(kind, icon_, value_, label_, muted_) {
    if (!armed || !(cfg.enabled ?? true))
      return;
    const kinds = cfg.kinds ?? ({});
    if (kinds[kind] === false)
      return;
    icon = icon_;
    value = value_;
    showProgress = value_ >= 0;
    label = label_;
    muted = muted_ ?? false;
    visible = true;
    card.opacity = 1;
    hideTimer.restart();
  }

  flags: Qt.ToolTip | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.WindowTransparentForInput
         | Qt.WindowDoesNotAcceptFocus | Qt.NoDropShadowWindowHint
  color: "transparent"
  objectName: "osd"
  visible: false
  screen: targetScreen
  width: cardW + 2 * pad
  height: cardH + 2 * pad

  x: {
    const s = targetScreen;
    if (position.endsWith("left"))
      return s.virtualX + offsetX - pad;
    if (position.endsWith("center"))
      return s.virtualX + (s.width - width) / 2;
    return s.virtualX + s.width - width - offsetX + pad;
  }
  y: {
    const s = targetScreen;
    if (position.startsWith("bottom"))
      return s.virtualY + s.height - height - offsetY + pad - (barTop ? 0 : barReserve);
    return s.virtualY + offsetY - pad + (barTop ? barReserve : 0);
  }

  Timer {
    id: hideTimer
    interval: Math.max(250, osd.cfg.hide_delay_ms ?? 1400)
    onTriggered: fadeOut.restart()
  }

  NumberAnimation {
    id: fadeOut
    target: card
    property: "opacity"
    to: 0
    duration: Style.animNormal
    onFinished: osd.visible = false
  }

  Rectangle {
    id: card
    x: osd.pad
    y: osd.pad
    width: osd.cardW
    height: osd.cardH
    radius: height / 2
    color: Style.glassColor(Theme.colors.surface, osd.cfg.background_opacity ?? 0.97)
    border.width: (osd.cfg.border ?? true) ? Style.borderWidth : 0
    border.color: Theme.colors.outlineVariant

    // shadow
    Repeater {
      model: 6
      delegate: Rectangle {
        required property int index
        z: -1
        x: -index - 1
        y: -index
        width: card.width + 2 * (index + 1)
        height: card.height + 2 * (index + 1)
        radius: card.radius + index + 1
        color: Qt.rgba(0, 0, 0, 0.05 * (1 - index / 6))
      }
    }

    NGlyph {
      id: glyph
      anchors.left: parent.left
      anchors.leftMargin: Style.spaceMd + 2
      anchors.verticalCenter: parent.verticalCenter
      icon: osd.icon
      size: Style.fontSizeTitle + 4
      color: osd.muted ? Theme.colors.error : Theme.colors.primary
    }

    NProgress {
      id: bar
      visible: osd.showProgress
      anchors.left: glyph.right
      anchors.leftMargin: Style.spaceMd
      anchors.right: valueText.left
      anchors.rightMargin: Style.spaceMd
      anchors.verticalCenter: parent.verticalCenter
      height: Style.spaceXs + 2
      value: osd.muted ? 0 : osd.value
    }

    NText {
      id: valueText
      anchors.right: parent.right
      anchors.rightMargin: Style.spaceMd + 2
      anchors.verticalCenter: parent.verticalCenter
      width: osd.showProgress ? 40 : implicitWidth
      horizontalAlignment: osd.showProgress ? Text.AlignRight : Text.AlignLeft
      text: osd.label
      font.weight: Font.DemiBold
    }

    // Text-only popups (Wi-Fi on/off...) put the label next to the icon.
    NText {
      visible: !osd.showProgress
      anchors.left: glyph.right
      anchors.leftMargin: Style.spaceMd
      anchors.verticalCenter: parent.verticalCenter
      text: osd.label
      font.weight: Font.DemiBold
    }
  }

  // ── triggers ──────────────────────────────────────────────────────────────
  Connections {
    target: Audio
    function onVolumeChangedExternally() {
      const pct = Math.round(Audio.volume * 100);
      osd.show("volume_output", Audio.muted || pct === 0 ? "volume-mute" : Audio.volume < 0.4 ? "volume-low" : "volume-high",
               Audio.volume, Audio.muted ? "mudo" : pct + "%", Audio.muted);
    }
  }
  property bool lastMicMuted: Audio.micMuted
  property real lastMic: Audio.micVolume
  Connections {
    target: Audio
    function onMicChanged() {
      if (Audio.micMuted !== osd.lastMicMuted || Math.abs(Audio.micVolume - osd.lastMic) > 0.004) {
        osd.show("volume_input", Audio.micMuted ? "microphone-mute" : "microphone", Audio.micVolume,
                 Audio.micMuted ? "mudo" : Math.round(Audio.micVolume * 100) + "%", Audio.micMuted);
      }
      osd.lastMicMuted = Audio.micMuted;
      osd.lastMic = Audio.micVolume;
    }
  }
  Connections {
    target: Brightness
    function onChangedByUser() {
      osd.show("brightness", Brightness.value < 0.4 ? "brightness-low" : "brightness-high", Brightness.value,
               Math.round(Brightness.value * 100) + "%");
    }
  }
  property bool lastWifi: Wifi.enabled
  Connections {
    target: Wifi
    function onChanged() {
      if (Wifi.available && Wifi.enabled !== osd.lastWifi)
        osd.show("wifi", Wifi.enabled ? "wifi" : "wifi-off", -1, Wifi.enabled ? "Wi-Fi ligado" : "Wi-Fi desligado");
      osd.lastWifi = Wifi.enabled;
    }
  }
  property bool lastBt: Bluetooth.enabled
  Connections {
    target: Bluetooth
    function onChanged() {
      if (Bluetooth.available && Bluetooth.enabled !== osd.lastBt)
        osd.show("bluetooth", Bluetooth.enabled ? "bluetooth" : "bluetooth-off", -1,
                 Bluetooth.enabled ? "Bluetooth ligado" : "Bluetooth desligado");
      osd.lastBt = Bluetooth.enabled;
    }
  }
}
