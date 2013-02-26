import QtQuick 1.1
import Sailfish.Silica 1.0
import org.nemomobile.accounts 1.0
import org.nemomobile.signon 1.0

Page {
    id: root

    property Item _accountSettings
    property Item _accountCreator
    property Item _contextMenu
    property string _accountToCreate

    function _reloadAccountSettings(isNewAccount, providerName, properties) {
        var componentFileName = "/usr/share/accounts/ui/" + providerName + "-settings.qml"
        var comp = Qt.createComponent(componentFileName)
        if (comp.status !== Component.Ready) {
            // unable to create provider-specific settings page; create the default one instead
            console.log("Unable to create provider-specific settings page: " + comp.errorString())
            comp = Qt.createComponent("AccountSettingsDialog.qml")
            if (comp.status !== Component.Ready) {
                throw new Error("Error creating default account settings page: " + comp.errorString())
            }
        }

        if (_accountSettings !== null) {
            _accountSettings.destroy()
        }

        // by default, we enable all services in a new account.
        var modifiedProperties = properties
        modifiedProperties["_isNewAccount"] = isNewAccount
        _accountSettings = comp.createObject(root, modifiedProperties)
        if (isNewAccount) {
            _accountSettings.rejected.connect(function() {
                _deleteAccount(lastCreatedAccountRef.accountId)
            })
        }
        return _accountSettings
    }

    function _deleteAccount(accountId) {
        var account = accountManager.account(accountId)
        if (account === null) {
            return
        }
        account.statusChanged.connect(function() {
            if (account.status === Account.Initialized) {
                var identifiers = account.identityIdentifiers
                for (var serviceName in identifiers) {
                    var identity = identityManager.identity(identifiers[serviceName])
                    if (identity) {
                        identity.remove()
                    }
                }
                account.remove()
            }
        })
    }

    function _cleanUpAccountCreation() {
        if (_accountCreator !== null) {
            _accountCreator.destroy()
            _accountCreator = null
        }
        lastCreatedAccountRef.accountId = 0
        _accountToCreate = ""
    }

    AccountModel {
        id: accountModel
    }

    AccountManager {
        id: accountManager
    }

    IdentityManager {
        id: identityManager
    }

    // This allows AccountSettingsDialog::accountId to be updated when AccountCreationDialog has
    // successfully saved an account and created a valid accountId.
    QtObject {
        id: lastCreatedAccountRef
        property int accountId
    }
    Connections {
        target: root._accountCreator
        onAccountIdChanged: lastCreatedAccountRef.accountId = root._accountCreator.accountId
    }

    Component {
        id: authDialogComponent

        Dialog {
            id: authDialog

            property string _providerName

            anchors.fill: parent
            acceptDestination: root._reloadAccountSettings(true, _providerName,
                                   {"_accountIdRef": lastCreatedAccountRef,
                                    "acceptDestination": root,
                                    "acceptDestinationAction": PageStackAction.Pop})
            acceptDestinationAction: PageStackAction.Replace

            onStatusChanged: {
                if (status === PageStatus.Active && root._accountToCreate !== "") {
                    var provider = accountModel.provider(root._accountToCreate)
                    if (!provider) {
                        throw new Error("Unable to obtain provider with name: " + root._accountToCreate)
                    }
                    _providerName = provider.name
                    var componentFileName = "/usr/share/accounts/ui/" + _providerName + ".qml"
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
                    root._deleteAccount(accountId)
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
                    pageStack.openDialog(root._reloadAccountSettings(false,
                            accountModel.provider(model.accountId).name,
                            {"accountId": model.accountId}))
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
                    root._cleanUpAccountCreation()
                    var picker = pageStack.openDialog(
                                Qt.resolvedUrl("AccountProviderPickerDialog.qml"),
                                {"acceptDestination": authDialogComponent,
                                 "acceptDestinationAction": PageStackAction.Replace})
                    picker.accepted.connect(function() {
                        root._accountToCreate = picker.selectedProvider
                    })
                }
            }
        }

        ViewPlaceholder {
            enabled: accountsView.count == 0

            //% "Pull down to add accounts"
            text: qsTrId("components_accounts-he-pull_down_to_add_account")
        }
    }
}
