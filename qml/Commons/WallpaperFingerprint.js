// Bounded image-content signature: detects cache-file rewrites without decoding pixels.
function fingerprint(bytes) {
    if (!bytes || bytes.length < 16) return "";
    var hash = 2166136261;
    var step = Math.max(1, Math.floor(bytes.length / 16384));
    for (var i = 0; i < bytes.length; i += step) {
        hash ^= bytes[i];
        hash = Math.imul(hash, 16777619);
    }
    return bytes.length + ":" + (hash >>> 0).toString(16);
}
function observe(state, key) {
    if (!key) return {candidate:state.candidate,applied:state.applied,changed:false};
    if (!state.applied) return {candidate:key,applied:key,changed:false};
    if (key === state.applied) return {candidate:key,applied:state.applied,changed:false};
    if (key !== state.candidate) return {candidate:key,applied:state.applied,changed:false};
    return {candidate:key,applied:key,changed:true};
}
