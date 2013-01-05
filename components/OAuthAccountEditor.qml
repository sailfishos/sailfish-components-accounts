import QtQuick 1.1
import com.jolla.components 1.0
import org.nemomobile.accounts 1.0
import org.nemomobile.signon 1.0

Page {
    id: root

    property Provider provider
    property int accountId: 0

    // the following are for extension plugins to set (if required)
    property bool _needsCaption: false
    property bool _needsMechParamsAndSettings: false // by default, the following three properties' values are prefilled from .provider file.
    property variant _oauthParameters
    property variant _accountSettings
    property string _mechanism

    // the following contains the available mechanisms.  _mechanism must be set to one of these.
    property variant _mechanisms: ["user_agent", "web_server", "HMAC-SHA1", "PLAINTEXT", "RSA-SHA1"]

    // implementation details.
    property string __defaultServiceName: provider.serviceNames[0]
    property bool __isNewAccount: accountId == 0

    AccountHeader {
        id: header
        width: parent.width

        providerDisplayName: provider.displayName
        providerIconImageUrl: provider.iconName

        // Left icon - cancel edit
        ToolIcon {
            iconSource: "image://theme/icon-header-cancel"
            height: parent.height

            y: parent.y - 15        // Battling implicit margins.
            anchors {
                left: parent.left
                leftMargin: 10
            }

            onClicked: {
                pageStack.pop()
            }

        }

        // Right icon - save edit
        ToolIcon {
            iconSource: "image://theme/icon-header-accept"
            height: parent.height

            y: parent.y - 15        // Battling implicit margins.
            anchors {
                right: parent.right
                rightMargin: 10
            }

            onClicked: {
                saveAccount()
            }
        }
    }

    JollaFlickable {
        id: flick
        contentHeight: childrenRect.height

        anchors {
            top: header.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            margins: 20
        }

        Column {
            id: col
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 20

            Label {
                id: usernameLabel
                //: oauth account editor
                //% "User Name"
                text: qsTrId("components_accounts-oauth_account_editor-user_name")
                anchors.right: parent.right
            }

            TextField {
                id: usernameText
                width: root.width
                //: oauth account editor
                //% "username"
                placeholderText: qsTrId("components_accounts-oauth_account_editor-username_ph")
                property bool hasChanged: false
                onTextChanged: {
                    if (text != "" && !hasChanged) {
                        hasChanged = true
                    }
                    if (hasChanged) {
                        _ident.userName = text
                    }
                }
            }

            Label {
                id: passwordLabel
                //: oauth account editor
                //% "Password"
                text: qsTrId("components_accounts-oauth_account_editor-password")
                anchors.right: parent.right
            }

            TextField {
                id: passwordText
                width: root.width
                echoMode: TextInput.PasswordEchoOnEdit
                //: oauth account editor
                //% "password"
                placeholderText: qsTrId("components_accounts-oauth_account_editor-password_ph")
                property bool hasChanged: false
                onTextChanged: {
                    if (text != "" && !hasChanged) {
                        hasChanged = true
                    }
                    if (hasChanged) {
                        _ident.secret = text
                    }
                }
            }

            Label {
                id: captionLabel
                visible: _needsCaption
                //: oauth account editor
                //% "Caption"
                text: qsTrId("components_accounts-oauth_account_editor-caption")
                anchors.right: parent.right
            }

            TextField {
                id: captionText
                visible: _needsCaption
                width: root.width
                //: oauth account editor
                //% "caption"
                placeholderText: qsTrId("components_accounts-oauth_account_editor-caption_ph")
                property bool hasChanged: false
                onTextChanged: {
                    if (text != "" && !hasChanged) {
                        hasChanged = true
                    }
                    if (hasChanged) {
                        _ident.caption = text
                    }
                }
            }
        }
    }

    function saveAccount() {
        // we actually save the identity first.
        if (_needsMechParamsAndSettings) {
            _ident.setMethodMechanisms("oauth2", [_mechanism])
        }
        _ident.sync()
    }

    function cleanup() {
        if (__isNewAccount) {
            // error occurred, new account, attempt to remove everything we added.
            _ident.remove()
            _account.remove()
        }
    }

    property Account _account: Account {
        identifier: root.accountId
        providerName: root.accountId != 0 ? "" : provider.name

        onStatusChanged: {
            if (status == Account.Initialized && !__isNewAccount) {
                usernameText.text = _account.displayName // we save the username in the display name.
                _ident.identifier = _account.identityIdentifier(__defaultServiceName) // if zero, will create new identity.
            } else if (status == Account.Synced) {
                pageStack.pop() // Success!
            } else if (status == Account.Error) {
                // display "error" dialog
                cleanup();
                pageStack.pop() // Failed.
            }
        }
    }

    property Identity _ident: Identity {
        identifier: root.accountId ? _account.identityIdentifier(__defaultServiceName) : 0
        identifierPending: root.accountId != 0

        onStatusChanged: {
            if (status == Identity.Initialized) {
                usernameText.text = userName
                passwordText.text = secret
                if (_needsCaption) {
                    captionText.text = caption
                }
            } else if (status == Identity.Synced) {
                _account.displayName = usernameText.text // XXX TODO: ensure this is correct...
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
            } else if (status == Identity.Error) {
                // display "error" dialog
                cleanup();
                pageStack.pop() // Failed.
            }
        }
    }
}
