import QtQuick 1.1
import Sailfish.Silica 1.0
import org.nemomobile.accounts 1.0

Dialog {
    id: root

    property int accountId: 0

    onAccepted: {
        item.save()
    }

    width: parent.width

    sourceComponent: SilicaFlickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: contentColumn.height

        function save() {
            account.displayName = accountDisplayName.text
            account.sync()
        }

        function _populateSettingsModel() {
            serviceModel.clear()
            for (var i in account.supportedServiceNames) {
                var service = accountManager.service(account.supportedServiceNames[i])
                var serviceEnabled = false
                console.log("XXX TODO: for some reason, the enabledServiceNames for any account seems empty.... FIXME!")
                for (var j in account.enabledServiceNames) {
                    if (account.enabledServiceNames[j] === service.name) {
                        serviceEnabled = true
                        break
                    }
                }
                serviceModel.append({"name": service.name, "icon": service.iconName, "enabled": serviceEnabled})
            }
        }

        VerticalScrollDecorator {}

        PullDownMenu {
            MenuItem {
                //: Deletes the account
                //% "Delete Account";
                text: qsTrId("accounts-me-delete_account")
                onClicked: account.remove() // when removes successfully, will pop the page automatically.
            }
        }

        Account {
            id: account

            identifier: root.accountId

            onStatusChanged: {
                if (status === Account.Initialized) {
                    var provider = accountManager.provider(account.providerName)
                    if (provider) {
                        accountName.text = provider.displayName
                        accountIcon.source = provider.iconName
                    }
                    _populateSettingsModel()

                } else if (status === Account.Error) {
                    // display "error" dialog

                } else if (status === Account.Invalid) {
                    // successfully deleted
                    reject()
                }
            }
        }

        AccountManager {
            id: accountManager
        }

        ListModel {
            id: serviceModel
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

                    onCheckedChanged: account.enabled = checked
                }
            }

            Item {
                x: theme.paddingLarge
                width: 1
                height: accountDisplayNameLabel.height + accountDisplayName.height

                Label {
                    id: accountDisplayNameLabel
                    color: theme.secondaryColor

                    //: Short name or summary for a user account
                    //% "Account Identification"
                    text: qsTrId("components_accounts-la-settings_account_name")
                }

                TextField {
                    id: accountDisplayName
                    anchors.top: accountDisplayNameLabel.bottom
                    text: account.displayName
                    width: contentColumn.width - theme.paddingLarge*2

                    //: Placeholder text for short name or summary for a user account
                    //% "Enter account name"
                    placeholderText: qsTrId("components_accounts-ph-settings_account_name")
                }
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
                            anchors.right: parent.right
                            checked: model.enabled
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
