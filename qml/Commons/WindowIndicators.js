function monitorKey(name) {
    return String(name || "").replace(/\\/g, "").replace(/\./g, "").toLowerCase();
}
function read(text, monitor, now, x, y) {
    try {
        var status = JSON.parse(text);
        if (!status.running || status.paused || !Array.isArray(status.windows)
                || typeof status.updated_at_ms !== "number" || now - status.updated_at_ms > 3000
                || status.updated_at_ms > now + 3000) return [];
        var key = monitorKey(monitor);
        if (!key) return [];
        return status.windows.filter(function(w) {
            var sameMonitor = monitorKey(w.monitor) === key
                || (typeof x === "number" && typeof y === "number" && w.monitor_x === x && w.monitor_y === y);
            return sameMonitor && typeof w.id === "string"
                && Number.isInteger(w.number) && w.number > 0;
        }).map(function(w) {
            return { id: w.id, number: w.number, active: w.active === true };
        });
    } catch (e) { return []; }
}
