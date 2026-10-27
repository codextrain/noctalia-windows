pragma Singleton

import QtQuick
import Noctalia

// Design constants copied from noctalia/src/ui/style.h so sizes match the Linux shell.
QtObject {
  id: styleRoot
  property var niriState: ({})
  property bool readingNiriState: false
  // The current Qt runtime paints white margins with system acrylic.
  // Preserve the configured shell backgrounds until a compatible renderer exists.
  readonly property bool glassActive: false
  readonly property real glassStrength: Math.max(0,Math.min(100,Config.windows.blur_intensity_percent ?? 50)) / 100
  function glassColor(color, opacity) { return alpha(color, opacity * (glassActive ? 1 - 0.65 * glassStrength : 1)); }
  function refreshNiriState() {
    const url = Shell.niriStatusUrl;
    if (!url.length || readingNiriState) return;
    readingNiriState = true;
    const request = new XMLHttpRequest();
    request.onreadystatechange = function() {
      if (request.readyState !== XMLHttpRequest.DONE) return;
      styleRoot.readingNiriState = false;
      try { styleRoot.niriState = JSON.parse(request.responseText); } catch(e) { styleRoot.niriState = ({}); }
    };
    try { request.open("GET",url); request.send(); } catch(e) { readingNiriState=false; niriState=({}); }
  }
  property Timer glassPoll: Timer { interval:250; running:true; repeat:true; triggeredOnStart:true; onTriggered: styleRoot.refreshNiriState() }
  readonly property real uiScale: Config.shell.ui_scale ?? 1.0
  readonly property real radiusScale: Config.shell.corner_radius_scale ?? 1.0

  readonly property real radiusSm: 3 * radiusScale
  readonly property real radiusMd: 6 * radiusScale
  readonly property real radiusLg: 9 * radiusScale
  readonly property real radiusXl: 12 * radiusScale

  readonly property real borderWidth: 1
  readonly property real spaceXs: 4
  readonly property real spaceSm: 8
  readonly property real spaceMd: 12
  readonly property real spaceLg: 16
  readonly property real panelPadding: 14

  readonly property real barCapsulePadding: 6
  readonly property real barCapsuleThickness: 0.76 // capsule height as a fraction of the bar
  readonly property real baseGlyphSize: 16

  readonly property real fontSizeMini: 11
  readonly property real fontSizeCaption: 13
  readonly property real fontSizeBody: 14
  readonly property real fontSizeTitle: 16
  readonly property real fontSizeHeader: 20

  readonly property string fontFamily: {
    const f = Config.shell.font_family ?? "";
    if (f.length > 0)
      return f;
    return Shell.isWindows ? "Segoe UI Variable Text" : "sans-serif";
  }

  // Animations: same durations as Noctalia (animFast/Normal/Slow), scaled by animation_speed.
  readonly property bool animationsEnabled: Config.shell.animation_enabled ?? true
  readonly property real animationSpeed: Math.max(0.1, Config.shell.animation_speed ?? 1.0)
  readonly property int animFast: animationsEnabled ? Math.round(100 / animationSpeed) : 0
  readonly property int animNormal: animationsEnabled ? Math.round(200 / animationSpeed) : 0
  readonly property int animSlow: animationsEnabled ? Math.round(400 / animationSpeed) : 0

  function alpha(c, a) {
    return Qt.rgba(c.r, c.g, c.b, c.a * a);
  }
}
