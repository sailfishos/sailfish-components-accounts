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
    property bool __saveOnInit
    property bool __hasCancelledOrError
    property bool __hasSynced
    property int __pendingResult: -1
    property string __errorMessage

    function saveAccount() {
        if (ident.status == Identity.Initialized) {
            // we actually save the identity first.
            ident.caption = "caption" // have to set either caption or username in order to save :-/
            ident.sync()
        } else {
            __saveOnInit = true
        }
    }

    function _cleanUp() {
        if (__isNewAccount) {
            // error occurred, new account, attempt to remove everything we added.
            if (ident != null) {
                serviceIdent.signOut()
                ident.remove()
            }
            if (account != null) {
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

    function _closeDialog(result) {
        if (status === PageStatus.Active) {
            __pendingResult = -1
            if (result === DialogResult.Accepted) {
                root.dialog.forwardNavigation = true
                root.dialog.accept()
            } else {
                root.dialog.reject()
            }
        } else {
            __pendingResult = result
        }
    }

    Timer {
        // can't set it immediately or else forward nav is not available later on
        running: true
        interval: 100
        onTriggered: root.dialog.forwardNavigation = false
    }

    anchors.fill: parent

    Component.onCompleted: {
        saveAccount()
    }

    Component.onDestruction: {
        jolla_signon_ui_service.inProcessParent = null // no longer servicing signon requests
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

        Label {
            id: centreLabel

            //: Message displayed when waiting for authentication web page to be loaded
            //% "Loading web page..."
            property string loadingString: qsTrId("components_accounts-la-oauth_loading_web_page")

            //: Message displayed when error occurs during user authentication
            //% "Error occurred:"
            property string errorString: qsTrId("components_accounts-la-oauth_error")

            text: root.__errorMessage === "" ? loadingString : errorString
            font.family: theme.fontFamilyHeading
            anchors.centerIn: parent
        }
        Label {
            anchors.top: centreLabel.bottom
            anchors.horizontalCenter: centreLabel.horizontalCenter
            font.family: theme.fontFamilyHeading
            text: root.__errorMessage
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
        providerName: root.accountId != 0 ? "" : root.provider.name

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
                if (__saveOnInit) {
                    saveAccount()
                }
            } else if (status === Identity.Synced) {
                if (!__hasSynced) {
                    __hasSynced = true
                    for (var i in root.provider.serviceNames) {
                        account.enableWithService(root.provider.serviceNames[i])
                        account.setIdentityIdentifier(ident.identifier, root.provider.serviceNames[i])
                    }
                    account.sync()
                }
            } else if (status === Identity.Error) {
                root._errorOccurred(errorMessage)
            }
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
        root._closeDialog(DialogResult.Accepted)
    }
}
