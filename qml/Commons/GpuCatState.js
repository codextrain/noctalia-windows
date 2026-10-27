function read(text, now) {
    try {
        var data=JSON.parse(text);
        if(data.running!==true || data.available!==true || typeof data.updated_at_ms!=="number"
                || now-data.updated_at_ms>5000 || data.updated_at_ms>now+5000
                || typeof data.gpu_percent!=="number" || !isFinite(data.gpu_percent)
                || data.gpu_percent<0 || data.gpu_percent>100) return null;
        return {percent:data.gpu_percent,stamp:data.updated_at_ms};
    } catch(e) {return null;}
}
function next(previous, percent) {
    var smooth=previous.available ? previous.percent*0.6+percent*0.4 : percent;
    var running=previous.running ? smooth>8 : smooth>=12;
    return {available:true,percent:smooth,running:running,interval:running ? Math.max(40,Math.round(200-160*Math.sqrt(smooth/100))) : 650};
}
