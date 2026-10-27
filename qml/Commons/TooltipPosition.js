// Global desktop coordinates; each monitor can have a negative origin.
function position(px, py, width, height, below, gap, sx, sy, sw, sh, margin) {
    var minX = sx + margin, minY = sy + margin;
    var maxX = Math.max(minX, sx + sw - margin - width);
    var maxY = Math.max(minY, sy + sh - margin - height);
    var y = below ? py + gap : py - height - gap;
    if (below && y > maxY) y = py - height - gap;
    else if (!below && y < minY) y = py + gap;
    return {x: Math.round(Math.max(minX, Math.min(maxX, px - width / 2))),
            y: Math.round(Math.max(minY, Math.min(maxY, y)))};
}
