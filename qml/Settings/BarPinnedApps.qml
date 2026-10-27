import QtQuick
import Noctalia
import "../Commons"
import "../Widgets"

SettingSection {
  id: root
  title: "Aplicativos fixados na barra"
  property string search: ""
  readonly property var pins: Config.bar.pinned_apps ?? []
  readonly property var results: {
    const count=Launcher.appCount;
    return Launcher.query(search,"all").filter(r=>r.kind==="app");
  }
  function toggle(id) {
    let next=pins.slice();
    const i=next.indexOf(id);
    if(i>=0)next.splice(i,1);else next.push(id);
    Config.setValue("bar.pinned_apps",next);
  }
  NText {
    width:parent.width;wrapMode:Text.Wrap
    text:"Escolha os programas que devem aparecer mesmo fechados. Clique no ícone da barra para abrir ou trazer a janela. Use o widget Aplicativos da área atual na posição desejada."
    color:Theme.colors.onSurfaceVariant;font.pixelSize:Style.fontSizeCaption
  }
  Repeater {
    model:root.pins
    delegate:NListItem {
      required property string modelData
      width:parent.width
      title:Launcher.appInfo(modelData).name||modelData
      icon:"pin";trailingIcon:"x"
      onClicked:root.toggle(modelData)
    }
  }
  TextBox {width:parent.width;value:root.search;placeholder:"Buscar um aplicativo para fixar…";onTextChanged:root.search=text}
  NText {visible:Launcher.loading;text:"Carregando aplicativos…";color:Theme.colors.onSurfaceVariant}
  ListView {
    width:parent.width;height:220;clip:true
    model:root.results
    boundsBehavior:Flickable.StopAtBounds
    delegate:NListItem {
      required property var modelData
      width:ListView.view.width
      title:modelData.title
      icon:"app-window"
      checked:root.pins.indexOf(modelData.id)>=0
      trailingIcon:checked?"check":"plus"
      onClicked:root.toggle(modelData.id)
    }
  }
}
