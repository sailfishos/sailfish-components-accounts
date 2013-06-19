import QtQuick 2.0
import Sailfish.Silica 1.0
import org.nemomobile.accounts 1.0

Dialog {
    id: root

    property Provider accountProvider

    // Once this dialog becomes active, this property is automatically set to the account's
    // settings page. If autoDirectToSettingsPage=true, this settings page will also be
    // automatically set as the acceptDestination.
    property Item settingsPage

    // If true, automatically sets acceptDestination to the settingsPage property when it becomes
    // available.
    property bool autoDirectToSettingsPage: true

    // This should be emitted when the account is successfully created.
    signal accountCreated(int newAccountId)


    acceptDestinationAction: PageStackAction.Replace

    onAccountCreated: {
        // Set AccountSettingsDialog::accountId so that the settings page will load this new account
        if (settingsPage !== null
                && settingsPage.__sailfish_account_settings_dialog !== undefined) {
            settingsPage.accountId = newAccountId
        }
    }

    onSettingsPageChanged: {
        if (settingsPage !== null && autoDirectToSettingsPage) {
            acceptDestination = settingsPage
        }
    }
}
