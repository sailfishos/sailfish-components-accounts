import QtQuick 1.1
import com.jolla.components 1.0
import org.nemomobile.accounts 1.0
import org.nemomobile.signon 1.0

JollaListView {
    id: root

    property AccountModel accountsModel // must be provided by client
    signal accountClicked(int accountId)

    model: accountsModel // default sort should be providerName alphabetical
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
                //: accounts list view
                //% "Accounts"
                text: qsTrId("components_accounts-accounts_list_view-accounts")
                color: theme.highlightColor
                anchors.centerIn: parent
            }
        }
    }

    Component {
        id: delegateComponent
        AccountDelegate {
            onClicked: parent.parent.accountClicked(accountId)
            iconUrl: accountIcon
            topLabelText: providerDisplayName
            bottomLabelText: accountDisplayName
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 20
        }
    }
}
