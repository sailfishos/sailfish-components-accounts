import QtQuick 1.1
import Sailfish.Silica 1.0
import org.nemomobile.accounts 1.0
import org.nemomobile.signon 1.0

AccountAuthenticator {
    id: root

    // the following are for extension plugins to set (if required)
    property variant _signonSessionData // ClientId/ClientKey, ConsumerKey/ConsumerSecret, etc
    property string _signonServiceName  // Which service should be signed onto by default
    property string _signonUserNameKey  // Which key in the response data is the username.  None by default.
    property bool _needsCaption: false
    property bool _needsMechParamsAndSettings: false // by default, the following three properties' values are prefilled from .provider file.
    property variant _oauthParameters
    property variant _accountSettings
    property string _mechanism

    // the following contains the available mechanisms.  _mechanism must be set to one of these.
    property variant _mechanisms: ["user_agent", "web_server", "HMAC-SHA1", "PLAINTEXT", "RSA-SHA1"]

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
            if (_needsMechParamsAndSettings) {
                ident.setMethodMechanisms("oauth2", [_mechanism])
            }
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
            if (_account != null) {
                _account.remove()
            }
        }
    }

    function _cancel() {
        if (!__hasCancelledOrError) {
            root.dialog.backNavigation = true
            __hasCancelledOrError = true

            _cleanUp()
            root.dialog.reject()
        }
    }

    function _errorOccurred(errorMessage) {
        if (!__hasCancelledOrError) {
            root.dialog.backNavigation = true
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

    function _dialogStatusChanged() {
        if (status === PageStatus.Active) {
            if (__hasCancelledOrError) {
                root.dialog.forwardNavigation = false
            }
        }
        if (__pendingResult >= 0) {
            _closeDialog(__pendingResult)
        }
    }

    anchors.fill: parent

    Component.onCompleted: {
        saveAccount()
        root.dialog.backNavigation = false
        root.dialog.statusChanged.connect(_dialogStatusChanged)
    }

    SignOnUiContainer {
        id: container
        anchors.fill: root

        PageHeader {
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
    }

    Account {
        id: _account
        identifier: root.accountId
        providerName: root.accountId != 0 ? "" : root.provider.name

        onStatusChanged: {
            if (status === Account.Initialized && !__isNewAccount) {
                ident.identifier = _account.identityIdentifier(_signonServiceName) // if zero, will create new identity.
            } else if (status === Account.Synced) {
                // Successfully created the identity+account.  Begin signon.
                serviceIdent.identifier = ident.identifier
            } else if (status === Account.Error) {
                root._errorOccurred(errorMessage)
            }
        }
    }

    AccountManager {
        id: accountMgr
    }

    Identity {
        id: ident
        identifier: root.accountId ? _account.identityIdentifier(_signonServiceName) : 0
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
                        _account.enableWithService(root.provider.serviceNames[i])
                        _account.setIdentityIdentifier(ident.identifier, root.provider.serviceNames[i])
                    }
                    if (_needsMechParamsAndSettings) {
                        _account.setConfigurationValue("auth/method", "oauth2")
                        _account.setConfigurationValue("auth/mechanism", _mechanism)
                        var prefix = "auth/oauth2/" + _mechanism + "/"
                        for (var i in _oauthParameters) {
                            _account.setConfigurationValue(prefix + i, _oauthParameters[i])
                        }
                        for (var i in _accountSettings) {
                            _account.setConfigurationValue(i, _accountSettings[i])
                        }
                    }
                    _account.sync()
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
                var serviceAccount = accountMgr.serviceAccount(_account.identifier, _signonServiceName)
                var adp = serviceAccount.authData.parameters
                for (var i in _signonSessionData) {
                    adp[i] = _signonSessionData[i]
                }

                // also ensure that we set up embedding / etc correctly
                // XXX TODO: fix this (broken due to recent orientation changes)
                //adp["WindowId"] = container.windowId()
                //adp["Embedded"] = false // just use dialog mode
                adp["Title"] = root.provider.displayName

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
            root.accountId = _account.identifier
            if (_signonUserNameKey != "" && data["ScreenName"] != undefined) {
                ident.userName = data[_signonUserNameKey]
                _account.displayName = data[_signonUserNameKey]
                ident.sync()
                _account.sync()
            }
            serviceIdent.signOut()
            root._closeDialog(DialogResult.Accepted)
        }
    }
}
