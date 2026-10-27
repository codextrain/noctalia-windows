function monitorKey(name) {
    return String(name || "").replace(/\\/g, "").replace(/\./g, "").toLowerCase();
}
function available(text, now) {
    try {
        var status=JSON.parse(text);
        return status.running === true && status.paused !== true && Array.isArray(status.windows)
            && typeof status.updated_at_ms === "number" && Math.abs(now-status.updated_at_ms)<=3000;
    } catch(e) {return false;}
}
function view(text,monitor,now,x,y) {
    if(!available(text,now)) return {niri:false,entries:[]};
    var entries=read(text,monitor,now,x,y);
    if(!entries.length) entries=[{id:"",number:1,active:true,count:0,empty:true}];
    return {niri:true,entries:entries};
}
function read(text, monitor, now, x, y) {
    try {
        var status = JSON.parse(text);
        if (!status.running || status.paused || !Array.isArray(status.windows)
                || typeof status.updated_at_ms !== "number" || now - status.updated_at_ms > 3000
                || status.updated_at_ms > now + 3000) return [];
        var key = monitorKey(monitor);
        if (!key) return [];
        var windows = status.windows.filter(function(w) {
            var sameMonitor = monitorKey(w.monitor) === key
                || (typeof x === "number" && typeof y === "number" && w.monitor_x === x && w.monitor_y === y);
            return sameMonitor && typeof w.id === "string"
                && Number.isInteger(w.number) && w.number > 0;
        });
        var entries = [];
        windows.forEach(function(w) {
            var grouped = Number.isInteger(w.workspace) && w.workspace > 0;
            var number = grouped ? w.workspace : w.number;
            var entry = grouped ? entries.find(function(e) { return e.number === number; }) : null;
            if (!entry) {
                entry = { id: w.id, number: number, active: false, count: 0 };
                entries.push(entry);
            }
            entry.count++;
            if (w.spotlight === true) {entry.spotlight=true;entry.id=w.id;}
            if (w.active === true) { entry.active = true; if(!entry.spotlight) entry.id = w.id; }
        });
        return entries.sort(function(a,b) { return a.number - b.number; });
    } catch (e) { return []; }
}
