function forGlyph(icon) {
    if(icon.indexOf("weather-")===0)return "weather";
    if(icon.indexOf("volume")===0)return "volume";
    if(icon.indexOf("brightness")===0||icon==="sun")return "brightness";
    if(["network","network-off","world","wifi","wifi-off","wifi-1","wifi-2"].indexOf(icon)>=0)return "network";
    if(icon.indexOf("battery")===0)return "battery";
    if(icon.indexOf("bluetooth")===0)return "bluetooth";
    if(icon.indexOf("calendar")===0)return "calendar";
    if(["cpu-usage","cpu","memory"].indexOf(icon)>=0)return "hardware";
    if(["activity-heartbeat","activity"].indexOf(icon)>=0)return "system";
    if(["device-desktop","device-laptop"].indexOf(icon)>=0)return "monitor";
    if(["disc","player-play","player-pause"].indexOf(icon)>=0)return "disc";
    if(icon==="home")return "home";
    if(icon==="temperature")return "temperature";
    if(icon==="droplet")return "droplet";
    if(icon==="wind")return "wind";
    if(icon==="vitoria-cat")return "cat";
    if(icon==="search")return "search";
    if(icon==="shutdown")return "power";
    if(icon==="chevron-up")return "chevron";
    return "";
}
