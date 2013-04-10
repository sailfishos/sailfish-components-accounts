import QtQuick 1.1
import Sailfish.Silica 1.0
import org.nemomobile.accounts 1.0
import org.nemomobile.signon 1.0

Page {
    id: root

    property Item _accountSettings // saving settings changes is async, so we can't delete it immediately on accept/pop.
    property Item _accountCreator
    property Item _contextMenu
    property string _accountToCreate

    function selectedAccountToCreate(providerName) {
        root._accountToCreate = providerName
    }

    function _rejectAccountCreation() {
        root._deleteAccount(lastCreatedAccountRef.accountId)
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

    function _cleanUpAccountCreator() {
        if (_accountCreator !== null) {
            _accountCreator.destroy()
            _accountCreator = null
        }
        lastCreatedAccountRef.accountId = 0
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

    // This allows AccountSettings::accountId to be updated when AccountCreationDialog has
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
        id: settingsDialogComponent
        Dialog {
            id: settingsDialog
            anchors.fill: parent
            property bool _isNewAccount
            property string _providerName
            property variant _properties
            onStatusChanged: {
                if (status === PageStatus.Activating) {
                    if (_providerName == "") {
                        // assume constructed via new account.
                        _isNewAccount = true
                        _providerName = root._accountToCreate
                        _properties = { "_accountIdRef": lastCreatedAccountRef }
                    }

                    // construct the settings page, set me as its dialog.
                    var componentFileName = "/usr/share/accounts/ui/" + _providerName + "-settings.qml"
                    var comp = Qt.createComponent(componentFileName)
                    if (comp.status !== Component.Ready) {
                        // unable to create provider-specific settings page; create the default one instead
                        console.log("Unable to create provider-specific settings page: " + comp.errorString())
                        comp = Qt.createComponent("AccountSettings.qml")
                        if (comp.status !== Component.Ready) {
                            throw new Error("Error creating default account settings page: " + comp.errorString())
                        }
                    }

                    // delete the old settings page, if it exists.
                    if (_accountSettings !== null) {
                        _accountSettings.destroy()
                    }

                    // by default, we enable all services in a new account.
                    var modifiedProperties = _properties
                    modifiedProperties["_isNewAccount"] = _isNewAccount
                    modifiedProperties["dialog"] = settingsDialog
                    modifiedProperties["parent"] = settingsDialog // itemParent must be dialog so that it is rendered correctly
                    root._accountSettings = comp.createObject(root, modifiedProperties) // note: QObject parent must be root to avoid gc on dialog pop.
                    if (root._accountSettings === null) {
                        throw new Error("Error: cannot load instance of " + componentFileName + ":", comp.errorString())
                    }
                    if (_isNewAccount) {
                        _accountSettings.dialog.rejected.connect(root._rejectAccountCreation)
                    }
                }
            }
        }
    }

    Component {
        id: authDialogComponent

        Dialog {
            id: authDialog

            anchors.fill: parent
            acceptDestination: settingsDialogComponent
            acceptDestinationAction: PageStackAction.Replace

            onStatusChanged: {
                if (status === PageStatus.Activating) {
                    if (root._accountToCreate == "") {
                        // NOTE: if root._accountToCreate isn't set prior to this dialog being activated, we bail out.
                        throw new Error("Error: account creation page activated without provider name being set!")
                    }

                    var provider = accountModel.provider(root._accountToCreate)
                    if (!provider) {
                        throw new Error("Unable to obtain provider with name: " + root._accountToCreate)
                    }
                    var componentFileName = "/usr/share/accounts/ui/" + root._accountToCreate + ".qml"
                    var comp = Qt.createComponent(componentFileName)
                    root._cleanUpAccountCreator() // delete the old account creator page
                    if (comp.status === Component.Ready) {
                        root._accountCreator = comp.createObject(root, { // QObject parent must be root to avoid gc on dialog pop.
                                "dialog": authDialog,
                                "accountModel": accountModel,
                                "provider": provider,
                                "parent": authDialog // itemParent set to authDialog so it gets rendered correctly
                            })
                        if (root._accountCreator === null) {
                            throw new Error("Error: cannot load instance of " + componentFileName + ":", comp.errorString())
                        }
                    } else {
                        throw new Error("Error: cannot load component file " + componentFileName + ":", comp.errorString())
                    }
                }
            }
        }
    }

    Component {
        id: contextMenuComponent

        ContextMenu {
            id: menu

            signal removeAccount()

            MenuItem {
                //: Removes a user account
                //% "Remove";
                text: qsTrId("components_accounts-me-remove_account")
                onClicked: menu.removeAccount()
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

        delegate: ListItem {
            showMenuOnPressAndHold: false
            menu: contextMenuComponent

            onPressAndHold: {
                var menu = showMenu()
                menu.removeAccount.connect(function() {
                    //: Deleting this account in 5 seconds
                    //% "Removing account"
                    remorseAction(qsTrId("component_accounts-la-remove_account"),
                                  function() { root._deleteAccount(model.accountId) })
                })
            }

            onClicked: {
                pageStack.push(settingsDialogComponent.createObject(root, {
                                   "_isNewAccount": false,
                                   "_providerName": accountModel.provider(model.accountId).name,
                                   "_properties": { "accountId": model.accountId }
                               }))
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

        VerticalScrollDecorator {}

        PullDownMenu {
            MenuItem {
                //: Initiates adding a new account
                //% "Add Account";
                text: qsTrId("components_accounts-me-add_account")
                onClicked: {
                    root._accountToCreate = ""
                    var picker = pageStack.push(
                                Qt.resolvedUrl("AccountProviderPickerDialog.qml"),
                                {"acceptDestination": authDialogComponent,
                                 "acceptDestinationAction": PageStackAction.Replace})
                    picker.providerSelected.connect(root.selectedAccountToCreate)
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
