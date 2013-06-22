import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Silica.theme 1.0

Dialog {
    id: root

    property string successHeading

    //: Heading displayed when an account cannot be created.
    //% "Oops, something went wrong"
    property string errorHeading: qsTrId("components_accounts-he-account_creation_error")

    property string successDescription

    //: Description displayed when an account cannot be created.
    //% "Go back to try again or skip now and add this account later."
    property string errorDescription: qsTrId("components_accounts-la-account_creation_error")

    property alias progressStatusText: statusLabel.text
    property int minimumBusyDuration: 2000

    property bool becameTopPage

    signal authenticationFinished(bool success)

    function authenticationDone(success) {
        if (minimumBusyDurationTimer.running) {
            minimumBusyDurationTimer.triggered.connect(function() {
                authenticationDone(success)
            })
            return
        }
        backNavigation = true
        if (success) {
            headingLabel.text = successHeading
            headingLabel.color = Theme.primaryColor
            descriptionLabel.text = successHeading
        } else {
            headingLabel.text = errorHeading
            headingLabel.color = Theme.highlightColor
            descriptionLabel.text = errorDescription
        }
        busyIndicator.running = false
        infoColumn.opacity = 1
        if (becameTopPage) {
            authenticationFinished(success)
        } else {
            becameTopPageChanged.connect(function() {
                authenticationFinished(success)
            })
        }
    }

    acceptDestinationAction: PageStackAction.Replace
    backNavigation: false
    canAccept: false

    Connections {
        target: pageStack
        onCurrentPageChanged: {
            if (!root.becameTopPage && root === pageStack.currentPage) {
                root.becameTopPage = true
                minimumBusyDurationTimer.start()
            }
        }
    }

    DialogHeader {
        id: header
        //: Skip the account creation process as there was an error.
        //% "Skip"
        title: qsTrId("components_accounts-he-skip")
        opacity: root.canAccept ? 1 : 0

        Behavior on opacity {
            enabled: root.minimumBusyDuration > 0
            FadeAnimation {}
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: Theme.paddingLarge
        opacity: busyIndicator.opacity

        Label {
            id: statusLabel
            width: root.width - Theme.paddingLarge*2
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap

            //: Notifies user that the account is currently being created.
            //% "Creating account..."
            text: qsTrId("components_accounts-la-creating_account")
        }

        BusyIndicator {
            id: busyIndicator
            anchors.horizontalCenter: parent.horizontalCenter
            size: BusyIndicatorSize.Large
            running: true
        }
    }

    Timer {
        id: minimumBusyDurationTimer
        interval: root.minimumBusyDuration
    }

    Column {
        id: infoColumn

        anchors.top: header.bottom
        x: Theme.paddingLarge
        width: parent.width - Theme.paddingLarge*2
        spacing: Theme.paddingLarge
        opacity: 0

        Behavior on opacity {
            enabled: root.minimumBusyDuration > 0
            FadeAnimation {}
        }

        Label {
            id: headingLabel
            width: parent.width
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeHuge
        }

        Label {
            id: descriptionLabel
            width: parent.width
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeExtraSmall
        }
    }
}
