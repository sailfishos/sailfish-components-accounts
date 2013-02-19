import QtQuick 1.1
import Sailfish.Silica 1.0
import org.nemomobile.accounts 1.0
import org.nemomobile.signon 1.0

AccountAuthenticator {
    id: root

    property string name: provider.displayName
    property string iconSource: provider.iconName
    property string description

    //: Username for account login action
    //% "Username"
    property string usernameLabel: qsTrId("components_accounts-la-username")
    property string usernamePlaceholderText

    property string username
    property string password

    account: Account {
        identifier: root.accountId
        providerName: root.accountId != 0 ? "" : provider.name

        onStatusChanged: {
            if (status === Account.Initialized && !root.__isNewAccount) {
                usernameField.text = account.displayName // we save the username in the display name.
                identity.identifier = account.identityIdentifier(root.__defaultServiceName) // if zero, will create new identity.
            } else if (status === Account.Synced) {
                // success
                root.accountId = account.identifier
            } else if (status === Account.Error) {
                // XXX display "error" dialog?
                console.log("SimpleAccountAuthenticator account error:", errorMessage)
                root.dialog.reject()
            }
        }
    }

    property Identity identity: Identity {
        identifier: root.accountId ? account.identityIdentifier(root.__defaultServiceName) : 0
        identifierPending: root.accountId != 0

        onStatusChanged: {
            if (status === Identity.Initialized) {
                usernameField.text = userName
                passwordField.text = secret
            } else if (status === Identity.Synced) {
                account.displayName = usernameField.text
                for (var i in provider.serviceNames) {
                    account.enableWithService(provider.serviceNames[i])
                    account.setIdentityIdentifier(identity.identifier, provider.serviceNames[i])
                }
                account.sync()
            } else if (status === Identity.Error) {
                // XXX display "error" dialog?
                console.log("SimpleAccountAuthenticator identity error:", errorMessage)
                root.dialog.reject()
            }
        }
    }

    // implementation details.
    property string __defaultServiceName: provider.serviceNames[0]
    property bool __isNewAccount: accountId == 0

    anchors.fill: parent

    SilicaFlickable {
        id: flickable

        anchors.fill: parent
        contentHeight: contentColumn.height

        Column {
            id: contentColumn

            spacing: theme.paddingLarge
            width: parent.width

            DialogHeader {
                dialog: root.dialog
            }

            Item {
                width: parent.width
                height: theme.itemSizeSmall
                x: theme.paddingLarge

                Image {
                    id: icon
                    width: 64
                    height: 64
                    anchors.verticalCenter: parent.verticalCenter
                    source: root.iconSource
                }
                Label {
                    anchors.left: icon.right
                    anchors.leftMargin: theme.paddingLarge
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.name
                }
            }

            Text {
                x: theme.paddingLarge
                width: parent.width - theme.paddingLarge
                text: root.description
            }

            TextField {
                id: usernameField
                width: parent.width
                placeholderText: root.usernamePlaceholderText
                label: root.usernameLabel
                onTextChanged: root.username = text
                Keys.onReturnPressed: passwordField.focus = true
            }

            TextField {
                id: passwordField
                width: parent.width
                inputMethodHints: Qt.ImhNoPredictiveText
                echoMode: TextInput.Password

                //: Password for account login action
                //% "Password"
                label: qsTrId("components_accounts-la-password")

                //: Placeholder text for password of account login
                //% "Enter password"
                placeholderText: qsTrId("components_accounts-ph-password")
                onTextChanged: root.password = text
                Keys.onReturnPressed: flickable.focus = true
            }
        }
    }
}
