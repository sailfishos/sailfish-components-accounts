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
        width: parent.width
        spacing: 12
        Image {
            id: accountIcon
            source: root.iconUrl
            height: 80
            width: 80
            fillMode: Image.PreserveAspectFit
            y: 6 // XXX TODO: better way to do this?
        }

        Column {
            id: accountDelegateColumn
            width: parent.width - accountIcon.width
            visible: root.bottomLabelText != ""
            HeadingLabel {
                id: topLabel
                text: root.topLabelText
                color: theme.primaryColor
                font: root.labelFont
                width: parent.width
                truncationMode: TruncationMode.Fade
            }
            HeadingLabel {
                id: bottomLabel
                text: root.bottomLabelText
                color: theme.secondaryColor
                font: root.labelFont
                width: parent.width
                truncationMode: TruncationMode.Fade
            }
        }

        // if no bottom label is given, then only show top label text, centered wrt the image
        HeadingLabel {
            id: label
            visible: root.bottomLabelText == ""
            text: root.topLabelText
            color: theme.primaryColor
            anchors.verticalCenter: parent.verticalCenter
            font: root.labelFont
            width: parent.width - accountIcon.width
            truncationMode: TruncationMode.Fade
        }
    }
}
