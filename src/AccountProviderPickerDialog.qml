import QtQuick 1.1
import Sailfish.Silica 1.0
import org.nemomobile.accounts 1.0

Dialog {
    id: root

    signal providerSelected(string providerName)

    canAccept: false

    AccountProviderModel {
        id: providerModel
    }

    SilicaListView {
        anchors.fill: parent
        model: providerModel

        header: DialogHeader {
            //: accounts list view
            //% "Add accounts"
            acceptText: qsTrId("components_accounts-he-account_picker_title")
            dialog: root
        }

        delegate: BackgroundItem {
            width: ListView.view.width
            height: theme.itemSizeSmall

            onClicked: {
                root.canAccept = true
                providerSelected(model.providerName)
                root.accept()
            }

            AccountIcon {
                id: icon
                x: theme.paddingLarge
                anchors.verticalCenter: parent.verticalCenter
                source: model.providerIcon
            }
            Label {
                anchors.left: icon.right
                anchors.leftMargin: theme.paddingLarge
                anchors.verticalCenter: parent.verticalCenter
                text: model.providerDisplayName
                color: highlighted ? theme.highlightColor : theme.primaryColor
            }
        }

        VerticalScrollDecorator {}
    }
}
