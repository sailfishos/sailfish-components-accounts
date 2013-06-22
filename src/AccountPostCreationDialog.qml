import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Silica.theme 1.0

AccountPostAuthenticationDialog {
    id: root

    // The settings page for the created account. It is automatically set as the acceptDestination
    // of this dialog.
    property Item settingsPage

    function accountCreationSucceeded(newAccountId) {
        // Set AccountSettingsDialog::accountId so that the settings page will load this new account
        if (settingsPage !== null
                && settingsPage.__sailfish_account_settings_dialog !== undefined) {
            settingsPage.accountId = newAccountId
        }
        authenticationDone(true)
    }

    function accountCreationFailed() {
        authenticationDone(false)
    }

    acceptDestination: settingsPage

    onAuthenticationFinished: {
        if (success) {
            canAccept = true
            accept()
        } else {
            // if there is a page after the settingsPage, make that the new acceptDestination
            if (settingsPage !== null && acceptDestination === settingsPage) {
                acceptDestination = settingsPage.acceptDestination
            }
            canAccept = (acceptDestination != null && acceptDestination != undefined)
        }
    }
}
