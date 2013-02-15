import QtQuick 1.1
import Sailfish.Silica 1.0
import org.nemomobile.accounts 1.0

Page {
    id: root

    property Item _accountSettings
    property Item _accountCreator
    property Item _contextMenu
    property string _accountToCreate

    function _reloadAccountSettings(isNewAccount, properties) {
        var comp = Qt.createComponent("AccountSettingsDialog.qml")
        if (comp.status !== Component.Ready) {
            throw new Error("Error creating account settings page: " + comp.errorString())
        }
        if (_accountSettings !== null) {
            _accountSettings.destroy()
        }
        _accountSettings = comp.createObject(root, properties)
        if (isNewAccount) {
            _accountSettings.rejected.connect(function() {
                _deleteAccount(_accountCreator.account)
            })
        }
        return _accountSettings
    }

    function _deleteAccount(accountObj) {
        if (accountObj !== null) {
            var identifiers = accountObj.identityIdentifiers
            for (var serviceName in identifiers) {
                var identityObj = identityManager.identity(identifiers[serviceName])
                if (identityObj) {
                    identityObj.remove()
                }
            }
            accountObj.remove()
        }
    }

    AccountModel {
        id: accountModel
    }

    AccountManager {
        id: accountManager
    }

    // This allows AccountSettingsDialog::accountId to be updated when AccountCreationDialog has
    // successfully saved an account and created a valid accountId.
    QtObject {
        id: accountIdRef
        property int accountId
    }
    Connections {
        target: root._accountCreator
        onAccountIdChanged: accountIdRef.accountId = root._accountCreator.accountId
    }

    Component {
        id: authDialogComponent

        Dialog {
            id: authDialog

            anchors.fill: parent
            acceptDestination: root._reloadAccountSettings(true,
                                   {"_accountIdRef": accountIdRef,
                                    "acceptDestination": root,
                                    "acceptDestinationAction": PageStackAction.Pop})
            acceptDestinationAction: PageStackAction.Push

            onStatusChanged: {
                if (status === PageStatus.Active && root._accountToCreate !== "") {
                    var provider = accountModel.provider(root._accountToCreate)
                    if (!provider) {
                        throw new Error("Unable to obtain provider with name: " + root._accountToCreate)
                    }
                    root._accountToCreate = ""
                    if (root._accountCreator !== null) {
                        root._accountCreator.destroy()
                        root._accountCreator = null
                    }
                    var componentFileName = "/usr/share/accounts/ui/" + provider.name + ".qml"
                    var comp = Qt.createComponent(componentFileName)
                    if (comp.status === Component.Ready) {
                        root._accountCreator = comp.createObject(authDialog, {
                                "dialog": authDialog,
                                "accountModel": accountModel,
                                "provider": provider
                            })
                        if (root._accountCreator === null) {
                            console.log("AccountsPage: cannot load instance of " + componentFileName + ":", comp.errorString())
                        }
                    } else {
                        console.log("AccountsPage: cannot load component file " + componentFileName + ":", comp.errorString())
                    }
                }
            }
        }
    }

    Component {
        id: contextMenuComponent

        ContextMenu {
            id: menu

            property int accountId
            property bool _removePending

            MenuItem {
                //: Removes a user account
                //% "Remove";
                text: qsTrId("components_accounts-me-remove_account")

                // delay deletion until menu is closed, otherwise deleting an accountsView item
                // while its menu is closing will confuse accountsView's total height calculation
                onClicked: menu._removePending = true
            }

            onVisibleChanged: {
                if (!visible && _removePending) {
                    var account = accountManager.account(accountId)
                    if (account !== null) {
                        account.remove()
                    }
                    _removePending = false
                }
            }
        }
    }

    SilicaListView {
        id: accountsView

        anchors.fill: parent
        model: AccountModel {}
        spacing: theme.paddingLarge
        header: PageHeader {
            //: accounts list view
            //% "Accounts"
            title: qsTrId("components_accounts-he-accounts_list")
        }

        delegate: Item {
            id: delegateItem

            width: ListView.view.width
            height: (root._contextMenu != null && root._contextMenu.parent === delegateItem)
                    ? root._contextMenu.height + contentItem.height
                    : contentItem.height

            BackgroundItem {
                id: contentItem

                height: theme.itemSizeSmall

                onClicked: {
                    pageStack.openDialog(root._reloadAccountSettings(false, {"accountId": model.accountId}))
                }

                onPressAndHold: {
                    if (!root._contextMenu) {
                        root._contextMenu = contextMenuComponent.createObject(accountsView, {"accountId": model.accountId})
                    } else {
                        root._contextMenu.accountId = model.accountId
                    }
                    root._contextMenu.show(delegateItem)
                }

                Image {
                    id: icon
                    x: theme.paddingLarge
                    anchors.verticalCenter: parent.verticalCenter
                    width: 64
                    height: 64
                    source: model.accountIcon
                }
                Label {
                    id: accountName
                    anchors.left: icon.right
                    anchors.leftMargin: theme.paddingLarge
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.verticalCenterOffset: model.accountDisplayName === "" ? 0 : -implicitHeight/2
                    text: model.providerDisplayName
                    color: contentItem.down ? theme.highlightColor : theme.primaryColor
                }
                Label {
                    anchors.left: icon.right
                    anchors.leftMargin: theme.paddingLarge
                    anchors.top: accountName.bottom
                    text: model.accountDisplayName
                    color: contentItem.down ? theme.secondaryHighlightColor : theme.secondaryColor
                }
            }
        }

        VerticalScrollDecorator {}

        PullDownMenu {
            MenuItem {
                //: Initiates adding a new account
                //% "Add Account";
                text: qsTrId("components_accounts-me-add_account")
                onClicked: {
                    var picker = pageStack.openDialog(
                                Qt.resolvedUrl("AccountPickerDialog.qml"),
                                {"acceptDestination": authDialogComponent,
                                 "acceptDestinationAction": PageStackAction.Replace})
                    picker.accepted.connect(function() {
                        root._accountToCreate = picker.selectedProvider
                    })
                }
            }
        }
    }
}
