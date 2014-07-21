import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Accounts 1.0

SilicaListView {
    id: root

    property alias serviceFilter: providerModel.serviceFilter

    signal providerSelected(int index, string providerName)
    signal providerDeselected(int index, string providerName)   // deprecated


    property AccountManager _accountManager: AccountManager {}
    property bool _hasExistingJollaAccount

    model: providerModel

    ProviderModel {
        id: providerModel
    }

    delegate: ListItem {
        width: ListView.view.width

        // don't offer the chance to create multiple jolla accounts through the UI
        visible: model.providerName !== "jolla" || !root._hasExistingJollaAccount
        contentHeight: visible ? Theme.itemSizeSmall : 0

        onClicked: {
            root.providerSelected(model.index, model.providerName)
        }

        AccountIcon {
            id: icon
            x: Theme.paddingLarge
            anchors.verticalCenter: parent.verticalCenter
            source: model.providerIcon
        }
        Label {
            anchors.left: icon.right
            anchors.leftMargin: Theme.paddingLarge
            anchors.verticalCenter: parent.verticalCenter
            text: model.providerDisplayName
            color: highlighted ? Theme.highlightColor : Theme.primaryColor
        }
    }

    Connections {
        target: root._accountManager
        onAccountCreated: {
            if (!root._hasExistingJollaAccount) {
                var account = _accountManager.account(accountId)
                if (account && account.providerName === "jolla") {
                    root._hasExistingJollaAccount = true
                }
            }
        }
    }

    Component.onCompleted: {
        root._hasExistingJollaAccount = (_accountManager.providerAccountIdentifiers("jolla").length > 0)
    }

    VerticalScrollDecorator {}
}
