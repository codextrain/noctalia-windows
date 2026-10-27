import QtQuick
import Noctalia
import "../Commons"

// Noctalia "card": surface_container fill, outline_variant border, radius_xl, 14px padding.
// Children go inside `contentItem` area (anchors.fill + margins already applied).
Rectangle {
  id: root

  default property alias content: inner.data
  property real padding: Style.panelPadding
  readonly property alias contentItem: inner

  radius: Style.radiusXl
  color: Theme.colors.surfaceContainer
  border.width: Style.borderWidth
  border.color: Theme.colors.outlineVariant

  Item {
    id: inner
    anchors.fill: parent
    anchors.margins: root.padding
  }
}
