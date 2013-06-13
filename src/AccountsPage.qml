import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Silica.theme 1.0
import org.nemomobile.accounts 1.0
import org.nemomobile.signon 1.0

Page {
    id: root

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

    AccountCreationManager {
        id: accountCreationManager

        onAccountDeletionRequested: _deleteAccount(accountId)
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
                        //% "Remove"
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
                anchors {
                    left: icon.right
                    leftMargin: Theme.paddingLarge
                    top: accountName.bottom
                    right: parent.right
                    rightMargin: Theme.paddingLarge
                }
                truncationMode: TruncationMode.Fade
                text: model.accountDisplayName
                color: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
            }

            onClicked: {
                var provider = accountModel.provider(providerName)
                if (!provider) {
                    throw new Error("Unable to obtain provider with name: " + providerName)
                }
                pageStack.push(accountCreationManager.createSettingsPage(model.providerName,
                        {"accountProvider": provider, "accountId": model.accountId}))
            }
        }

        VerticalScrollDecorator {}

        PullDownMenu {
            MenuItem {
                //: Initiates adding a new account
                //% "Add Account"
                text: qsTrId("components_accounts-me-add_account")
                onClicked: accountCreationManager.startAccountCreation()
            }
        }

        ViewPlaceholder {
            enabled: accountsView.count == 0

            //% "No accounts"
            text: qsTrId("components_accounts-he-no_accounts")
        }
    }
}
