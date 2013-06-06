import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Silica.theme 1.0
import org.nemomobile.accounts 1.0

Dialog {
    id: root

    property int providerCount: view.count

    property int _selectionCount

    signal providerSelected(int index, string providerName)
    signal providerDeselected(int index, string providerName)

    function _providerClicked(index, providerName) {
        if (selectionModel.get(index).providerName === "") {
            _selectionCount++
            selectionModel.setProperty(index, "providerName", providerName)
            providerSelected(index, providerName)
        } else {
            _selectionCount--
            selectionModel.setProperty(index, "providerName", "")
            providerDeselected(index, providerName)
        }
    }

    canAccept: _selectionCount > 0

    AccountProviderModel {
        id: providerModel
    }

    ListModel {
        id: selectionModel
    }

    SilicaListView {
        id: view
        anchors.fill: parent
        model: providerModel

        header: DialogHeader {
            //: Number of selected accounts
            //% "%n selected"
            acceptText: qsTrId("components_accounts-he-selected_accounts", root._selectionCount)
            dialog: root
        }

        delegate: ListItem {
            width: ListView.view.width

            highlighted: down || (model.index < selectionModel.count && selectionModel.get(model.index).providerName !== "")

            onClicked: root._providerClicked(model.index, model.providerName)

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

        Component.onCompleted: {
            for (var i=0; i<count; i++) {
                selectionModel.append({"providerName": ""})
            }
        }

        VerticalScrollDecorator {}
    }
}
