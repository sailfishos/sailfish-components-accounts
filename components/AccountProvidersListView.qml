import QtQuick 1.1
import com.jolla.components 1.0
import org.nemomobile.accounts 1.0
import org.nemomobile.signon 1.0

JollaListView {
    id: root

    property AccountProviderModel providerModel // must be provided by client
    signal providerClicked(string providerName)

    model: providerModel // default sort should be providerName alphabetical
    currentIndex: -1 // otherwise the last-added item steals focus
    delegate: delegateComponent
    header: headerComponent
    spacing: 20

    ScrollDecorator {}

    Component {
        id: headerComponent
        Item {
            width: root.width
            height: 147 // XXX TODO: design
            HeadingLabel {
                //: account providers list view
                //% "Account Providers"
                text: qsTrId("components_accounts-account_providers_list_view-account_providers")
                anchors.centerIn: parent
                color: theme.highlightColor
           }
        }
    }

    Component {
        id: delegateComponent
        AccountDelegate {
            id: accountDelegate
            onClicked: parent.parent.providerClicked(providerName)
            iconUrl: providerIcon
            topLabelText: providerDisplayName
        }
    }
}
