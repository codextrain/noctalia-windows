import QtQuick
import Noctalia
import "../../Commons"
import "../../Widgets"
import "../../Commons/BrazilHolidays.js" as Holidays

// Month calendar (week starts on the locale's first day; today = primary).
Item {
  id: tab

  property int year: new Date().getFullYear()
  property int month: new Date().getMonth() // 0-based
  readonly property date today: new Date()
  readonly property int firstDow: Qt.locale().firstDayOfWeek % 7 // 0 = Sunday
  readonly property bool includeOptional: Config.shell.calendar_optional_dates ?? true
  readonly property var monthHolidays: Holidays.monthDates(year,month,includeOptional)
  property string selectedKey: ""
  function dateDescription(entry) { return entry.name + " · " + entry.detail; }
  NTooltip { id: holidayTip }
  Component.onDestruction: holidayTip.hide()

  function shift(delta) {
    holidayTip.hide(); selectedKey="";
    let m = month + delta;
    let y = year;
    while (m < 0) { m += 12; y -= 1; }
    while (m > 11) { m -= 12; y += 1; }
    month = m;
    year = y;
  }

  readonly property var cells: {
    const first = new Date(year, month, 1);
    const offset = (first.getDay() - firstDow + 7) % 7;
    const out = [];
    for (let i = 0; i < 42; ++i) {
      const d = new Date(year, month, 1 - offset + i);
      const dateKey=Holidays.key(d);
      const holiday=Holidays.monthDates(d.getFullYear(),d.getMonth(),includeOptional).find(h=>h.key===dateKey);
      out.push({ day: d.getDate(), inMonth: d.getMonth() === month,
                 key:dateKey, holiday:holiday||null,
                 isToday: d.toDateString() === today.toDateString() });
    }
    return out;
  }

  NCard {
    anchors.fill: parent

    Item {
      id: head
      width: parent.width
      height: 36
      NIconButton {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        icon: "chevron-left"
        onClicked: tab.shift(-1)
      }
      NText {
        anchors.centerIn: parent
        font.pixelSize: Style.fontSizeTitle
        font.weight: Font.Bold
        text: {
          const s = Qt.locale().standaloneMonthName(tab.month, Locale.LongFormat) + " " + tab.year;
          return s.charAt(0).toUpperCase() + s.slice(1);
        }
      }
      Row {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        spacing: Style.spaceXs
        NIconButton {
          icon: "calendar-event"
          onClicked: {
            tab.year = tab.today.getFullYear();
            tab.month = tab.today.getMonth();
          }
        }
        NIconButton { icon: "chevron-right"; onClicked: tab.shift(1) }
      }
    }

    Grid {
      id: grid
      anchors.top: head.bottom
      anchors.topMargin: Style.spaceMd
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.bottom: footer.top
      anchors.bottomMargin: Style.spaceSm
      columns: 7
      readonly property real cellW: width / 7
      readonly property real cellH: (height - 28) / 6

      Repeater {
        model: 7
        delegate: NText {
          required property int index
          width: grid.cellW
          height: 28
          horizontalAlignment: Text.AlignHCenter
          font.pixelSize: Style.fontSizeCaption
          font.weight: Font.Bold
          color: Theme.colors.onSurfaceVariant
          text: {
            const s = Qt.locale().dayName((tab.firstDow + index) % 7, Locale.ShortFormat).replace(".", "");
            return s.charAt(0).toUpperCase() + s.slice(1, 3);
          }
        }
      }
      Repeater {
        model: tab.cells
        delegate: Item {
          id: dayCell
          required property var modelData
          width: grid.cellW
          height: grid.cellH
          Rectangle {
            anchors.centerIn: parent
            width: Math.min(parent.width, parent.height) - 6
            height: width
            radius: width / 2
            color: modelData.isToday ? Theme.colors.primary : "transparent"
            border.width: modelData.key===tab.selectedKey ? 2 : 0
            border.color: Theme.colors.primary
          }
          NText {
            anchors.centerIn: parent
            text: modelData.day
            font.weight: modelData.isToday ? Font.Bold : Font.Normal
            color: modelData.isToday ? Theme.colors.onPrimary
                 : modelData.inMonth ? Theme.colors.onSurface : Style.alpha(Theme.colors.onSurfaceVariant, 0.5)
          }
          Rectangle {
            anchors.horizontalCenter:parent.horizontalCenter
            anchors.bottom:parent.bottom; anchors.bottomMargin:3
            width:5;height:5;radius:3
            visible:!!dayCell.modelData.holiday
            color:dayCell.modelData.holiday && dayCell.modelData.holiday.kind==="optional" ? Theme.colors.secondary : Theme.colors.primary
            opacity:dayCell.modelData.inMonth?1:0.4
          }
          MouseArea {
            anchors.fill:parent;hoverEnabled:true
            cursorShape:dayCell.modelData.holiday?Qt.PointingHandCursor:Qt.ArrowCursor
            onEntered:if(dayCell.modelData.holiday)holidayTip.show(dayCell,tab.dateDescription(dayCell.modelData.holiday),true)
            onExited:holidayTip.hide()
            onClicked:{holidayTip.hide();tab.selectedKey=dayCell.modelData.key;}
          }
        }
      }
    }
    Column {
      id:footer
      anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom
      spacing:Style.spaceXs
      Row {
        width:parent.width;spacing:Style.spaceSm
        NText {text:"Feriados do Brasil";font.weight:Font.Bold}
        NText {text:"Pontos facultativos";font.pixelSize:Style.fontSizeCaption;color:Theme.colors.onSurfaceVariant}
        NToggle {checked:tab.includeOptional;onToggled:on=>Config.setValue("shell.calendar_optional_dates",on)}
      }
      ListView {
        id:holidayList
        width:parent.width;height:tab.monthHolidays.length?Math.min(tab.monthHolidays.length*32,96):26
        clip:true;model:tab.monthHolidays
        boundsBehavior:Flickable.StopAtBounds
        delegate:Rectangle {
          required property var modelData
          width:holidayList.width;height:32;radius:Style.radiusSm
          color:modelData.key===tab.selectedKey?Style.alpha(Theme.colors.primary,0.16):"transparent"
          NText {
            anchors.fill:parent;anchors.leftMargin:6
            text:("0"+modelData.day).slice(-2)+" · "+modelData.name+(modelData.kind==="optional"?" — ponto facultativo":"")
            font.pixelSize:Style.fontSizeCaption
            color:modelData.kind==="optional"?Theme.colors.secondary:Theme.colors.onSurface
          }
          MouseArea {anchors.fill:parent;hoverEnabled:true;onEntered:holidayTip.show(parent,tab.dateDescription(parent.modelData),true);onExited:holidayTip.hide();onClicked:tab.selectedKey=parent.modelData.key}
        }
        NText {visible:tab.monthHolidays.length===0;text:"Nenhum feriado nacional neste mês.";font.pixelSize:Style.fontSizeCaption;color:Theme.colors.onSurfaceVariant}
      }
      NText {width:parent.width;text:"Datas locais não incluídas · pontos facultativos dependem do órgão/localidade";wrapMode:Text.Wrap;font.pixelSize:Style.fontSizeCaption;color:Theme.colors.onSurfaceVariant}
    }
  }
}
