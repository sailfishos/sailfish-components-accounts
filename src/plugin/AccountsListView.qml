import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Accounts 1.0

SilicaListView {
    id: root

    //-------------- api

    property alias filterType: accountModel.filterType
    property alias filter: accountModel.filter

    signal accountClicked(int accountId, string providerName)

    //-------------- impl

    property bool _allowAccountDeletion
    signal _accountRemoveRequested(int accountId)

    model: AccountModel { id: accountModel }

    delegate: ListItem {
        contentHeight: Theme.itemSizeMedium
        menu: root._allowAccountDeletion ? menuComponent : null

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
                          function() { root._accountRemoveRequested(model.accountId) })

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
