import QtQuick 1.1
import com.jolla.components 1.0

Image {
    id: root

    property url providerIconImageUrl
    property string providerDisplayName

    source: "image://theme/graphic-header"
    height: 147

    Column {
        id: col
        spacing: 12
        anchors.fill: parent

        Image {
            id: img
            opacity: 0.5
            anchors.horizontalCenter: parent.horizontalCenter
            height: root.height - providerNameLabel.height
            width: 120
            fillMode: Image.PreserveAspectCrop
            clip: visible
            source: root.providerIconImageUrl
            onStatusChanged: {
                if (status == Image.Error)
                    visible = false
                else if (status == Image.Ready)
                    visible = true
            }
        }

        Label {
            id: providerNameLabel

            text: root.providerDisplayName

            anchors.horizontalCenter: parent.horizontalCenter
            color: theme.highlightColor
            font {
                family: theme.fontFamilyHeading
                pixelSize: theme.fontSizeLarge
            }
        }
    }
}

