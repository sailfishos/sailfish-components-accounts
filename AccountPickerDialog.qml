import QtQuick 1.1
import Sailfish.Silica 1.0
import org.nemomobile.accounts 1.0

Dialog {
    id: root

    property string selectedProvider

    AccountProviderModel {
        id: providerModel
    }

    onRejected: {
        selectedProvider = ""
    }

    sourceComponent: SilicaListView {
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
            opacity: 0.75

            onClicked: {
                root.selectedProvider = model.providerName
                root.accept()
            }

            Image {
                id: icon
                x: theme.paddingLarge
                anchors.verticalCenter: parent.verticalCenter
                width: 64
                height: 64
                source: model.providerIcon
            }
            Label {
                anchors.left: icon.right
                anchors.leftMargin: theme.paddingLarge
                anchors.verticalCenter: parent.verticalCenter
                text: model.providerDisplayName
                color: parent.down ? theme.highlightColor : theme.primaryColor
            }
        }

        VerticalScrollDecorator {}
    }
}
