import QtQuick 1.1
import Sailfish.Silica 1.0
import org.nemomobile.accounts 1.0

Dialog {
    id: root

    property int accountId: _accountIdRef === null ? 0 : _accountIdRef.accountId
    property QtObject _accountIdRef
    property Account _account

    function _populateSettingsModel() {
        if (_account === null) {
            return
        }
        serviceModel.clear()
        for (var i in _account.supportedServiceNames) {
            var service = accountManager.service(_account.supportedServiceNames[i])
            var serviceEnabled = false
            for (var j in _account.enabledServiceNames) {
                if (_account.enabledServiceNames[j] === service.name) {
                    serviceEnabled = true
                    break
                }
            }
            serviceModel.append({"name": service.name, "icon": service.iconName, "enabled": serviceEnabled})
        }
    }

    anchors.fill: parent

    onAccountIdChanged: {
        if (_account !== null) {
            _account.destroy()
        }
        _account = accountComponent.createObject(root, {"identifier": accountId})
    }

    onAccepted: {
        if (_account) {
            _account.displayName = accountDisplayNameField.text
            _account.sync()
        }
    }

    // Use this to delay Account creation until we have a valid identifier.
    // XXX fix Account type to delay loading until its identifier is set.
    Component {
        id: accountComponent

        Account {
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
                    if (root._account) {
                        root._account.remove()
                    }
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
                    checked: root._account !== null && root._account.enabled

                    onCheckedChanged: {
                        if (root._account) {
                            root._account.enabled = checked
                        }
                    }
                }
            }

            TextField {
                id: accountDisplayNameField
                text: root._account !== null ? root._account.displayName : ""
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
                            anchors.right: parent.right
                            checked: model.enabled
                            onCheckedChanged: {
                                if (root._account !== null) {
                                    if (checked) {
                                        root._account.enableWithService(model.name)
                                    } else {
                                        root._account.disableWithService(model.name)
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
