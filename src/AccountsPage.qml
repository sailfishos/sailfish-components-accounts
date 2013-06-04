import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Silica.theme 1.0
import org.nemomobile.accounts 1.0
import org.nemomobile.signon 1.0

Page {
    id: root

    // When an account creation or settings dialog is created, we keep a reference to them here
    // because they call Account::sync() which is asynchronous and we need to make sure the dialog
    // is not deleted (and thus deleting their account child) before the operation is complete.
    property Item _accountCreator
    property Item _accountSettings

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

    function _createAccount(providerName) {
        var provider = accountModel.provider(providerName)
        if (!provider) {
            throw new Error("Unable to obtain provider with name: " + providerName)
        }
        var props = {
            "accountProvider": provider,
            "acceptDestination": _createSettingsPage(providerName, {}),
            "acceptDestinationAction": PageStackAction.Replace,
            "acceptDestinationProperties": {"isNewAccount": true}
        }
        pageStack.replace(_createAccountCreationPage(providerName, props))
    }

    function _createSettingsPage(providerName, properties) {
        var componentFileName = "/usr/share/accounts/ui/" + providerName + "-settings.qml"
        var comp = Qt.createComponent(componentFileName)
        if (comp.status !== Component.Ready) {
            comp = Qt.createComponent(Qt.resolvedUrl("AccountSettings.qml"))
        }
        if (root._accountSettings !== null) {
            root._accountSettings.destroy()
        }
        var obj = comp.status === Component.Ready
                ? comp.createObject(root, properties)
                : null
        obj.rejected.connect(function() {
            if (obj === root._accountSettings && obj.isNewAccount) {
                root._deleteAccount(obj.accountId)
            }
        })
        root._accountSettings = obj
        return root._accountSettings
    }

    function _createAccountCreationPage(providerName, properties) {
        var componentFileName = "/usr/share/accounts/ui/" + providerName + ".qml"
        var comp = Qt.createComponent(componentFileName)
        if (comp.status !== Component.Ready) {
            throw new Error("Unable to load account creation page "
                            + componentFileName + ": " + comp.errorString())
        }
        if (root._accountCreator !== null) {
            root._accountCreator.destroy()
        }
        root._accountCreator = comp.status === Component.Ready
                ? comp.createObject(root, properties)
                : null
        return root._accountCreator
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

    SilicaListView {
        id: accountsView

        anchors.fill: parent
        model: accountModel
        header: PageHeader {
            //: accounts list view
            //% "Accounts"
            title: qsTrId("components_accounts-he-accounts_list")
        }

        delegate: ListItem {
            contentHeight: Theme.itemSizeMedium
            menu: Component {
                ContextMenu {
                    MenuItem {
                        //: Removes a user account
                        //% "Remove";
                        text: qsTrId("components_accounts-me-remove_account")
                        onClicked: removeAccount()
                    }
                }
            }

            function removeAccount() {
                //: Deleting this account in 5 seconds
                //% "Removing account"
                remorseAction(qsTrId("component_accounts-la-remove_account"),
                              function() { root._deleteAccount(model.accountId) })

            }

            ListView.onRemove: animateRemoval()

            AccountIcon {
                id: icon
                x: Theme.paddingLarge
                anchors.verticalCenter: parent.verticalCenter
                source: model.accountIcon
            }
            Label {
                id: accountName
                anchors.left: icon.right
                anchors.leftMargin: Theme.paddingLarge
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: model.accountDisplayName === "" ? 0 : -implicitHeight/2
                text: model.providerDisplayName
                color: highlighted ? Theme.highlightColor : Theme.primaryColor
            }
            Label {
                anchors.left: icon.right
                anchors.leftMargin: Theme.paddingLarge
                anchors.top: accountName.bottom
                text: model.accountDisplayName
                color: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
            }

            onClicked: {
                pageStack.push(root._createSettingsPage(model.providerName, {"accountId": model.accountId}))
            }
        }

        VerticalScrollDecorator {}

        PullDownMenu {
            MenuItem {
                //: Initiates adding a new account
                //% "Add Account";
                text: qsTrId("components_accounts-me-add_account")
                onClicked: {
                    var picker = pageStack.push(Qt.resolvedUrl("AccountProviderPickerDialog.qml"))
                    picker.providerSelected.connect(root._createAccount)
                }
            }
        }

        ViewPlaceholder {
            enabled: accountsView.count == 0

            //% "No accounts"
            text: qsTrId("components_accounts-he-no_accounts")
        }
    }
}
