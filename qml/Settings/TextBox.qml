import QtQuick
import Noctalia
import "../Commons"
import "../Widgets"

// Text field that saves on Enter or when it loses focus.
NTextInput {
  id: box
  property string value: ""
  signal edited(string value)
  width: 260
  height: 34
  icon: ""
  Component.onCompleted: text = value
  onValueChanged: if (!input.activeFocus) text = value
  onKeyPressed: event => {
    if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
      box.edited(text);
      event.accepted = true;
    }
  }
  Connections {
    target: box.input
    function onActiveFocusChanged() {
      if (!box.input.activeFocus && box.text !== box.value)
        box.edited(box.text);
    }
  }
}
