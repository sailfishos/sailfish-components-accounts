import QtQuick 1.1
import Sailfish.Silica 1.0
import org.nemomobile.accounts 1.0
import org.nemomobile.signon 1.0

AccountAuthenticator {
    id: root

    // The following are for extension plugins to set.
    property variant _signonSessionData // ClientId/ClientKey, ConsumerKey/ConsumerSecret, etc
    property string _signonServiceName  // Which service should be signed onto by default
    // Extension plugins may also implement: function postSignIn(variant signInResponseData) {}
    // If they implement this function, they _must_ call the postSignInFinished() function when finished.
    // The implementation of postSignIn may (for example) set the user name appropriately.

    //--------------------------------

    // implementation details.
    property bool __isNewAccount: accountId == 0
    property bool __hasCancelledOrError
    property bool __canSyncAccount
    property bool __canSyncIdentity: true
    property string __errorMessage

    function _syncIdentity() {
        if (__canSyncIdentity && ident.status == Identity.Initialized) {
            __canSyncIdentity = false
            __canSyncAccount = true

            // we actually save the identity first.
            ident.caption = "caption" // have to set either caption or username in order to save :-/
            ident.sync()
        }
    }

    function _cleanUp() {
        if (__isNewAccount) {
            serviceIdent.signOut()
            if (account != null) {
                var identifiers = account.identityIdentifiers
                for (var serviceName in identifiers) {
                    var identity = identityManager.identity(identifiers[serviceName])
                    if (identity) {
                        identity.remove()
                    }
                }
                account.remove()
            }
        }
    }

    function _cancel() {
        if (!__hasCancelledOrError) {
            __hasCancelledOrError = true

            _cleanUp()
            root.dialog.reject()
        }
    }

    function _errorOccurred(errorMessage) {
        if (!__hasCancelledOrError) {
            __hasCancelledOrError = true

            _cleanUp()
            root.__errorMessage = errorMessage
        }
    }

    anchors.fill: parent

    Timer {
        // can't set it immediately or else forward nav is not available later on
        running: true
        interval: 100
        onTriggered: root.dialog.forwardNavigation = false
    }

    Component.onDestruction: {
        jolla_signon_ui_service.inProcessParent = null // no longer servicing signon requests
    }

    Connections {
        target: root.dialog
        onDone: {
            if (result === DialogResult.Rejected) {
                root._cleanUp()
            }
        }
    }

    Item {
        id: container
        anchors.fill: root

        PageHeader {
            id: pageHeader
            //: Title of page for signing into a user account
            //% "Authentication"
            title: qsTrId("components_accounts-he-oauth_authentication")
        }

        Column {
            anchors.centerIn: parent
            width: parent.width - theme.paddingLarge*2
            spacing: theme.paddingMedium

            Label {
                //: Message displayed when waiting for authentication web page to be loaded
                //% "Loading web page..."
                property string loadingString: qsTrId("components_accounts-la-oauth_loading_web_page")

                //: Message displayed when error occurs during user authentication
                //% "Error while loading:"
                property string errorString: qsTrId("components_accounts-la-oauth_error")

                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                font.family: theme.fontFamilyHeading
                font.pixelSize: theme.fontSizeLarge
                text: root.__errorMessage === "" ? loadingString : errorString
            }
            Label {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                font.family: theme.fontFamilyHeading
                text: root.__errorMessage
            }
        }

        Item {
            id: webViewContainer
            anchors {
                top: pageHeader.bottom
                bottom: parent.bottom
                left: parent.left
                right: parent.right
            }
        }
    }

    AccountManager {
        id: accountMgr
    }

    account: Account { // this property may be accessed by provider extension ui qml
        identifier: root.accountId
        providerName: root.accountId != 0 ? "" : (root.provider ? root.provider.name : "")

        onStatusChanged: {
            if (status === Account.Initialized && !__isNewAccount) {
                ident.identifier = account.identityIdentifier(_signonServiceName) // if zero, will create new identity.
            } else if (status === Account.Synced) {
                // Successfully created the identity+account.  Begin signon.
                serviceIdent.identifier = ident.identifier
            } else if (status === Account.Error) {
                root._errorOccurred(errorMessage)
            }
        }
    }

    Identity {
        id: ident
        identifier: root.accountId ? account.identityIdentifier(_signonServiceName) : 0
        identifierPending: root.accountId != 0

        onStatusChanged: {
            if (status == Identity.Initialized) {
                root._syncIdentity()
            } else if (status === Identity.Synced) {
                if (__canSyncAccount) {
                    __canSyncAccount = false
                    for (var i in root.provider.serviceNames) {
                        account.disableWithService(root.provider.serviceNames[i]) // ensure disabled until Save in Settings page.
                        account.setIdentityIdentifier(ident.identifier, root.provider.serviceNames[i])
                    }
                    account.sync()
                }
            } else if (status === Identity.Error) {
                root._errorOccurred(errorMessage)
            }
        }

        Component.onCompleted: {
            root._syncIdentity()
        }
    }

    ServiceAccountIdentity {
        id: serviceIdent
        onStatusChanged: {
            if (status === ServiceAccountIdentity.Initialized) {
                var serviceAccount = accountMgr.serviceAccount(account.identifier, _signonServiceName)
                var adp = serviceAccount.authData.parameters
                for (var i in _signonSessionData) {
                    adp[i] = _signonSessionData[i]
                }

                // also ensure that we set up embedding / etc correctly:
                adp["Title"] = root.provider.displayName
                adp["InProcessServiceName"] = "com.jolla.settings"
                adp["InProcessObjectPath"] = "/JollaSettingsSignonUi"
                jolla_signon_ui_service.inProcessParent = webViewContainer

                // begin sign on procedure.
                signIn(serviceAccount.authData.method, serviceAccount.authData.mechanism, adp)
            } else if (status === ServiceAccountIdentity.Error) {
                if (error === ServiceAccountIdentity.CanceledError) {
                    root._cancel()
                } else {
                    root._errorOccurred(errorMessage)
                }
            }
        }

        onResponseReceived: {
            root.accountId = account.identifier
            postSignIn(data)
        }
    }

    function postSignIn(data) {
        // default implementation just calls finished function
        postSignInFinished()
    }

    function postSignInFinished() {
        serviceIdent.signOut()
        root.dialog.forwardNavigation = true
        root.dialog.accept()
    }
}
