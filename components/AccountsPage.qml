import QtQuick 1.1
import Sailfish.Silica 1.0
import org.nemomobile.accounts 1.0

Page {
    id: root

    AccountModel {
        id: accountsModel
    }

    AccountManager {
        id: accountManager
    }

    Component {
        id: contextMenuComponent

        ContextMenu {
            id: menu
            property int accountId

            MenuItem {
                //: Removes a user account
                //% "Remove";
                text: qsTrId("components_accounts-me-remove_account")
                onClicked: {
                    var account = accountManager.account(menu.accountId)
                    if (account !== null) {
                        account.remove()
                    }
                }
            }
        }
    }

    SilicaListView {
        id: accountsView

        anchors.fill: parent
        model: AccountModel {}
        spacing: 24
        header: PageHeader {
            //: accounts list view
            //% "Accounts"
            title: qsTrId("components_accounts-he-accounts_list")
        }

        delegate: Item {
            id: delegateItem

            width: ListView.view.width
            height: (root._contextMenu != null && root._contextMenu.parent === delegateItem)
                    ? root._contextMenu.height + contentItem.height
                    : contentItem.height

            BackgroundItem {
                id: contentItem

                height: 80

                onClicked: {
                    root.openAccountSettingsPage(model.accountId)
                }

                onPressAndHold: {
                    if (!root._contextMenu) {
                        root._contextMenu = contextMenuComponent.createObject(accountsView, {"accountId": model.accountId})
                    } else {
                        root._contextMenu.accountId = model.accountId
                    }
                    root._contextMenu.show(delegateItem)
                }

                Image {
                    id: icon
                    x: 24
                    anchors.verticalCenter: parent.verticalCenter
                    width: 64
                    height: 64
                    source: model.accountIcon
                }
                Label {
                    id: accountName
                    anchors.left: icon.right
                    anchors.leftMargin: 24
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.verticalCenterOffset: model.accountDisplayName === "" ? 0 : -implicitHeight/2
                    text: model.providerDisplayName
                }
                Label {
                    anchors.left: icon.right
                    anchors.leftMargin: 24
                    anchors.top: accountName.bottom
                    text: model.accountDisplayName
                    color: theme.secondaryColor
                }
            }
        }

        VerticalScrollDecorator {}

        PullDownMenu {
            MenuItem {
                //: Initiates adding a new account
                //% "Add Account";
                text: qsTrId("components_accounts-me-add_account")
                onClicked: {
                    if (_accountPicker === null) {
                        var comp = Qt.createComponent("AccountProviderPickerDialog.qml")
                        if (comp.status !== Component.Ready) {
                            throw new Error(comp.errorString())
                        }
                        _accountPicker = comp.createObject(root)
                        _accountPicker.accepted.connect(root._acceptedAccountPicker)
                    }
                    _accountPicker.open()
                }
            }
        }
    }

    Component.onCompleted: pageStack.busyChanged.connect(maybePushMaybePop)

    // ----------------------------------

    property Item _accountSettings
    property Item _accountPicker
    property Item _accountCreator
    property QtObject pushPage: null
    property bool __needsPush: false
    property bool __needsPop: false

    property Item _contextMenu

    function _acceptedAccountPicker() {
        openAccountCreationPage(_accountPicker.selectedProvider)
    }

    function continueAccountCreation(alreadyPopped) {
        // "queue up" showing the account settings page.
        //continueTimer.accountId = _accountCreator.accountId
        //continueTimer.start()

        if (!alreadyPopped) {
            pageStackPop()
        }
        openAccountSettingsPage(_accountCreator.accountId)
    }

    function cancelAccountCreation(alreadyPopped) {
        if (!alreadyPopped) {
            pageStackPop()
        }
        _accountCreator.destroy() // manually clean it up.
        _accountCreator = null
    }

    function maybePushMaybePop() {
        if (!pageStack.busy) {
            if (__needsPop) {
                __needsPop = false
                pageStack.pop()
            } else if (_accountSettings && __needsPush) {
                __needsPush = false
                pagePushTimer.start()
            }
        }
    }

    Timer {
        id: pagePushTimer
        interval: 1
        repeat: false
        triggeredOnStart: false
        onTriggered: pageStack.push(pushPage)
    }

    function pageStackPush(whichPage) {
        if (!pageStack.busy) {
            __needsPush = false
            pageStack.openDialog(whichPage)
        } else {
            __needsPush = true
            pushPage = whichPage
        }
    }

    function pageStackPop() {
        if (!pageStack.busy) {
            __needsPop = false
            pageStack.pop()
        } else {
            __needsPop = true
        }
    }

    function openAccountCreationPage(providerName) {
        console.log("openAccountCreationPage()", providerName)
        var provider = accountsModel.provider(providerName)
        if (!provider) {
            throw new Error("Unable to obtain provider with name: " + providerName)
        }

        // load the per-provider account creation page
        var componentName = "../extensions/" + provider.name + ".qml"
        var comp = Qt.createComponent(componentName)
        if (comp.status != Component.Ready) {
            throw new Error("Error creating provider-specific account creation page for provider \'" + provider.name + "\': " + comp.errorString())
        }

        if (_accountCreator != null) {
            _accountCreator.destroy() // clean up old creation page, if it exists.  This shouldn't happen, in practice, as it should be cleaned up on cancel.
        }

        // I want to use a Dialog for this, but cannot currently
        // because it doesn't allow specifying initial property values.
        _accountCreator = comp.createObject(root, { "accountsModel": accountsModel, "provider": provider })
        _accountCreator.success.connect(continueAccountCreation)
        _accountCreator.failure.connect(cancelAccountCreation)
        _accountCreator.cancel.connect(cancelAccountCreation)
        pageStack.replace(_accountCreator) // replace provider selection page with account creation page.
    }

    function openAccountSettingsPage(accountId) {
        console.log("\nopenAccountSettingsPage()", accountId)
        var comp = Qt.createComponent("AccountSettingsDialog.qml")
        if (comp.status !== Component.Ready) {
            throw new Error("Error creating account settings page for account \'" + accountId + "\': " + comp.errorString())
        }
        if (_accountSettings !== null) {
            _accountSettings.destroy()
        }
        _accountSettings = comp.createObject(root, {"accountId": accountId})

        if (_accountCreator != null) {
            // must be a continuation of an account creation.
            _accountCreator.destroy()
            _accountCreator = null
        }

        pageStackPush(_accountSettings)
    }
}
