import QtQuick 1.1
import com.jolla.components 1.0

Image {
    id: root

    property url iconImageUrl
    property string displayLabel

    source: "image://theme/graphic-header"
    height: 80

    Column {
        id: col
        spacing: 12
        anchors.fill: parent

        Image {
            id: img
            opacity: 0.5
            anchors.horizontalCenter: parent.horizontalCenter
            height: root.height - iconLabel.height
            fillMode: Image.PreserveAspectCrop
            clip: visible
            source: root.iconImageUrl
            asynchronous: true
            onStatusChanged: {
                if (status == Image.Error) {
                    visible = false
                } else if (status == Image.Ready) {
                    visible = true
                }
            }
        }

        HeadingLabel {
            id: iconLabel
            text: root.displayLabel
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width
            truncationMode: TruncationMode.Fade
        }
    }
}

