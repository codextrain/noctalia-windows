pragma Singleton
import QtQuick
import Noctalia
QtObject {
 readonly property string disableUrl: Shell.env("NOCTALIA_DESKTOP_CONTROL")
 readonly property bool available: disableUrl.length > 0
 function disable() {
  if (!available) return;
  Shell.openUri(disableUrl);
 }
}
