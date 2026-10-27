pragma Singleton
import QtQuick
import Noctalia
import "WallpaperFingerprint.js" as Fingerprint

QtObject {
  id: root
  property bool reading: false
  property bool refreshing: false
  property var observation: ({candidate:"",applied:""})
  property string lastError: ""
  property double lastRefresh: 0
  readonly property string statusText: lastError.length ? lastError : Theme.generating ? "Atualizando as cores…" : "As cores acompanham o papel de parede automaticamente."
  signal refreshRequested()
  // Useful for isolated software tests; the normal runtime always applies changes.
  property bool applyChanges: true

  function forceRefresh() {
    if (Config.theme.source !== "wallpaper" || refreshing) return false;
    refreshing = true;
    refreshRequested();
    if (!applyChanges) { refreshing=false; return true; }
    if (typeof Theme.refreshWallpaperPalette === "function") {
      Theme.refreshWallpaperPalette();
      lastRefresh=Date.now();refreshing=false;return true;
    }
    const desired = Config.theme.wallpaper_scheme ?? "vibrant";
    const temporary = desired === "m3-tonal-spot" ? "m3-content" : "m3-tonal-spot";
    // Installed binaries have no cache-invalidation API. Two synchronous scalar
    // writes invalidate their key while restoring the user's selected scheme.
    const first = Config.setValue("theme.wallpaper_scheme", temporary);
    const second = Config.setValue("theme.wallpaper_scheme", desired);
    lastError = first && second ? "" : "Não foi possível atualizar as cores. Tente novamente.";
    lastRefresh = Date.now();
    refreshing = false;
    return first && second;
  }
  function inspect(url) {
    if (reading || !url.length) return;
    reading=true;
    const request=new XMLHttpRequest();
    request.responseType="arraybuffer";
    request.onreadystatechange=function() {
      if (request.readyState !== XMLHttpRequest.DONE) return;
      root.reading=false;
      if (request.status !== 0 && request.status !== 200) { root.lastError="Não foi possível ler o papel de parede atual."; return; }
      const bytes=new Uint8Array(request.response);
      const signature=Fingerprint.fingerprint(bytes);
      if (!signature.length) { root.lastError="O papel de parede está indisponível ou ainda está sendo gravado."; return; }
      root.lastError="";
      const next=Fingerprint.observe(root.observation,url+"|"+signature);
      // Wait for a stable file and avoid queueing work while the generator is busy.
      if (next.changed && Theme.generating) return;
      root.observation=next;
      if (next.changed) root.forceRefresh();
    };
    try { request.open("GET",url); request.send(); }
    catch(e) { reading=false; lastError="Não foi possível ler o papel de parede atual."; }
  }
  function poll() {
    if (Config.theme.source !== "wallpaper") { observation=({candidate:"",applied:""}); repairBuiltin(); return; }
    inspect(Wallpaper.currentUrl);
  }
  function repairBuiltin() {
    if (refreshing || Config.theme.source !== "builtin" || Theme.generating || Theme.palette !== "Papel de parede") return;
    const desired=Config.theme.builtin ?? "Noctalia";
    refreshing=true;
    Config.setValue("theme.builtin",desired === "Noctalia" ? "Nord" : "Noctalia");
    Config.setValue("theme.builtin",desired);
    refreshing=false;
  }
  property Timer watcher: Timer { interval:1000; running:true; repeat:true; triggeredOnStart:true; onTriggered:root.poll() }
  property Connections wallpaperChanges: Connections { target:Wallpaper; function onCurrentChanged(){root.poll();} }
  property Connections themeChanges: Connections { target:Theme; function onChanged(){if(Config.theme.source === "builtin" && !Theme.generating && Theme.palette === "Papel de parede") Qt.callLater(root.repairBuiltin);} }
}
