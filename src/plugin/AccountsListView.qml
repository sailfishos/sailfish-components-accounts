import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Accounts 1.0

SilicaListView {
    id: root

    property alias filterType: accountModel.filterType
    property alias filter: accountModel.filter
    property bool allowAccountDeletion

    signal accountClicked(int accountId, string providerName)

    function _deleteAccount(accountId) {
        var account = accountManager.account(accountId)
        if (account === null) {
            return
        }
        account.statusChanged.connect(function() {
            if (account.status === Account.Initialized) {
                account.remove()
            }
        })
    }

    model: AccountModel { id: accountModel }
    header: PageHeader {
        //: accounts list view
        //% "Accounts"
        title: qsTrId("components_accounts-he-accounts_list")
    }

    delegate: ListItem {
        contentHeight: Theme.itemSizeMedium
        menu: root.allowAccountDeletion ? menuComponent : null

        Component {
            id: menuComponent

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
            anchors {
                left: icon.right
                leftMargin: Theme.paddingLarge
                verticalCenter: parent.verticalCenter
                verticalCenterOffset: model.accountDisplayName === "" ? 0 : -implicitHeight/2
            }
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
            root.accountClicked(model.accountId, model.providerName)
        }
    }

    AccountManager { id: accountManager }

    VerticalScrollDecorator {}
}
