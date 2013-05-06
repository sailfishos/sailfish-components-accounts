import QtQuick 1.1
import Sailfish.Silica 1.0
import org.nemomobile.accounts 1.0

Item {
    id: root
    anchors.fill: parent

    property Dialog dialog // provided by AccountsPage.qml
    property int accountId: _accountIdRef === null ? 0 : _accountIdRef.accountId
    property QtObject _accountIdRef
    property bool _isNewAccount: false

    function _populateSettingsModel() {
        serviceModel.clear()
        for (var i in account.supportedServiceNames) {
            var service = accountManager.service(account.supportedServiceNames[i])
            var serviceEnabled = false
            for (var j in account.enabledServiceNames) {
                if (account.enabledServiceNames[j] === service.name) {
                    serviceEnabled = true
                    break
                }
            }
            serviceModel.append({"name": service.name, "icon": service.iconName, "enabled": serviceEnabled})
        }
    }

    Connections {
        target: dialog
        onAccepted: {
            account.displayName = accountDisplayNameField.text
            account.sync()
        }
    }

    Account {
        id: account

        identifier: root.accountId

        onStatusChanged: {
            if (status === Account.Initialized) {
                var provider = accountManager.provider(providerName)
                if (provider) {
                    accountName.text = provider.displayName
                    accountIcon.source = provider.iconName
                }
                root._populateSettingsModel()
            } else if (status === Account.Synced) {
                // success
            } else if (status === Account.Error) {
                // display "error" dialog
            } else if (status === Account.Invalid) {
                // successfully deleted
                if (root.status == PageStatus.Active) {
                    root.reject()
                }
            }
        }
    }

    AccountManager {
        id: accountManager
    }

    ListModel {
        id: serviceModel
    }

    SilicaFlickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: contentColumn.height

        VerticalScrollDecorator {}

        PullDownMenu {
            MenuItem {
                //: Deletes the account
                //% "Delete Account";
                text: qsTrId("accounts-me-delete_account")
                onClicked: {
                    account.remove()
                }
            }
        }

        Column {
            id: contentColumn
            width: parent.width
            spacing: theme.paddingLarge

            DialogHeader {
                //: Save the account settings
                //% "Save"
                acceptText: qsTrId("accounts-me-save")
            }

            Item {
                id: accountHeadingItem
                width: parent.width
                height: theme.itemSizeSmall

                Image {
                    id: accountIcon
                    x: theme.paddingLarge
                    anchors.verticalCenter: parent.verticalCenter
                    width: 64
                    height: 64
                }
                Label {
                    id: accountName
                    anchors.left: accountIcon.right
                    anchors.leftMargin: theme.paddingLarge
                    anchors.verticalCenter: parent.verticalCenter
                }
                Switch {
                    id: switchButton
                    anchors.right: parent.right
                    checked: account.enabled

                    onCheckedChanged: {
                        account.enabled = checked
                    }
                }
            }

            TextField {
                id: accountDisplayNameField
                text: account.displayName
                width: parent.width

                //: Short name or summary for a user account
                //% "Account Identification"
                label: qsTrId("components_accounts-la-settings_account_name")

                //: Placeholder text for short name or summary for a user account
                //% "Enter account name"
                placeholderText: qsTrId("components_accounts-ph-settings_account_name")
            }

            Column {
                width: parent.width

                Repeater {
                    model: serviceModel

                    Item {
                        width: contentColumn.width
                        height: theme.itemSizeSmall

                        Image {
                            id: serviceIcon
                            x: theme.paddingLarge
                            anchors.verticalCenter: parent.verticalCenter
                            width: 64
                            height: 64
                            source: model.icon
                        }
                        Label {
                            anchors.left: serviceIcon.right
                            anchors.leftMargin: theme.paddingLarge
                            anchors.verticalCenter: parent.verticalCenter
                            text: model.name
                        }
                        Switch {
                            property bool isInitialised: false
                            anchors.right: parent.right
                            checked: initialiseChecked()

                            function initialiseChecked() {
                                if (!isInitialised) {
                                    isInitialised = true
                                    if (root._isNewAccount) {
                                        account.enableWithService(model.name)
                                        return true
                                    } else if (model.enabled) {
                                        return true
                                    }
                                }

                                return false
                            }

                            onCheckedChanged: {
                                if (checked) {
                                    account.enableWithService(model.name)
                                } else {
                                    account.disableWithService(model.name)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
