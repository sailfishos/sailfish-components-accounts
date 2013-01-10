import QtQuick 1.1
import com.jolla.components 1.0

BackgroundItem {
    id: root

    property url iconUrl
    property string topLabelText
    property string bottomLabelText
    property alias labelFont: fontProvider.font

    //--------------------
    
    height: childrenRect.height
    width: childrenRect.width

    // implementation datail... invisible text object
    Text {
        id: fontProvider
        visible: false
        font { // by default
            family: theme.fontFamilyHeading
            pixelSize: theme.fontSizeLarge
        }
    }

    Row {
        id: accountDelegateRow
        spacing: 12
        Image {
            source: root.iconUrl
            height: 80
            width: 80
            fillMode: Image.PreserveAspectFit
            y: 6 // XXX TODO: better way to do this?
        }

        Column {
            id: accountDelegateColumn
            visible: root.bottomLabelText != ""
            HeadingLabel {
                id: topLabel
                text: root.topLabelText
                font: root.labelFont
            }
            HeadingLabel {
                id: bottomLabel
                text: root.bottomLabelText
                color: theme.secondaryColor
                font: root.labelFont
            }
        }

        // if no bottom label is given, then only show top label text, centered wrt the image
        HeadingLabel {
            id: label
            visible: root.bottomLabelText == ""
            text: root.topLabelText
            anchors.verticalCenter: parent.verticalCenter
            font: root.labelFont
        }
    }
}
