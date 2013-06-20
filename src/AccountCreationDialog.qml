import QtQuick 1.1
import Sailfish.Silica 1.0
import org.nemomobile.accounts 1.0

Dialog {
    id: root

    property Provider accountProvider

    // Once this dialog becomes active, this property is automatically set to an instance of
    // AccountPostCreationDialog.qml.
    property Item postCreationDialog

    // This should be emitted when the account is successfully created.
    signal accountCreated(int newAccountId)

    // This should be emitted when the account creation fails.
    signal accountCreationError()

    acceptDestination: postCreationDialog
    acceptDestinationAction: PageStackAction.Replace

    onAccountCreated: {
        if (postCreationDialog !== null) {
            postCreationDialog.accountCreationSucceeded(newAccountId)
        }
    }

    onAccountCreationError: {
        if (postCreationDialog !== null) {
            postCreationDialog.accountCreationFailed()
        } else {
            // this signal may be received before the page becomes active and postCreationDialog is set
            postCreationDialogChanged.connect(function() {
                if (postCreationDialogChanged !== null) {
                    postCreationDialog.accountCreationFailed()
                }
            })
        }
    }
}
