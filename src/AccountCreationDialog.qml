import QtQuick 1.1
import Sailfish.Silica 1.0
import org.nemomobile.accounts 1.0

Dialog {
    property Provider accountProvider
    property bool autoDirectToSettingsPage: true

    acceptDestinationAction: PageStackAction.Replace

    // This should be emitted when the account is successfully created.
    signal accountCreated(int newAccountId)

    // Triggered when the settings page for this newly created account has been loaded. Use it to
    // ensure the user can swipe forwards to the settings page when the creation page is accepted.
    // This is done automatically if autoDirectToSettingsPage=true.
    signal settingsPageLoaded(variant settingsPage)

    onAccountCreated: {
        // Set the settings page to load the settings for this account.
        if (acceptDestinationInstance !== null
                && acceptDestinationInstance.__sailfish_account_settings_dialog !== undefined) {
            acceptDestinationInstance.accountId = newAccountId
        }
    }

    onSettingsPageLoaded: {
        if (autoDirectToSettingsPage) {
            acceptDestination = settingsPage
        }
    }
}
