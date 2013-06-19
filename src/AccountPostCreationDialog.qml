import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Silica.theme 1.0

Dialog {
    function accountCreationSucceeded(nextPage) {
        acceptDestination = nextPage
        if (!busyIndicator.running || minimumBusyDuration.running) {
            minimumBusyDuration.triggered.connect(_proceed)
        } else {
            _proceed()
        }
    }

    function accountCreationFailed(nextPage) {
        acceptDestination = nextPage
        if (!busyIndicator.running || minimumBusyDuration.running) {
            minimumBusyDuration.triggered.connect(_showError)
        } else {
            _showError()
        }
    }

    function _proceed() {
        canAccept = true
        accept()
    }

    function _showError() {
        canAccept = true
        busyIndicator.running = false
        header.opacity = 1
        errorInfo.opacity = 1
    }

    backNavigation: false
    canAccept: false
    acceptDestinationAction: PageStackAction.Replace

    onStatusChanged: {
        if (status === PageStatus.Active) {
            busyIndicator.running = true
            minimumBusyDuration.start()
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

    BusyIndicator {
        id: busyIndicator
        anchors.centerIn: parent
        size: BusyIndicatorSize.Large
    }

    Timer {
        id: minimumBusyDuration
        interval: 2000
    }

    Column {
        id: errorInfo

        anchors.top: header.bottom
        x: Theme.paddingLarge
        width: parent.width - Theme.paddingLarge*2
        spacing: Theme.paddingLarge
        opacity: 0

        Behavior on opacity { FadeAnimation {} }

        Label {
            width: parent.width
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeHuge
            color: Theme.highlightColor

            //: Heading displayed when an account cannot be created.
            //% "Oops, something went wrong"
            text: qsTrId("components_accounts-he-account_creation_error")
        }

        Label {
            width: parent.width
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeExtraSmall
            //: Description displayed when an account cannot be created.
            //% "Go back to try again or skip now and add this account later."
            text: qsTrId("components_accounts-la-account_creation_error")
        }
    }
}
