import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Silica.theme 1.0

Dialog {
    id: root

    // The settings page for the created account. It is automatically set as the acceptDestination
    // of this dialog.
    property Item settingsPage

    property alias progressStatusText: statusLabel.text
    property alias errorHeading: errorHeadingLabel.text
    property alias errorDescription: errorDetailLabel.text

    property int minimumBusyDuration: 2000

    property bool _becameActive

    function accountCreationSucceeded(newAccountId) {
        // Set AccountSettingsDialog::accountId so that the settings page will load this new account
        if (settingsPage !== null
                && settingsPage.__sailfish_account_settings_dialog !== undefined) {
            settingsPage.accountId = newAccountId
        }
        if (!_becameActive || minimumBusyDurationTimer.running) {
            minimumBusyDurationTimer.triggered.connect(_proceed)
        } else {
            _proceed()
        }
    }

    function accountCreationFailed() {
        if (!_becameActive || minimumBusyDurationTimer.running) {
            minimumBusyDurationTimer.triggered.connect(_showError)
        } else {
            _showError()
        }
    }

    function _proceed() {
        canAccept = true
        accept()
    }

    function _showError() {
        // if there is a page after the settingsPage, make that the new acceptDestination
        if (settingsPage !== null) {
            acceptDestination = settingsPage.acceptDestination
            if (acceptDestination) {
                canAccept = true
                header.opacity = 1
            }
        }
        backNavigation = true
        busyIndicator.running = false
        errorInfo.opacity = 1
    }

    acceptDestination: settingsPage
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
        id: errorInfo

        anchors.top: header.bottom
        x: Theme.paddingLarge
        width: parent.width - Theme.paddingLarge*2
        spacing: Theme.paddingLarge
        opacity: 0

        Behavior on opacity { FadeAnimation {} }

        Label {
            id: errorHeadingLabel
            width: parent.width
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeHuge
            color: Theme.highlightColor

            //: Heading displayed when an account cannot be created.
            //% "Oops, something went wrong"
            text: qsTrId("components_accounts-he-account_creation_error")
        }

        Label {
            id: errorDetailLabel
            width: parent.width
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeExtraSmall
            //: Description displayed when an account cannot be created.
            //% "Go back to try again or skip now and add this account later."
            text: qsTrId("components_accounts-la-account_creation_error")
        }
    }
}
