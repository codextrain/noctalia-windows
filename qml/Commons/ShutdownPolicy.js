// Shared deadline policy; never executes OS commands.
function deadline(minutes, now) {
  if (!Number.isFinite(minutes) || !Number.isInteger(minutes) || minutes < 1 || minutes > 1440 || !Number.isFinite(now)) return 0;
  return now + minutes * 60000;
}
function remaining(deadline, now) { return Math.max(0, Math.ceil((deadline - now) / 1000)); }
function format(seconds) {
  const h=Math.floor(seconds/3600),m=Math.floor(seconds%3600/60),s=seconds%60;
  return [h,m,s].map(x=>String(x).padStart(2,"0")).join(":");
}
if (typeof module !== "undefined") module.exports={deadline,remaining,format};
