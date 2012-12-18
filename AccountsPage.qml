import QtQuick 1.1
import com.jolla.components 1.0
import org.nemomobile.accounts 1.0

Page {
    id: root

    AccountModel { id: accountsModel }

    AccountsListView {
        id: accountsListView
        anchors.fill: parent

        accountsModel: accountsModel

        PullDownMenu {
            MenuItem {
                //: Initiates adding a new account
                //% "Add Account";
                text: qsTrId("accounts-me-add_account")
                onClicked: openProviderSelector()
            }
        }

        onAccountClicked: openAccountEditor(accountId)
    }

    property Component _gae: GoogleAccountEditor {}
    property Component _app: AccountProvidersPage {}
    property QtObject _psp

    function openProviderSelector() {
        _psp = _app.createObject(root)
        _psp.providerSelected.connect(openAccountCreator)
        pageStack.push(_psp)
    }

    function openAccountCreator(providerName) {
        // take the provider selection page out of the stack.
        pageStack.pop(undefined, true)

        // now create the account creation page and show it.
        var provider = accountsModel.provider(providerName)
        if (!provider)
            throw new Error("Unable to obtain provider with name: " + providerName)
        var obj = _gae.createObject(root, { "provider": provider, "accountId": 0 })
        pageStack.push(obj)
    }

    function openAccountEditor(accId) {
        var provider = accountsModel.provider(accId)
        if (!provider)
            throw new Error("Unable to ascertain provider for account with id: " + accId)
        var obj = _gae.createObject(root, { "provider": provider, "accountId": accId })
        pageStack.push(obj)
    }
}
