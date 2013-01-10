import QtQuick 1.1
import com.jolla.components 1.0
import org.nemomobile.accounts 1.0
import org.nemomobile.signon 1.0

Page {
    id: root

    property Provider provider
    property int accountId: 0

    property bool _isNewAccount: accountId == 0

    AccountHeader {
        id: header
        width: parent.width

        displayLabel: provider.displayName
        iconImageUrl: provider.iconName

        // Left icon - cancel edit
        ToolIcon {
            iconSource: "image://theme/icon-header-cancel"
            height: parent.height

            y: parent.y - 15        // Batteling implicit margins.
            anchors {
                left: parent.left
                leftMargin: 10
            }

            onClicked: {
                pageStack.pop()
            }

        }

        // Right icon - save edit
        ToolIcon {
            iconSource: "image://theme/icon-header-accept"
            height: parent.height

            y: parent.y - 15        // Batteling implicit margins.
            anchors {
                right: parent.right
                rightMargin: 10
            }

            onClicked: {
                saveAccount()
            }
        }
    }

    JollaFlickable {
        id: flick

        anchors {
            top: header.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            margins: 20
        }

        Column {
            id: col
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 20

            Label {
                id: usernameLabel
                //: google account editor
                //% "User Name"
                text: qsTrId("components_accounts-google_account_editor-user_name")
                anchors.right: parent.right
            }

            TextField {
                id: usernameText
                width: root.width
                //: google account editor
                //% "username"
                placeholderText: qsTrId("components_accounts-google_account_editor-username_ph")
                property bool hasChanged: false
                onTextChanged: {
                    if (text != "" && !hasChanged)
                        hasChanged = true
                    if (hasChanged)
                        ident.userName = text
                }
            }

            Label {
                id: passwordLabel
                //: google account editor
                //% "Password"
                text: qsTrId("components_accounts-google_account_editor-password")
                anchors.right: parent.right
            }

            TextField {
                id: passwordText
                width: root.width
                echoMode: TextInput.PasswordEchoOnEdit
                //: google account editor
                //% "password"
                placeholderText: qsTrId("components_accounts-google_account_editor-password_ph")
                property bool hasChanged: false
                onTextChanged: {
                    if (text != "" && !hasChanged)
                        hasChanged = true
                    if (hasChanged)
                        ident.secret = text
                }
            }

            Label {
                id: captionLabel
                //: google account editor
                //% "Caption"
                text: qsTrId("components_accounts-google_account_editor-caption")
                anchors.right: parent.right
            }

            TextField {
                id: captionText
                width: root.width
                //: google account editor
                //% "caption"
                placeholderText: qsTrId("components_accounts-google_account_editor-caption_ph")
                property bool hasChanged: false
                onTextChanged: {
                    if (text != "" && !hasChanged)
                        hasChanged = true
                    if (hasChanged)
                        ident.caption = text
                }
            }
        }
    }

    function saveAccount() {
        // we actually save the identity first.
        ident.setMethodMechanisms("password", ["password"])
        ident.realms = ["gmail.com", "youtube.com", "google.com"]
        ident.sync()
    }

    function cleanup() {
        if (_isNewAccount) {
            // error occurred, new account, attempt to remove everything we added.
            ident.remove()
            account.remove()
        }
    }

    Account {
        id: account
        identifier: root.accountId
        providerName: root.accountId != 0 ? "" : provider.name

        onStatusChanged: {
            if (status == Account.Initialized && !_isNewAccount) {
                usernameText.text = account.displayName // we save the username in the display name.  XXX TODO: check this is correct, with mardy.
                ident.identifier = account.identityIdentifier("google-talk") // if zero, will create new identity.
            } else if (status == Account.Synced) {
                pageStack.pop() // Success!
            } else if (status == Account.Error) {
                // display "error" dialog
                cleanup();
                pageStack.pop() // Failed.
            }
        }
    }

    Identity {
        id: ident
        identifier: root.accountId ? account.identityIdentifier("google-talk") : 0
        identifierPending: root.accountId != 0

        onStatusChanged: {
            if (status == Identity.Initialized) {
                usernameText.text = userName
                passwordText.text = secret
                captionText.text = caption
            } else if (status == Identity.Synced) {
                account.displayName = usernameText.text // XXX TODO: ensure this is correct...
                account.enableAccountWithService("google-talk")
                account.setIdentityIdentifier(ident.identifier, "google-talk")
                // XXX TODO: other services (gmail, picasa, g+, ...)
                account.sync()
            } else if (status == Identity.Error) {
                // display "error" dialog
                cleanup();
                pageStack.pop() // Failed.
            }
        }
    }
}
