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
    signal success(bool hasPopped)

    // EVERY provider-specific account creation page MUST call this function on failure / error
    signal failure(bool hasPopped)

    // EVERY provider-specific account creation page MUST call this function on cancel
    signal cancel(bool hasPopped)
}
