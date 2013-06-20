import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Silica.theme 1.0
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

    //: Enter username for account login action
    //% "Enter username"
    property string usernamePlaceholderText: qsTrId("components_accounts-ph-username")

    property string username
    property string password

    anchors.fill: parent
    canAccept: username !== "" && password !== ""

    Connections {
        target: root.postCreationDialog

        onStatusChanged: {
            // Start the account creation process when the next page becomes active, instead of
            // starting it when this page is accepted, to avoid synchronous account creation
            // processes interrupting the page transition animation.
            if (status === PageStatus.Active) {
                accountFactory.beginCreation()
            }
        }
    }

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
            root.accountCreationError()
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

            spacing: Theme.paddingLarge
            width: parent.width

            DialogHeader {
                dialog: root.dialog
            }

            Item {
                width: parent.width
                height: Theme.itemSizeSmall
                x: Theme.paddingLarge

                AccountIcon {
                    id: icon
                    anchors.verticalCenter: parent.verticalCenter
                    source: root.iconSource
                }
                Label {
                    anchors.left: icon.right
                    anchors.leftMargin: Theme.paddingLarge
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.accountProvider.displayName
                }
            }

            Text {
                x: Theme.paddingLarge
                width: parent.width - Theme.paddingLarge
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
