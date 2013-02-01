import QtQuick 1.1
import Sailfish.Silica 1.0

SilicaListView {

    signal accountClicked(int accountId)

    header: PageHeader {
        //: service accounts list view
        //% "Service Accounts"
        title: qsTrId("components_accounts-he-service_accounts_list")
    }

    delegate: Item {
        Image {
            id: icon
            x: 24
            anchors.verticalCenter: parent.verticalCenter
            width: 64
            height: 64
            source: model.serviceIcon
        }
        Label {
            id: accountName
            anchors.left: icon.right
            anchors.leftMargin: 24
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: model.accountDisplayName === "" ? 0 : -implicitHeight/2
            text: model.serviceDisplayName
        }
        Label {
            anchors.left: icon.right
            anchors.leftMargin: 24
            anchors.top: accountName.bottom
            text: model.accountDisplayName
            color: theme.secondaryColor
        }
    }

    VerticalScrollDecorator {}
}
