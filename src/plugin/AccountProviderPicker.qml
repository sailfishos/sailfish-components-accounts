import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Accounts 1.0

SilicaListView {
    id: root

    property alias serviceFilter: providerModel.serviceFilter

    signal providerSelected(int index, string providerName)
    signal providerDeselected(int index, string providerName)


    property AccountManager _accountManager: AccountManager {}
    property bool _hasExistingJollaAccount

    function _providerClicked(index, providerName) {
        if (selectionModel.get(index).providerName === "") {
            selectionModel.setProperty(index, "providerName", providerName)
            providerSelected(index, providerName)
        } else {
            selectionModel.setProperty(index, "providerName", "")
            providerDeselected(index, providerName)
        }
    }

    model: providerModel

    ProviderModel {
        id: providerModel
    }

    ListModel {
        id: selectionModel
    }

    delegate: ListItem {
        width: ListView.view.width
        highlighted: down || (model.index < selectionModel.count && selectionModel.get(model.index).providerName !== "")

        // don't offer the chance to create multiple jolla accounts through the UI
        visible: model.providerName !== "jolla" || !root._hasExistingJollaAccount
        contentHeight: visible ? Theme.itemSizeSmall : 0

        onClicked: {
            root._providerClicked(model.index, model.providerName)
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
        for (var i=0; i<count; i++) {
            selectionModel.append({"providerName": ""})
        }
        root._hasExistingJollaAccount = (_accountManager.providerAccountIdentifiers("jolla").length > 0)
    }

    VerticalScrollDecorator {}
}
