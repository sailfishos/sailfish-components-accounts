import QtQuick 1.1
import Sailfish.Silica 1.0
import org.nemomobile.accounts 1.0
import org.nemomobile.signon 1.0

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
