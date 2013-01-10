import QtQuick 1.1
import com.jolla.components 1.0
import org.nemomobile.accounts 1.0
import org.nemomobile.signon 1.0
import "UiNavigation.js" as UiNavigation

Page {
    id: root

    property AccountModel accountsModel
    property Provider provider
    property int accountId: 0

    // EVERY provider-specific account creation page MUST call this function on successful save completion.
    function success() {
        UiNavigation.openAccountSettingsPage(root, true, accountsModel, accountId)
    }

    // EVERY provider-specific account creation page MUST call this function on failure / error
    function failure() {
        pageStack.pop();
    }

    // EVERY provider-specific account creation page MUST call this function on cancel
    function cancel() {
        pageStack.pop();
    }
}
