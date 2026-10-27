pragma Singleton
import QtQuick
import Noctalia

// Status text and the "refresh colors now" button for Settings → Appearance.
//
// Automatic updates need nothing here: WallpaperService notices when the wallpaper path,
// modification time or size changes (cheap file metadata, once per second) and Theme
// regenerates the palette for the new key. No image bytes are read from QML.
QtObject {
  id: root
  property bool refreshing: false
  property string lastError: ""
  property double lastRefresh: 0
  readonly property string statusText: lastError.length ? lastError
      : Theme.lastError.length ? "Não foi possível gerar as cores: " + Theme.lastError
      : Theme.generating ? "Atualizando as cores…"
      : "As cores acompanham o papel de parede automaticamente."
  signal refreshRequested()

  function forceRefresh() {
    if (Config.theme.source !== "wallpaper" || refreshing) return false;
    refreshing = true;
    refreshRequested();
    Theme.refreshWallpaperPalette();
    lastError = "";
    lastRefresh = Date.now();
    refreshing = false;
    return true;
  }
}
