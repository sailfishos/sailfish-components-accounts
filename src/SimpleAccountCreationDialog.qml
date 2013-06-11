import QtQuick 1.1
import Sailfish.Silica 1.0
import org.nemomobile.accounts 1.0
import org.nemomobile.signon 1.0
import Sailfish.Accounts.private 1.0

AccountCreationDialog {
    id: root

    property string iconSource: accountProvider.iconName
    property string description

    //: Username for account login action
    //% "Username"
    property string usernameLabel: qsTrId("components_accounts-la-username")
    property string usernamePlaceholderText

    property string username
    property string password

    canAccept: username !== "" && password !== ""

    onAccepted: {
        accountFactory.beginCreation()
    }

    anchors.fill: parent

    AccountModel {
        id: accountModel
    }

    AccountFactory {
        id: accountFactory

        function beginCreation() {
            var defaultServiceName = root.accountProvider.serviceNames[0]
            createAccount(root.accountProvider.name, defaultServiceName, root.username, root.password, root.username)
        }

        onError: {
            console.log("SimpleAccountCreationDialog error:", message)
        }

        onSuccess: {
            root.accountCreated(newAccountId)
        }
    }

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

                AccountIcon {
                    id: icon
                    anchors.verticalCenter: parent.verticalCenter
                    source: root.iconSource
                }
                Label {
                    anchors.left: icon.right
                    anchors.leftMargin: theme.paddingLarge
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.accountProvider.displayName
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
                EnterKey.iconSource: "image://theme/icon-m-enter-next"
                EnterKey.onClicked: passwordField.focus = true
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
                EnterKey.iconSource: "image://theme/icon-m-enter-next"
                EnterKey.onClicked: usernameField.focus = true
            }
        }
    }
}
