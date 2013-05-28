import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Silica.theme 1.0
import org.nemomobile.accounts 1.0

Dialog {
    id: root

    signal providerSelected(string providerName)

    AccountProviderModel {
        id: providerModel
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
            height: Theme.itemSizeSmall

            onClicked: {
                providerSelected(model.providerName)
                root.accept()
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

        VerticalScrollDecorator {}
    }
}
