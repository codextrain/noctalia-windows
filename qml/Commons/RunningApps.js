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
            lookup[String(window.handle)]={appId:app.appId||"",icon:app.icon||"",name:app.name||"Aplicativo",title:window.title||app.name||"Janela"};
        }
    }
    return windows.map(function(w){
        var app=lookup[w.id] || {icon:"",name:"Aplicativo",title:"Janela " + w.number};
        return {id:w.id,appId:app.appId||"",icon:app.icon,name:app.name,title:app.title,active:w.active===true};
    });
}
// Pins are independent of the bottom dock and remain on every monitor/workspace.
function withPins(entries, pins, apps, info) {
    var result=[],seen={};
    (pins||[]).forEach(function(id){
        if(typeof id!=="string"||!id||seen[id])return;
        seen[id]=true;
        var app=(apps||[]).find(function(a){return a.appId===id;});
        var meta=info(id)||{};
        var current=entries.find(function(w){return w.appId===id&&w.active;})
            ||entries.find(function(w){return w.appId===id;});
        var window=current||(app&&app.windows||[])[0];
        result.push({id:window?String(window.id||window.handle):"",appId:id,pinned:true,
            title:meta.name||(app&&app.name)||id,icon:meta.icon||(app&&app.icon)||"",
            active:!!(current&&current.active)});
    });
    return result.concat(entries.filter(function(w){return !seen[w.appId];}));
}
