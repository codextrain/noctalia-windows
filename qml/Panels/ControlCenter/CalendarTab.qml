import QtQuick
import Noctalia
import "../../Commons"
import "../../Widgets"

// Month calendar (week starts on the locale's first day; today = primary).
Item {
  id: tab

  property int year: new Date().getFullYear()
  property int month: new Date().getMonth() // 0-based
  readonly property date today: new Date()
  readonly property int firstDow: Qt.locale().firstDayOfWeek % 7 // 0 = Sunday

  function shift(delta) {
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
      out.push({ day: d.getDate(), inMonth: d.getMonth() === month,
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
      anchors.bottom: parent.bottom
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
          required property var modelData
          width: grid.cellW
          height: grid.cellH
          Rectangle {
            anchors.centerIn: parent
            width: Math.min(parent.width, parent.height) - 6
            height: width
            radius: width / 2
            color: modelData.isToday ? Theme.colors.primary : "transparent"
          }
          NText {
            anchors.centerIn: parent
            text: modelData.day
            font.weight: modelData.isToday ? Font.Bold : Font.Normal
            color: modelData.isToday ? Theme.colors.onPrimary
                 : modelData.inMonth ? Theme.colors.onSurface : Style.alpha(Theme.colors.onSurfaceVariant, 0.5)
          }
        }
      }
    }
  }
}
