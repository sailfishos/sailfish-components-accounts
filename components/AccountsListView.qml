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
                anchors.centerIn: parent
            }
        }
    }

    Component {
        id: delegateComponent
        BackgroundItem {
            id: accountDelegate
            onClicked: parent.parent.accountClicked(accountId)
            height: childrenRect.height
            width: childrenRect.width

            Row {
                id: accountDelegateRow
                spacing: 12
                Image {
                    source: accountIcon
                    height: 80
                    width: 80
                    fillMode: Image.PreserveAspectFit
                    y: 6 // XXX TODO: better way to do this?
                }

                Column {
                    id: accountDelegateColumn
                    HeadingLabel {
                        id: providerLabel
                        text: providerDisplayName
                    }
                    HeadingLabel {
                        id: usernameLabel
                        text: accountDisplayName // XXX TODO: modify model to supply username... or should displayName be username in most cases?
                        color: theme.secondaryColor
                    }
                }
            }
        }
    }
}
