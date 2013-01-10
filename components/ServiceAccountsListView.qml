import QtQuick 1.1
import com.jolla.components 1.0
import org.nemomobile.accounts 1.0
import org.nemomobile.signon 1.0

JollaListView {
    id: root

    property ServiceAccountModel accountsModel // must be provided by client
    signal accountClicked(int accountId)

    model: accountsModel // default sort should be providerDisplayName alphabetical followed by serviceDisplayName alphabetical
    currentIndex: -1 // otherwise the last-added item steals focus
    delegate: delegateComponent
    header: headerComponent
    spacing: 20

    ScrollDecorator {}

    Component {
        id: headerComponent
        Item {
            width: root.width
            height: 147 // XXX TODO: get real design

            HeadingLabel {
                //: service accounts list view
                //% "Service Accounts"
                text: qsTrId("components_accounts-service_accounts_list_view-service_accounts")
                anchors.centerIn: parent
            }
        }
    }

    Component {
        id: delegateComponent
        AccountDelegate {
            id: accountDelegate
            onClicked: parent.parent.accountClicked(accountId)
            iconUrl: serviceIcon
            topLabelText: serviceDisplayName
            bottomLabelText: accountDisplayName
        }
    }
}
