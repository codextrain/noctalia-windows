import QtQuick
import Noctalia
import "../../Commons"
import "../../Widgets"
import "../../Settings"

// CPU, memory, disk and uptime.
Flickable {
  id: tab
  contentHeight: stats.height
  clip: true
  boundsBehavior: Flickable.StopAtBounds
  Column {
    id: stats
    width: tab.width - 18
    spacing: Style.spaceMd

  Component.onCompleted: SystemMonitor.acquire()
  Component.onDestruction: SystemMonitor.release()

  function gb(bytes) {
    return (bytes / 1073741824).toLocaleString(Qt.locale(), "f", 1) + " GB";
  }

  component Stat: NCard {
    id: stat
    property string icon
    property string label
    property string valueText
    property real fraction
    width: stats.width
    height: 86

    NGlyph {
      id: statIcon
      icon: stat.icon
      size: 22
      color: Theme.colors.primary
      anchors.left: parent.left
      anchors.top: parent.top
    }
    NText {
      anchors.left: statIcon.right
      anchors.leftMargin: Style.spaceSm
      anchors.verticalCenter: statIcon.verticalCenter
      text: stat.label
      font.weight: Font.Bold
    }
    NText {
      anchors.right: parent.right
      anchors.verticalCenter: statIcon.verticalCenter
      text: stat.valueText
      font.weight: Font.Normal
      color: Theme.colors.onSurfaceVariant
    }
    NProgress {
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.bottom: parent.bottom
      value: stat.fraction
      accent: stat.fraction > 0.9 ? Theme.colors.error : Theme.colors.primary
    }
  }

  // CPU with a small history graph
  NCard {
    width: stats.width
    height: 140

    NGlyph {
      id: cpuIcon
      icon: "cpu-usage"
      size: 22
      color: Theme.colors.primary
      anchors.left: parent.left
      anchors.top: parent.top
    }
    NText {
      anchors.left: cpuIcon.right
      anchors.leftMargin: Style.spaceSm
      anchors.verticalCenter: cpuIcon.verticalCenter
      text: "Processador"
      font.weight: Font.Bold
    }
    NText {
      anchors.right: parent.right
      anchors.verticalCenter: cpuIcon.verticalCenter
      text: Math.round(SystemMonitor.cpu) + "%"
      font.weight: Font.Normal
      color: Theme.colors.onSurfaceVariant
    }
    Row {
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.bottom: parent.bottom
      height: 70
      spacing: 2
      Repeater {
        model: SystemMonitor.cpuHistory
        delegate: Rectangle {
          required property var modelData
          width: (stats.width - 2 * Style.panelPadding - 39 * 2) / 40
          height: Math.max(2, 70 * modelData / 100)
          anchors.bottom: parent.bottom
          radius: 2
          color: Style.alpha(Theme.colors.primary, 0.35 + 0.65 * modelData / 100)
        }
      }
    }
  }

  Stat {
    icon: "memory"
    label: "Memória"
    valueText: stats.gb(SystemMonitor.memUsed) + " de " + stats.gb(SystemMonitor.memTotal)
    fraction: SystemMonitor.memTotal > 0 ? SystemMonitor.memUsed / SystemMonitor.memTotal : 0
  }
  Stat {
    icon: "storage"
    label: "Disco do sistema"
    valueText: stats.gb(SystemMonitor.diskUsed) + " de " + stats.gb(SystemMonitor.diskTotal)
    fraction: SystemMonitor.diskTotal > 0 ? SystemMonitor.diskUsed / SystemMonitor.diskTotal : 0
  }
  NCard {
    width: stats.width
    height: 56
    NGlyph {
      id: upIcon
      icon: "clock"
      size: 22
      color: Theme.colors.primary
      anchors.left: parent.left
      anchors.verticalCenter: parent.verticalCenter
    }
    NText {
      anchors.left: upIcon.right
      anchors.leftMargin: Style.spaceSm
      anchors.verticalCenter: parent.verticalCenter
      text: "Ligado há"
      font.weight: Font.Bold
    }
    NText {
      anchors.right: parent.right
      anchors.verticalCenter: parent.verticalCenter
      color: Theme.colors.onSurfaceVariant
      font.weight: Font.Normal
      text: {
        const s = SystemMonitor.uptime;
        const d = Math.floor(s / 86400), h = Math.floor((s % 86400) / 3600), m = Math.floor((s % 3600) / 60);
        return (d > 0 ? d + " dias, " : "") + h + "h " + m + "min";
      }
    }
  }

  NCard {
    width:stats.width
    height: shutdownControls.height + 2 * padding
    Column {
      id:shutdownControls
      width:parent.width
      spacing:Style.spaceSm
      property int hours:0
      property int minutes:30
      NSectionTitle {text:"Desligamento automático"}
      Row {
        spacing:Style.spaceMd
        NText {text:"Comandos por voz (PC)";height:32;verticalAlignment:Text.AlignVCenter}
        NToggle {checked:ShutdownTimer.voiceEnabled;onToggled:on=>ShutdownTimer.setVoiceEnabled(on)}
      }
      NText {
        width:parent.width;wrapMode:Text.WordWrap;font.pixelSize:Style.fontSizeCaption
        text:!ShutdownTimer.voiceEnabled ? "Voz desligada: microfone fechado." : ShutdownTimer.voiceStatus==="listening" ? "Ouvindo: PC, desligar em uma hora. PC, cancelar. PC, desativar comandos de voz." : ShutdownTimer.voiceStatus==="loading" ? "Preparando reconhecimento local…" : ShutdownTimer.voiceStatus==="error" ? "Não foi possível iniciar a voz. Confira o microfone e a permissão do Windows." : "Aguardando serviço de voz…"
        color:Theme.colors.onSurfaceVariant
      }

      NText {width:parent.width;wrapMode:Text.WordWrap;text:ShutdownTimer.active ? "Desligar em " + ShutdownTimer.countdown : "Escolha o tempo até desligar o PC."}
      Row {
        spacing:Style.spaceSm
        NText {text:"Horas";height:32;verticalAlignment:Text.AlignVCenter}
        NumberBox {value:shutdownControls.hours;from:0;to:24;onEdited:v=>shutdownControls.hours=v}
        NText {text:"Min";height:32;verticalAlignment:Text.AlignVCenter}
        NumberBox {value:shutdownControls.minutes;from:0;to:59;onEdited:v=>shutdownControls.minutes=v}
      }
      NListItem {
        width:parent.width; icon:"clock";title:ShutdownTimer.active ? "Substituir agendamento" : "Agendar desligamento"
        onClicked:ShutdownTimer.schedule(shutdownControls.hours*60+shutdownControls.minutes)
      }
      NListItem {
        width:parent.width;icon:"x";title:"Cancelar desligamento";visible:ShutdownTimer.active
        onClicked:ShutdownTimer.cancel()
      }
      NText {width:parent.width;wrapMode:Text.WordWrap;text:ShutdownTimer.message;color:Theme.colors.onSurfaceVariant;visible:text.length>0}
      NText {width:parent.width;wrapMode:Text.WordWrap;font.pixelSize:Style.fontSizeCaption;color:Theme.colors.onSurfaceVariant;text:"Funciona com o painel fechado. O Vitória precisa permanecer aberto. Salve seus arquivos: o Windows pode impedir o desligamento se algum programa pedir confirmação."}
    }
  }
  }
  ViewportScrollBar {target:tab;anchors.right:parent.right;anchors.top:parent.top;anchors.bottom:parent.bottom}
}
