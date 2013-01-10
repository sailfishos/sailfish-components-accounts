import QtQuick 1.1
import com.jolla.components 1.0
import com.jolla.components.accounts 1.0
import org.nemomobile.accounts 1.0
import org.nemomobile.signon 1.0

// JabberID / password, optional server address
AccountCreationPage {
    id: root

    // implementation details.
    property string __defaultServiceName: provider.serviceNames[0]
    property bool __isNewAccount: accountId == 0

    AccountHeader {
        id: header
        width: parent.width

        displayLabel: provider.displayName
        iconImageUrl: provider.iconName

        // Left icon - cancel edit
        ToolIcon {
            iconSource: "image://theme/icon-header-cancel"
            height: parent.height

            y: parent.y - 15        // Battling implicit margins.
            anchors {
                left: parent.left
                leftMargin: 10
            }

            onClicked: cancel()

        }

        // Right icon - save edit
        ToolIcon {
            iconSource: "image://theme/icon-header-accept"
            height: parent.height

            y: parent.y - 15        // Battling implicit margins.
            anchors {
                right: parent.right
                rightMargin: 10
            }

            onClicked: saveAccount()
        }
    }

    JollaFlickable {
        id: flick
        contentHeight: childrenRect.height

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
                //: jabber account editor
                //% "Jabber ID"
                text: qsTrId("components_accounts-jabber_account_editor-jabber_id")
                anchors.right: parent.right
            }

            TextField {
                id: usernameText
                width: root.width
                //: jabber account editor
                //% "JabberID"
                placeholderText: qsTrId("components_accounts-jabber_account_editor-jabberid_ph")
                property bool hasChanged: false
                onTextChanged: {
                    if (text != "" && !hasChanged) {
                        hasChanged = true
                    }
                    if (hasChanged) {
                        _ident.userName = text
                    }
                }
            }

            Label {
                id: passwordLabel
                //: jabber account editor
                //% "Password"
                text: qsTrId("components_accounts-jabber_account_editor-password")
                anchors.right: parent.right
            }

            TextField {
                id: passwordText
                width: root.width
                echoMode: TextInput.PasswordEchoOnEdit
                //: jabber account editor
                //% "password"
                placeholderText: qsTrId("components_accounts-jabber_account_editor-password_ph")
                property bool hasChanged: false
                onTextChanged: {
                    if (text != "" && !hasChanged) {
                        hasChanged = true
                    }
                    if (hasChanged) {
                        _ident.secret = text
                    }
                }
            }
        }
    }

    function saveAccount() {
        // we actually save the identity first.
        _ident.sync()
    }

    function cleanup() {
        if (__isNewAccount) {
            // error occurred, new account, attempt to remove everything we added.
            _ident.remove()
            _account.remove()
        }
    }

    property Account _account: Account {
        identifier: root.accountId
        providerName: root.accountId != 0 ? "" : provider.name

        onStatusChanged: {
            if (status == Account.Initialized && !__isNewAccount) {
                usernameText.text = _account.displayName // we save the username in the display name.
                _ident.identifier = _account.identityIdentifier(__defaultServiceName) // if zero, will create new identity.
            } else if (status == Account.Synced) {
                // success
                success()
            } else if (status == Account.Error) {
                // display "error" dialog
                console.log("ERROR creating jabber account!")
                cleanup()
                root.failure()
            }
        }
    }

    property Identity _ident: Identity {
        identifier: root.accountId ? _account.identityIdentifier(__defaultServiceName) : 0
        identifierPending: root.accountId != 0

        onStatusChanged: {
            if (status == Identity.Initialized) {
                usernameText.text = userName
                passwordText.text = secret
            } else if (status == Identity.Synced) {
                _account.displayName = usernameText.text // XXX TODO: ensure this is correct...
                for (var i in provider.serviceNames) {
                    _account.enableWithService(provider.serviceNames[i])
                    _account.setIdentityIdentifier(_ident.identifier, provider.serviceNames[i])
                }
                _account.sync()
            } else if (status == Identity.Error) {
                // display "error" dialog
                console.log("ERROR creating jabber identity!")
                cleanup()
                failure()
            }
        }
    }
}
