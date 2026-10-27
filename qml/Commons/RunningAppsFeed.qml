pragma Singleton
import QtQuick
import Noctalia
QtObject {
  id:feed
  property int users:0
  function acquire(){users++;Dock.refresh();}
  function release(){users=Math.max(0,users-1);}
  property Timer refresh:Timer {
    interval:1000
    running:feed.users>0 && !(Config.dock.enabled ?? false)
    repeat:true
    onTriggered:Dock.refresh()
  }
}
