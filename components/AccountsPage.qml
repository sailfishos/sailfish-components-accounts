import QtQuick 1.1
import com.jolla.components 1.0
import org.nemomobile.accounts 1.0
import "UiNavigation.js" as UiNavigation

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

        onAccountClicked: openAccountSettings(accountId)
    }

    property Component _app: AccountProvidersPage {}
    property QtObject _psp

    function openProviderSelector() {
        _psp = _app.createObject(root)
        _psp.providerSelected.connect(openAccountCreator)
        pageStack.push(_psp)
    }

    function openAccountCreator(providerName) {
        UiNavigation.openAccountCreationPage(root, true, accountsModel, providerName, 0)        
    }

    function openAccountSettings(accId) {
        UiNavigation.openAccountSettingsPage(root, false, accountsModel, accId)
    }
}
