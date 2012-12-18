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
            Label {
                //: account providers list view
                //% "Account Providers"
                text: qsTrId("components_accounts-account_providers_list_view-account_providers")
                color: theme.primaryColor
                font {
                    family: theme.fontFamilyHeading
                    pixelSize: theme.fontSizeLarge
                }
                anchors.centerIn: parent
           }
        }
    }

    Component {
        id: delegateComponent
        MouseArea {
            id: accountDelegate
            onClicked: parent.parent.providerClicked(providerName)
            height: childrenRect.height
            width: childrenRect.width

            Row {
                id: accountDelegateRow
                spacing: 12
                Image {
                    source: providerIcon
                    height: 80
                    width: 80
                    fillMode: Image.PreserveAspectFit
                    y: 6 // XXX TODO: better way to do this?
                }

                Label {
                    id: providerLabel
                    text: providerDisplayName
                    color: theme.secondaryColor
                    font {
                        pixelSize: theme.fontSizeLarge
                        family: theme.fontFamilyHeading
                    }
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }
}
