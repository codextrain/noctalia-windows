// Match the current per-monitor NiriWin workspace to Windows' real application icons.
function current(state, apps, monitor, now, x, y) {
    if (!state || !state.running || state.paused || !Array.isArray(state.windows)
            || typeof state.updated_at_ms !== "number" || Math.abs(now-state.updated_at_ms)>3000) return [];
    function key(s) { return String(s||"").replace(/[\\.]/g,"").toLowerCase(); }
    var windows=state.windows.filter(function(w){
        return typeof w.id==="string" && (key(w.monitor)===key(monitor)
            || (typeof x==="number" && typeof y==="number" && w.monitor_x===x && w.monitor_y===y));
    });
    if(!windows.length) return [];
    function group(w){return Number.isInteger(w.workspace) ? w.workspace : w.number;}
    var selected=windows.find(function(w){return w.active;}) || windows[0];
    windows=windows.filter(function(w){return group(w)===group(selected);});
    var lookup={};
    for(var i=0;i<(apps||[]).length;i++) {
        var app=apps[i];
        for(var j=0;j<(app.windows||[]).length;j++) {
            var window=app.windows[j];
            lookup[String(window.handle)]={icon:app.icon||"",name:app.name||"Aplicativo",title:window.title||app.name||"Janela"};
        }
    }
    return windows.map(function(w){
        var app=lookup[w.id] || {icon:"",name:"Aplicativo",title:"Janela " + w.number};
        return {id:w.id,icon:app.icon,name:app.name,title:app.title,active:w.active===true};
    });
}
