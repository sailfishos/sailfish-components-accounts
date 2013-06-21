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
    property bool autoAcceptOnSuccess: true

    property bool _becameActive

    signal authenticationFinished(bool success)

    function authenticationDone(success) {
        if (!_becameActive || minimumBusyDurationTimer.running) {
            minimumBusyDurationTimer.triggered.connect(function() {
                authenticationDone(success)
            })
            return
        }
        canAccept = true
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
        header.opacity = 1
        busyIndicator.running = false
        infoColumn.opacity = 1
        authenticationFinished(success)
    }

    acceptDestinationAction: PageStackAction.Replace
    backNavigation: false
    canAccept: false

    onStatusChanged: {
        if (status === PageStatus.Active) {
            _becameActive = true
            minimumBusyDurationTimer.start()
        }
    }

    DialogHeader {
        id: header
        //: Skip the account creation process as there was an error.
        //% "Skip"
        title: qsTrId("components_accounts-he-skip")
        opacity: 0

        Behavior on opacity { FadeAnimation {} }
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

        Behavior on opacity { FadeAnimation {} }

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
