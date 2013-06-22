import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Silica.theme 1.0
import Sailfish.Accounts 1.0
import "accountutil.js" as AccountUtil

AccountSettingsDialog {
    id: root

    function _populateSettingsModel() {
        serviceModel.clear()
        for (var i in account.supportedServiceNames) {
            var service = accountManager.service(account.supportedServiceNames[i])
            var serviceEnabled = false
            if (isNewAccount) {
                // enable all services for new accounts
                account.enableWithService(service.name)
                serviceEnabled = true
            } else {
                serviceEnabled = account.isEnabledWithService(service.name)
            }
            serviceModel.append({"name": service.name,
                                 "serviceType": service.serviceType,
                                 "iconName": service.iconName,
                                 "displayName": service.displayName,
                                 "enabled": serviceEnabled})
        }
    }

    onAccepted: {
        account.displayName = accountDisplayNameField.text
        account.sync()
    }

    Account {
        id: account

        identifier: root.accountId

        onStatusChanged: {
            if (status === Account.Initialized) {
                if (root.isNewAccount) {
                    account.enabled = true
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
        contentHeight: contentColumn.height + Theme.paddingLarge

        VerticalScrollDecorator {}

        PullDownMenu {
            MenuItem {
                //: Deletes the account
                //% "Delete Account"
                text: qsTrId("accounts-me-delete_account")
                onClicked: {
                    account.remove()
                }
            }
        }

        Column {
            id: contentColumn
            width: parent.width

            DialogHeader {
                //: Save the account settings
                //% "Save"
                acceptText: qsTrId("accounts-me-save")
            }

            MouseArea {
                id: accountHeadingItem
                width: parent.width
                height: Theme.itemSizeSmall

                onClicked: switchButton.checked = !switchButton.checked

                AccountIcon {
                    id: accountIcon
                    x: Theme.paddingLarge
                    anchors.verticalCenter: parent.verticalCenter
                    source: root.accountProvider.iconName
                }
                Label {
                    id: accountName
                    anchors.left: accountIcon.right
                    anchors.leftMargin: Theme.paddingLarge
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

            Item {
                width: 1
                height: Theme.paddingSmall
            }

            Column {
                width: parent.width
                spacing: Theme.paddingMedium
                opacity: account.enabled ? 1 : 0

                Behavior on opacity { FadeAnimation {} }

                Repeater {
                    model: serviceModel

                    MouseArea {
                        width: contentColumn.width
                        height: serviceIcon.height + serviceDescription.height

                        onClicked: serviceSwitch.checked = !serviceSwitch.checked

                        AccountIcon {
                            id: serviceIcon
                            x: Theme.paddingLarge
                            source: model.iconName
                        }
                        Label {
                            anchors {
                                left: serviceIcon.right
                                leftMargin: Theme.paddingLarge
                                right: serviceSwitch.left
                                verticalCenter: serviceIcon.verticalCenter
                            }
                            wrapMode: Text.WrapAnywhere
                            text: AccountUtil.serviceName(model.serviceType, model.displayName)
                        }
                        Switch {
                            id: serviceSwitch
                            anchors {
                                right: parent.right
                                verticalCenter: serviceIcon.verticalCenter
                            }
                            checked: model.enabled

                            onCheckedChanged: {
                                if (checked) {
                                    account.enableWithService(model.name)
                                } else {
                                    account.disableWithService(model.name)
                                }
                            }
                        }
                        Label {
                            id: serviceDescription
                            y: serviceIcon.height
                            anchors {
                                left: parent.left
                                leftMargin: Theme.paddingLarge
                                right: parent.right
                                rightMargin: Theme.paddingLarge
                            }
                            wrapMode: Text.Wrap
                            font.pixelSize: Theme.fontSizeExtraSmall
                            text: AccountUtil.serviceDescription(model.serviceType, root.accountProvider.displayName)
                        }
                    }
                }
            }
        }
    }
}
