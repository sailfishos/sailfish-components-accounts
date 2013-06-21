import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Silica.theme 1.0

SilicaListView {
    id: root

    property alias filterType: accountModel.filterType
    property alias filter: accountModel.filter

    signal serviceClicked(int accountId, string serviceName)

    spacing: Theme.paddingLarge
    model: AccountModel { id: accountModel }

    header: PageHeader {
        //: service accounts list view
        //% "Service Accounts"
        title: qsTrId("components_accounts-he-service_accounts_list")
    }

    delegate: Item {
        width: ListView.view.width
        height: Theme.itemSizeSmall

        AccountIcon {
            id: icon
            x: Theme.paddingLarge
            anchors.verticalCenter: parent.verticalCenter
            source: accountManager.provider(model.accountId).iconName
        }
        Label {
            id: accountName
            anchors.left: icon.right
            anchors.leftMargin: Theme.paddingLarge
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: model.accountDisplayName === "" ? 0 : -implicitHeight/2
            color: mouseArea.pressed ? Theme.highlightColor : Theme.primaryColor
            text: model.serviceDisplayName
        }
        Label {
            anchors.left: icon.right
            anchors.leftMargin: Theme.paddingLarge
            anchors.top: accountName.bottom
            text: model.accountDisplayName
            color: mouseArea.pressed ? Theme.secondaryHighlightColor : Theme.secondaryColor
        }
        MouseArea {
            id: mouseArea

            anchors.fill: parent
            onClicked: root.serviceClicked(model.accountId, model.serviceName)
        }
    }

    AccountManager { id: accountManager }

    VerticalScrollDecorator {}
}
