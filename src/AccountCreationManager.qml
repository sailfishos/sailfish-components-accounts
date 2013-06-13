import QtQuick 2.0
import Sailfish.Silica 1.0
import "accountcreationmanager.js" as ManagerScript

Item {
    id: accountCreationManager

    property variant finalPageDestination

    function startAccountCreation() {
        ManagerScript.startAccountCreation()
    }

    function createSettingsPage(providerName, properties) {
        return ManagerScript.createSettingsPage(providerName, properties)
    }

    signal accountDeletionRequested(int accountId)
}
