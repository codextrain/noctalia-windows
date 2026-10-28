import QtQuick
import Noctalia
import ".."
import "../../Commons"
import "../../Widgets"
BarWidget {
 id:root
 shown:ShutdownTimer.active || ShutdownTimer.barNotice.length>0
 tooltipText:ShutdownTimer.active ? "Desligamento agendado. Clique para ver ou cancelar." : ShutdownTimer.barNotice
 onClicked:Panels.open("control-center:system")
 NGlyph {icon:"clock";size:16;anchors.verticalCenter:parent.verticalCenter;color:ShutdownTimer.secondsLeft<=60 && ShutdownTimer.active ? Theme.colors.error : Theme.colors.primary}
 NText {
   objectName:"shutdown-countdown"
   anchors.verticalCenter:parent.verticalCenter
   text:ShutdownTimer.active ? "Desliga em " + ShutdownTimer.countdown : ShutdownTimer.barNotice
   font.pixelSize:Style.fontSizeCaption;font.weight:Font.DemiBold
   color:ShutdownTimer.active && ShutdownTimer.secondsLeft<=60 ? Theme.colors.error : Theme.colors.onSurface
 }
}
