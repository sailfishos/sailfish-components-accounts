import QtQuick 1.1
import Sailfish.Silica 1.0
import org.nemomobile.accounts 1.0
import org.nemomobile.signon 1.0

// page allows toggling enabling with different services + deleting the account
Page {
    id: root

    property AccountModel accountsModel
    property int accountId: 0

    // ------------------

    forwardNavigation: true

    SilicaFlickable {
        id: flick
        contentHeight: childrenRect.height
        anchors.fill: parent

        PullDownMenu {
            MenuItem {
                //: Deletes the account
                //% "Delete Account";
                text: qsTrId("accounts-me-delete_account")
                onClicked: _account.remove() // when removes successfully, will pop the page automatically.
            }
        }

        PageHeader {
            id: header
            //: Save the account settings
            //% "Save"
            title: qsTrId("accounts-me-save")
            anchors.right: parent.right
            anchors.rightMargin: 60 // XXX TODO: better way to do this
        }


        Column {
            id: accountNameColumn
            anchors {
                left: parent.left
                right: parent.right
                top: header.bottom
                margins: 20
            }

            Label {
                id: accountNameLabel
                font.family: theme.fontFamilyHeading
                //: account settings page
                //% "Account Name"
                text: qsTrId("components_accounts-account_settings_page-account_name")
                truncationMode: TruncationMode.Fade
                anchors.right: parent.right
            }
            TextField {
                id: accountNameText
                width: root.width
                //: account settings page
                //% "Name this account"
                placeholderText: qsTrId("components_accounts-account_settings_page-account_name_ph")
                horizontalAlignment: Text.AlignRight
                anchors {
                    right: parent.right
                    left: parent.left
                }
            }
        }

        Grid {
            id: grid
            columns: 2
            anchors {
                top: accountNameColumn.bottom
                right: accountNameColumn.right
                left: accountNameColumn.left
            }

            AccountDelegate {
                id: accountDelegate
                iconUrl: _provider.iconName
                topLabelText: _provider.displayName
                bottomLabelText: _account.displayName
                labelFont: fontProvider.font
                width: parent.width - globalEnableSwitch.width
            }
            Switch {
                id: globalEnableSwitch
                checked: _account.enabled
                onCheckedChanged: _account.enabled = checked // note: we don't sync() until save().
            }

            // toggles for each service will be dynamically added to this grid
        }
    }

    QtObject {
        id: saveFlickHandler
        property int navigation: root._navigation
        onNavigationChanged: {
            if (navigation == PageNavigation.Forward) {
                // Save
                _account.displayName = accountNameText.text
                _account.sync()
            } else if (navigation == PageNavigation.Back) {
                // Cancel
                console.log("AccountSettings not saved for account: " + _account.displayName)
            }
        }
    }

    Component {
        id: toggleFactory
        Switch {
            id: toggle
            property Account account
            property QtObject service
            onCheckedChanged: {
                if (checked) {
                    account.enableWithService(service.name) // note: we don't sync() until save().
                } else {
                    account.disableWithService(service.name) // note: we don't sync() until save().
                }
            }
        }
    }

    Component {
        id: serviceDelegateFactory
        AccountDelegate {
            id: serviceDelegate
            property QtObject service
            iconUrl: service.iconName
            topLabelText: service.displayName
            labelFont: fontProvider.font
        }
    }

    Text {
        id: fontProvider
        visible: false
        font {
            family: theme.fontFamilyHeading
            pixelSize: theme.fontSizeMedium
        }
    }

    function populateServiceSwitches() {
        // feels horrible to do this imperatively... but
        // the alternative is writing a per-provider service model.
        for (var i in _account.supportedServiceNames) {
            var service = _acm.service(_account.supportedServiceNames[i])
            var accountIsEnabledWithService = false
            console.log("XXX TODO: for some reason, the enabledServiceNames for any account seems empty.... FIXME!")
            for (var j in _account.enabledServiceNames) {
                if (_account.enabledServiceNames[j] == service.name) {
                    accountIsEnabledWithService = true
                    break
                }
            }

            var toggle = toggleFactory.createObject(null, { "account": _account, "service": service, "checked": accountIsEnabledWithService })
            serviceDelegateFactory.createObject(grid, { "service": service, "width": (grid.width - toggle.width) })
            toggle.parent = grid
        }
    }

    property AccountManager _acm: AccountManager { }
    property Provider _provider: _acm.provider(_account.providerName)
    property Account _account: Account {
        property bool _hasInited: false
        identifier: root.accountId
        onStatusChanged: {
            if (status == Account.Initialized) {
                _hasInited = true
                populateServiceSwitches()
                if (displayName != "") {
                    accountNameText.text = displayName
                }
            } else if (status == Account.Error) {
                // display "error" dialog
                cleanup() // no need to pop, as already popped due to forwardStep.
            } else if (status == Account.Invalid) {
                if (_hasInited) {
                    console.log("Successfully deleted.")
                    pageStack.pop()
                }
            }
        }
    }
}
