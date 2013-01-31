import QtQuick 1.1
import Sailfish.Silica 1.0
import org.nemomobile.accounts 1.0
import org.nemomobile.signon 1.0

AccountCreationPage {
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
    property bool __saveOnInit: false
    property bool __hasCancelledOrError: false
    property bool __hasSynced: false

    backNavigation: false
    forwardNavigation: false

    SignOnUiContainer {
        id: container
        anchors.fill: root

        Item {
            id: topBar
            width: parent.width
            anchors.top: parent.top
            height: 80

            Label {
                text: provider.displayName
                font.family: theme.fontFamilyHeading
                anchors {
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                    margins: 10
                }
            }

            ToolIcon {
                iconSource: "image://theme/icon-header-cancel"
                onClicked: cancelOrBackstep()
                anchors {
                    top: parent.top
                    bottom: parent.bottom
                    left: parent.left
                    margins: 10
                }
            }
        }

        Label {
            id: centreLabel
            //: oauth account editor
            //% "Loading web page..."
            property string loadingString: qsTrId("components_accounts-oauth_account_editor-loading")
            //: oauth account editor
            //% "Error occurred:"
            property string errorString: qsTrId("components_accounts-oauth_account_editor-error_occurred")
            text: loadingString
            font.family: theme.fontFamilyHeading
            anchors.centerIn: parent
        }
        Label {
            id: errorLabel
            text: ""
            anchors.top: centreLabel.bottom
            anchors.horizontalCenter: centreLabel.horizontalCenter
            font.family: theme.fontFamilyHeading
        }
    }

    Component.onCompleted: saveAccount()

    function saveAccount() {
        if (_ident.status == Identity.Initialized) {
            // we actually save the identity first.
            if (_needsMechParamsAndSettings) {
                _ident.setMethodMechanisms("oauth2", [_mechanism])
            }
            _ident.caption = "caption" // have to set either caption or username in order to save :-/
            _ident.sync()
        } else {
            __saveOnInit = true
        }
    }

    function cleanup() {
        if (__isNewAccount) {
            // error occurred, new account, attempt to remove everything we added.
            if (_ident != null) {
                serviceIdent.signOut()
                _ident.remove()
            }
            if (_account != null) {
                _account.remove()
            }
        }
    }

    function cancelOrBackstep() {
        if (!__hasCancelledOrError) {
            __hasCancelledOrError = true
            cleanup()
            cancel(false)
        }
    }

    function errorOccurred(errorMessage) {
        if (!__hasCancelledOrError) {
            __hasCancelledOrError = true
            cleanup()
            centreLabel.text = centreLabel.errorString
            errorLabel.text = errorMessage
            errorTimer.start()
        }
    }

    Timer {
        id: errorTimer
        interval: 4000 // 4 seconds.
        repeat: false
        triggeredOnStart: false
        onTriggered: root.failure(false)
    }

    property Account _account: Account {
        identifier: root.accountId
        providerName: root.accountId != 0 ? "" : provider.name

        onStatusChanged: {
            if (status == Account.Initialized && !__isNewAccount) {
                _ident.identifier = _account.identityIdentifier(_signonServiceName) // if zero, will create new identity.
            } else if (status == Account.Synced) {
                // Successfully created the identity+account.  Begin signon.
                serviceIdent.identifier = _ident.identifier
            } else if (status == Account.Error) {
                errorOccurred(errorMessage)
            }
        }
    }

    property Identity _ident: Identity {
        identifier: root.accountId ? _account.identityIdentifier(_signonServiceName) : 0
        identifierPending: root.accountId != 0

        onStatusChanged: {
            if (status == Identity.Initialized) {
                if (__saveOnInit) {
                    saveAccount()
                }
            } else if (status == Identity.Synced) {
                if (!__hasSynced) {
                    __hasSynced = true
                    for (var i in provider.serviceNames) {
                        _account.enableWithService(provider.serviceNames[i])
                        _account.setIdentityIdentifier(_ident.identifier, provider.serviceNames[i])
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
            } else if (status == Identity.Error) {
                errorOccurred(errorMessage)
            }
        }
    }

    property AccountManager _acm: AccountManager { }

    ServiceAccountIdentity {
        id: serviceIdent
        onStatusChanged: {
            if (status == ServiceAccountIdentity.Initialized) {
                var serviceAccount = _acm.serviceAccount(_account.identifier, _signonServiceName)
                var adp = serviceAccount.authData.parameters
                for (var i in _signonSessionData) {
                    adp[i] = _signonSessionData[i]
                }

                // also ensure that we set up embedding / etc correctly
                // XXX TODO: fix this (broken due to recent orientation changes)
                //adp["WindowId"] = container.windowId()
                //adp["Embedded"] = false // just use dialog mode
                adp["Title"] = provider.displayName

                // begin sign on procedure.
                signIn(serviceAccount.authData.method, serviceAccount.authData.mechanism, adp)
            } else if (status == ServiceAccountIdentity.Error) {
                if (error == ServiceAccountIdentity.CanceledError) {
                    cancelOrBackstep()
                } else {
                    errorOccurred(errorMessage)
                }
            }
        }

        onResponseReceived: {
            root.accountId = _account.identifier
            if (_signonUserNameKey != "" && data["ScreenName"] != undefined) {
                _ident.userName = data[_signonUserNameKey]
                _account.displayName = data[_signonUserNameKey]
                _ident.sync()
                _account.sync()
            }
            serviceIdent.signOut()
            success(false) // Woohoo!
        }
    }
}
