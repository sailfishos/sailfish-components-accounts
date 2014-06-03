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
            opacity: model.accountEnabled ? 1.0 : 0.3
        }
        Label {
            id: accountName
            anchors {
                left: icon.right
                leftMargin: Theme.paddingLarge
                right: parent.right
                rightMargin: Theme.paddingLarge
                verticalCenter: parent.verticalCenter
                verticalCenterOffset: model.accountDisplayName === "" ? 0 : -implicitHeight/2
            }
            truncationMode: TruncationMode.Fade
            text: model.providerDisplayName
            color: {
                if (highlighted || model.accountError !== AccountModel.NoAccountError) {
                    return Theme.highlightColor
                }
                return model.accountEnabled
                        ? Theme.primaryColor
                        : Theme.rgba(Theme.primaryColor, 0.55)
            }
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
            text: model.accountError === AccountModel.AccountNotSignedInError
                    //: The user has not logged into this account and needs to do so
                    //% "Not signed in"
                  ? qsTrId("component_accounts-la-not_signed_in")
                  : model.accountDisplayName
            color: {
                if (highlighted || model.accountError !== AccountModel.NoAccountError) {
                    return Theme.secondaryHighlightColor
                }
                return model.accountEnabled
                        ? Theme.secondaryColor
                        : Theme.rgba(Theme.secondaryColor, 0.3)
            }
        }

        onClicked: {
            root.accountClicked(model.accountId, model.providerName)
        }
    }

    AccountManager { id: accountManager }
    VerticalScrollDecorator {}
}
