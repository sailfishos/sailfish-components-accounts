import QtQuick 1.1
import com.jolla.components 1.0
import org.nemomobile.accounts 1.0

Page {
    id: root

    // if true, shows per-service accounts.
    // if false, shows per-provider accounts.
    property bool showServiceAccounts: false

    // ------------------------------------

    AccountModel { id: accountsModel }
    ServiceAccountModel { id: serviceAccountsModel }

    ServiceAccountsListView {
        id: serviceAccountsListView
        visible: showServiceAccounts == true
        anchors.fill: parent
        accountsModel: serviceAccountsModel
        onAccountClicked: openAccountSettings(accountId)

        PullDownMenu {
            MenuItem {
                //: Initiates adding a new account
                //% "Add Account";
                text: qsTrId("accounts-me-add_account")
                onClicked: openProviderSelector()
            }
        }
    }

    AccountsListView {
        id: accountsListView
        visible: showServiceAccounts == false
        anchors.fill: parent
        accountsModel: accountsModel
        onAccountClicked: openAccountSettings(accountId)

        PullDownMenu {
            MenuItem {
                //: Initiates adding a new account
                //% "Add Account";
                text: qsTrId("accounts-me-add_account")
                onClicked: openProviderSelector()
            }
        }
    }

    Component.onCompleted: pageStack.busyChanged.connect(maybePushMaybePop)

    // ----------------------------------

    property Component _app: AccountProvidersPage {}
    property QtObject _psp // provider selection page
    property QtObject _acp // account creation page
    property QtObject _asp // account settings page
    property QtObject pushPage: null
    property bool __needsPush: false
    property bool __needsPop: false

    function openProviderSelector() {
        if (_psp == null) {
            _psp = _app.createObject(root)
            _psp.providerSelected.connect(openAccountCreator)
        }
        pageStackPush(_psp)
    }

    function openAccountSettings(accId) {
        openAccountSettingsPage(accId)
    }

    function openAccountCreator(providerName) {
        openAccountCreationPage(providerName, 0)        
    }

    function continueAccountCreation(alreadyPopped) {
        // "queue up" showing the account settings page.
        //continueTimer.accountId = _acp.accountId
        //continueTimer.start()

        if (!alreadyPopped) {
            pageStackPop()
        }
        openAccountSettingsPage(_acp.accountId)
    }

    function cancelAccountCreation(alreadyPopped) {
        if (!alreadyPopped) {
            pageStackPop()
        }
        _acp.destroy() // manually clean it up.
        _acp = null
    }

    function maybePushMaybePop() {
        if (!pageStack.busy) {
            if (__needsPop) {
                __needsPop = false
                pageStack.pop()
            } else if (_asp && __needsPush) {
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
            pageStack.push(whichPage)
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

    /* Factory Functions. */

    // providerName OR accountId must be given, not both
    function openAccountCreationPage(providerName, accountId) {
        var provider = null
        if (accountId == 0) {
            // creating a new account.
            provider = accountsModel.provider(providerName)
            if (!provider) {
                throw new Error("Unable to obtain provider with name: " + providerName)
            }
        } else {
            // editing an existing account.
            provider = accountsModel.provider(accountId)
            if (!provider) {
                throw new Error("Unable to ascertain provider for account with id: " + accountId)
            }
        }

        // load the per-provider account creation page
        var componentName = "../extensions/" + provider.name + ".qml"
        var comp = Qt.createComponent(componentName)
        if (comp.status != Component.Ready) {
            throw new Error("Error creating provider-specific account creation page for provider \'" + provider.name + "\': " + comp.errorString())
        }

        if (_acp != null) {
            _acp.destroy() // clean up old creation page, if it exists.  This shouldn't happen, in practice, as it should be cleaned up on cancel.
        }

        // I want to use a Dialog for this, but cannot currently
        // because it doesn't allow specifying initial property values.
        _acp = comp.createObject(root, { "accountsModel": accountsModel, "provider": provider, "accountId": accountId })
        _acp.success.connect(continueAccountCreation)
        _acp.failure.connect(cancelAccountCreation)
        _acp.cancel.connect(cancelAccountCreation)
        pageStack.replace(_acp) // replace provider selection page with account creation page.
    }

    // accountId MUST be non-zero
    function openAccountSettingsPage(accountId) {
        var componentName = "AccountSettingsPage.qml"
        var comp = Qt.createComponent(componentName)
        if (comp.status != Component.Ready) {
            throw new Error("Error creating account settings page for account \'" + accountId + "\': " + comp.errorString())
        }

        if (_acp != null) {
            // must be a continuation of an account creation.
            _acp.destroy()
            _acp = null
        }

        if (_asp != null) {
            _asp.destroy() // clean up old settings page.
        }

        _asp = comp.createObject(root, { "accountsModel": accountsModel, "accountId": accountId })
        pageStackPush(_asp)
    }
}
