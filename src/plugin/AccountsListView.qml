import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Accounts 1.0

SilicaListView {
    id: root

    //-------------- api

    property alias filterType: accountModel.filterType
    property alias filter: accountModel.filter

    signal accountClicked(int accountId, string providerName)

    //-------------- impl

    property bool _allowAccountDeletion
    signal _accountRemoveRequested(int accountId)
    signal _accountSyncRequested(int accountId)

    model: AccountModel { id: accountModel }

    delegate: ListItem {
        id: delegateItem

        property QtObject _syncHelper

        contentHeight: Theme.itemSizeMedium
        menu: root._allowAccountDeletion ? menuComponent : null

        Component {
            id: menuComponent

            ContextMenu {
                MenuItem {
                    text: model.accountEnabled
                            //: Disables a user account
                            //% "Disable"
                          ? qsTrId("components_accounts-me-disable")
                            //: Enables a user account
                            //% "Enable"
                          : qsTrId("components_accounts-me-enable")
                    onClicked: {
                        accountModel.setAccountEnabled(model.accountId, !accountEnabled)
                    }
                }

                MenuItem {
                    //: Removes a user account
                    //% "Remove"
                    text: qsTrId("components_accounts-me-remove_account")
                    onClicked: removeAccount()
                }

                MenuItem {
                    //: Syncs the data for this account
                    //% "Sync"
                    text: qsTrId("components_accounts-me-sync")
                    visible: model.accountEnabled
                            && (menu.providerName === "activesync" || accountSyncManager.profileIds(model.accountId).length > 0)

                    onClicked: {
                        root._accountSyncRequested(model.accountId)
                    }
                }
            }
        }

        function removeAccount() {
            //: Deleting this account in 5 seconds
            //% "Removing account"
            remorseAction(qsTrId("component_accounts-la-remove_account"),
                          function() { root._accountRemoveRequested(model.accountId) })

        }

        function _getSyncHelper() {
            if (_syncHelper == null) {
                _syncHelper = syncHelperComponent.createObject(delegateItem, {"accountId": model.accountId})
            }
            return _syncHelper
        }

        property bool isNewAccount: model.isNewAccount
        onIsNewAccountChanged: {
            if (isNewAccount) {
                var obj = _getSyncHelper()
                obj.monitorAccountSyncStatus = true
                obj.onAccountHasSyncingProfilesChanged.connect(function() {
                    // only monitor the initial sync status change for a new account
                    if (!obj.accountHasSyncingProfiles) {
                        obj.monitorAccountSyncStatus = false
                    }
                })
            }
        }

        onHighlightedChanged: {
            // Set the icon.opacity value manually to ensure the icon opacity doesn't change before
            // the label text colour change is applied (which is done when highlighted changes).
            icon.opacity = model.accountEnabled ? 1.0 : 0.3
        }

        ListView.onRemove: animateRemoval()

        AccountIcon {
            id: icon
            x: Theme.paddingLarge
            anchors.verticalCenter: parent.verticalCenter
            source: model.accountIcon
            opacity: model.accountEnabled ? 1.0 : 0.3
        }
        Label {
            id: accountName
            anchors {
                left: icon.right
                leftMargin: Theme.paddingLarge
                right: parent.right
                rightMargin: Theme.paddingLarge
                verticalCenter: parent.verticalCenter
                verticalCenterOffset: model.accountDisplayName === "" ? 0 : -implicitHeight/2
            }
            truncationMode: TruncationMode.Fade
            text: model.providerDisplayName
            color: {
                if (highlighted || model.accountError !== AccountModel.NoAccountError) {
                    return Theme.highlightColor
                }
                return model.accountEnabled
                        ? Theme.primaryColor
                        : Theme.rgba(Theme.primaryColor, 0.55)
            }
        }
        Label {
            anchors {
                left: icon.right
                leftMargin: Theme.paddingLarge
                top: accountName.bottom
                right: parent.right
                rightMargin: Theme.paddingLarge
            }
            truncationMode: TruncationMode.Fade
            text: {
                if (model.accountError === AccountModel.AccountNotSignedInError) {
                    //: The user has not logged into this account and needs to do so
                    //% "Not signed in"
                    return qsTrId("component_accounts-la-not_signed_in")
                }
                if (model.isNewAccount && _syncHelper && _syncHelper.accountHasSyncingProfiles) {
                    //: In the process of setting up this account
                    //% "Setting up account..."
                    return qsTrId("component_accounts-la-setting_up_account")
                }
                return model.accountDisplayName
            }
            color: {
                if (highlighted || model.accountError !== AccountModel.NoAccountError) {
                    return Theme.secondaryHighlightColor
                }
                return model.accountEnabled
                        ? Theme.secondaryColor
                        : Theme.rgba(Theme.secondaryColor, 0.3)
            }
        }

        onClicked: {
            root.accountClicked(model.accountId, model.providerName)
        }
    }

    AccountSyncManager {
        id: accountSyncManager
    }

    Component {
        id: syncHelperComponent

        AccountSyncManager {
            property int accountId
            property bool accountHasSyncingProfiles
            property bool monitorAccountSyncStatus

            property var _profilesSyncStatus
            property int _trackedProfileCount

            onProfileSyncStatusChanged: {
                if (!monitorAccountSyncStatus) {
                    return
                }
                if (_profilesSyncStatus === undefined) {
                    _profilesSyncStatus = {}
                    var accountProfiles = profileIds(accountId)
                    for (var i=0; i<accountProfiles.length; i++) {
                        _profilesSyncStatus[accountProfiles[i]] = AccountSyncManager.UnknownSyncStatus
                        _trackedProfileCount++
                    }
                }
                var profileIsSyncing = (status === AccountSyncManager.SyncStarted)
                if (_profilesSyncStatus[profileId] !== undefined) {
                    var wasSyncing = (_profilesSyncStatus[profileId] === AccountSyncManager.SyncStarted)
                    if (wasSyncing && !profileIsSyncing) {
                        // profile has finished syncing, remove it from the map
                        delete _profilesSyncStatus[profileId]
                        _trackedProfileCount--
                    } else {
                        _profilesSyncStatus[profileId] = status
                        if (!accountHasSyncingProfiles && profileIsSyncing) {
                            accountHasSyncingProfiles = true
                        }
                    }
                }
                if (_trackedProfileCount === 0) {
                    accountHasSyncingProfiles = false
                }
            }
        }
    }

    AccountManager { id: accountManager }
    VerticalScrollDecorator {}
}
