import QtQuick
import QtQuick.Window
import Noctalia
import "../Commons"

// An input-transparent desktop layer. It never captures a keyboard or mouse event.
Window {
  id: overlay
  required property var modelData
  screen: modelData
  transientParent: null
  flags: Qt.FramelessWindowHint | Qt.Tool | Qt.WindowStaysOnBottomHint | Qt.WindowDoesNotAcceptFocus | Qt.WindowTransparentForInput | Qt.NoDropShadowWindowHint
  title: "Noctalia Wallpaper Transition"
  color: "transparent"
  x: modelData.virtualX
  y: modelData.virtualY
  width: modelData.width
  height: modelData.height
  visible: false
  property bool animationActive: fade.running
  property bool ready: false
  readonly property string fill: Config.wallpaper.fill_mode ?? "crop"
  readonly property bool supported: ["crop","fit","stretch"].indexOf(fill)>=0
  readonly property bool enabled: supported && Style.animationsEnabled && (Config.wallpaper.transition_enabled ?? true)
  function remember() { oldImage.source=""; if(enabled) oldImage.source=Wallpaper.currentUrl; }
  function changed() {
    if (fade.running) return; // finish this fade toward the newest desktop image
    if (!enabled || oldImage.status!==Image.Ready) { remember(); return; }
    opacity=1;visible=true;lower();fade.restart();
  }
  onVisibleChanged: if(visible) lower()
  Component.onCompleted: { ready=true; remember(); }
  onEnabledChanged: if(ready) { if(!enabled) { fade.stop();visible=false; } remember(); }
  Image {
    id:oldImage
    anchors.fill:parent
    cache:false
    asynchronous:false
    sourceSize: Qt.size(overlay.width,overlay.height)
    fillMode:overlay.fill==="fit" ? Image.PreserveAspectFit : overlay.fill==="stretch" ? Image.Stretch : Image.PreserveAspectCrop
  }
  NumberAnimation { id:fade; target:overlay;property:"opacity";from:1;to:0;duration:350;easing.type:Easing.InOutQuad;onFinished:{overlay.visible=false;overlay.remember();} }
  Connections { target:Wallpaper;function onCurrentChanged(){overlay.changed();} }
}
