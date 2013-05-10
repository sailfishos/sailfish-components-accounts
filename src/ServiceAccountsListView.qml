import QtQuick 1.1
import Sailfish.Silica 1.0
import org.nemomobile.accounts 1.0

SilicaListView {
    id: root

    signal serviceClicked(int accountId, string serviceName)

    spacing: theme.paddingLarge
    model: ServiceAccountModel {}

    header: PageHeader {
        //: service accounts list view
        //% "Service Accounts"
        title: qsTrId("components_accounts-he-service_accounts_list")
    }

    delegate: Item {
        width: ListView.view.width
        height: theme.itemSizeSmall

        AccountIcon {
            id: icon
            x: theme.paddingLarge
            anchors.verticalCenter: parent.verticalCenter
            source: model.serviceIcon
        }
        Label {
            id: accountName
            anchors.left: icon.right
            anchors.leftMargin: theme.paddingLarge
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: model.accountDisplayName === "" ? 0 : -implicitHeight/2
            color: mouseArea.pressed ? theme.highlightColor : theme.primaryColor
            text: model.serviceDisplayName
        }
        Label {
            anchors.left: icon.right
            anchors.leftMargin: theme.paddingLarge
            anchors.top: accountName.bottom
            text: model.accountDisplayName
            color: mouseArea.pressed ? theme.secondaryHighlightColor : theme.secondaryColor
        }
        MouseArea {
            id: mouseArea

            anchors.fill: parent
            onClicked: root.serviceClicked(model.accountId, model.serviceName)
        }
    }

    VerticalScrollDecorator {}
}
