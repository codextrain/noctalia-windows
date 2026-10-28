pragma Singleton
import QtQuick
import Noctalia
import "ShutdownPolicy.js" as Policy
QtObject {
 id: timer
 property real currentTime: Date.now()
 property real deadline: 0
 readonly property bool active: deadline > 0
 readonly property int secondsLeft: active ? Policy.remaining(deadline,currentTime) : 0
 readonly property string countdown: Policy.format(secondsLeft)
 property string message: ""
 property string barNotice:""
 function announce(text){barNotice=text;noticeClear.restart();}
 property Timer noticeClear:Timer {id:noticeClear;interval:6000;onTriggered:timer.barNotice=""}

 property bool ready: false
 property string voiceStatus: "unavailable"
 property bool voiceReading:false
 property string voiceCommandId:""
 readonly property bool voiceEnabled:Config.windows.shutdown_voice_enabled ?? false
 function setVoiceEnabled(on) {
   if (!Config.setValue("windows.shutdown_voice_enabled",on)) {message="Não foi possível salvar a opção de voz.";return false;}
   message=on ? "Voz ativada. Diga PC antes do comando." : "Comandos por voz desativados.";
   return true;
 }
 function consumeVoice(state) {
   voiceStatus=Date.now()-Number(state.updatedAt || 0)<5000 ? state.status : "unavailable";
   const cmd=state.command;
   if (!cmd || cmd.id===voiceCommandId) return;
   voiceCommandId=cmd.id;
   if (!voiceEnabled || Date.now()-cmd.createdAt>5000 || cmd.createdAt>Date.now()) return;
   let accepted=false;
   if (cmd.action==="schedule") accepted=schedule(cmd.minutes);
   else if (cmd.action==="cancel") accepted=cancel();
   else if (cmd.action==="disable") {accepted=setVoiceEnabled(false);if(accepted)announce("Voz desativada");}
   // Only successful voice actions acknowledge to the helper; button actions stay silent.
   if(accepted)Config.setValue("windows.shutdown_voice_feedback_json",JSON.stringify({id:cmd.id,action:cmd.action,accepted:true,createdAt:Date.now()}));
 }
 function pollVoice() {
   const url=Shell.env("NOCTALIA_VOICE_STATE");
   if (!url || voiceReading) return;
   voiceReading=true;
   const req=new XMLHttpRequest();
   req.onreadystatechange=function(){
     if(req.readyState!==XMLHttpRequest.DONE)return;
     voiceReading=false;
     try{consumeVoice(JSON.parse(req.responseText));}catch(e){voiceStatus="unavailable";}
   };
   try{req.open("GET",url);req.send();}catch(e){voiceReading=false;voiceStatus="unavailable";}
 }
 property Timer voicePoll:Timer {interval:500;running:true;repeat:true;triggeredOnStart:true;onTriggered:timer.pollVoice()}

 signal shutdownRequested()
 function schedule(minutes) {
  const end=Policy.deadline(minutes,Date.now());
  if (!end) {message="Escolha entre 1 minuto e 24 horas.";return false;}
  if (!Config.setValue("windows.shutdown_deadline_ms",end)) {message="Não foi possível salvar o timer.";return false;}
  deadline=end;currentTime=Date.now();message="Desligamento agendado.";announce("Desligamento agendado");return true;
 }
 function cancel() {
  if (!Config.setValue("windows.shutdown_deadline_ms",0)) {message="Não foi possível cancelar o timer.";return false;}
  deadline=0;message="Timer cancelado.";announce("Timer cancelado");return true;
 }
 function check(now) {
  currentTime=now;
  if (!ready || !active || now < deadline) return;
  // Persist consumption before dispatch, so restarting cannot repeat shutdown.
  if (!Config.setValue("windows.shutdown_deadline_ms",0)) {message="Timer não executado: falha ao salvar.";return;}
  deadline=0;message="Desligamento solicitado ao Windows.";
  shutdownRequested();
 }
 Component.onCompleted: {
  // A deadline missed while the shell was closed must never cause surprise shutdown on launch.
  const saved=Number(Config.windows.shutdown_deadline_ms ?? 0);
  if (Number.isFinite(saved) && saved>Date.now() && saved<=Date.now()+86400000) deadline=saved;
  else if (saved>0) Config.setValue("windows.shutdown_deadline_ms",0);
  ready=true;
 }
 property Timer clock: Timer {interval:1000;running:timer.ready && timer.active;repeat:true;onTriggered:timer.check(Date.now())}
}
