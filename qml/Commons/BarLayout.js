// Shared immutable layout operations. Empty arrays are intentional; null is corrupt.
var sections = ["start", "center", "end"];
var defaults = {
    start: ["launcher", "workspaces", "gpu-cat"],
    center: ["clock"],
    end: ["media", "tray", "notifications", "clipboard", "network", "bluetooth", "volume", "brightness", "battery", "control-center", "session"]
};
function validList(value) {
    return Array.isArray(value) && value.every(function(v) { return typeof v === "string" && v.length > 0; });
}
function read(config) {
    var source = config || {};
    // Compatibility with shipped binaries lacking setBarLayoutJson().
    if (typeof source.widget_layout_json === "string" && source.widget_layout_json.length) {
        try {
            var saved = JSON.parse(source.widget_layout_json);
            if (sections.every(function(s) { return validList(saved[s]); })) source = saved;
        } catch (e) { }
    }
    var result = {}, seen = {};
    sections.forEach(function(s) {
        result[s] = (validList(source[s]) ? source[s] : defaults[s]).filter(function(key) {
            if (seen[key]) return false;
            seen[key] = true; return true;
        });
    });
    return result;
}
function move(config, key, target, insertionIndex) {
    if (typeof key !== "string" || !key.length || (target !== "" && sections.indexOf(target) < 0)) return null;
    var layout = read(config), oldIndex = target ? layout[target].indexOf(key) : -1;
    var position = Math.max(0, Math.min(target ? layout[target].length : 0, Math.floor(insertionIndex || 0)));
    if (oldIndex >= 0 && oldIndex < position) position--;
    sections.forEach(function(s) { layout[s] = layout[s].filter(function(v) { return v !== key; }); });
    if (target) layout[target].splice(position, 0, key);
    return layout;
}
