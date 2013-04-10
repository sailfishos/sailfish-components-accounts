import QtQuick 1.1
import Sailfish.Silica 1.0
import org.nemomobile.accounts 1.0
import org.nemomobile.signon 1.0
import com.jolla.components.accounts.private 1.0

AccountAuthenticator {
    id: root

    // The following are for extension plugins to set.
    property Dialog _consentDialog      // Dialog requesting acceptance of terms and conditions etc
    property variant _signonSessionData // ClientId/ClientKey, ConsumerKey/ConsumerSecret, etc
    property string _signonServiceName  // Which service should be signed onto by default
    // Extension plugins may also implement: function postSignIn(variant signInResponseData) {}
    // If they implement this function, they _must_ call the postSignInFinished() function when finished.
    // The implementation of postSignIn may (for example) set the user name appropriately.

    //--------------------------------

    property string __errorMessage
    property bool __needsConsentDialogPush: false
    property bool __needsConsentDialogReject: false
    property bool __needsConsentDialogAccept: false

    anchors.fill: parent

    // Note: we can't set forwardNavigation: false otherwise the reject/pop navigation also fails...
    Component.onCompleted: {
        root.dialog.canAccept = false
        if (_consentDialog == null) {
            root.opacity = 1.0
            root.dialog.opacity = 1.0
            accountFactory.beginCreation()
        } else {
            root.opacity = 0.0
            root.dialog.opacity = 0.0
            __needsConsentDialogPush = true
        }
    }

    Connections {
        target: root.dialog
        onStatusChanged: handleConsentDialogTransitions()
    }

    function handleConsentDialogTransitions() {
        if (root.dialog.status === PageStatus.Active) {
            if (__needsConsentDialogPush) {
                __needsConsentDialogPush = false
                _consentDialog.accepted.connect(function() {
                    root.__needsConsentDialogAccept = true
                })
                _consentDialog.rejected.connect(function() {
                    root.dialog.opacity = 0.0
                    root.opacity = 0.0
                    root.__needsConsentDialogReject = true
                })
                root.dialog.opacity = 1.0
                root.opacity = 1.0
                pageStack.push(_consentDialog, {}, PageStackAction.Immediate)
            } else if (__needsConsentDialogAccept) {
                __needsConsentDialogAccept = false
                accountFactory.beginCreation()
            } else if (__needsConsentDialogReject) {
                __needsConsentDialogReject = false
                root.dialog.opacity = 0.0
                root.opacity = 0.0
                root.dialog.reject() // would be nice if we could use Immediate but that causes a flicker
            }
        }
    }

    Component.onDestruction: {
        jolla_signon_ui_service.inProcessParent = null // no longer servicing signon requests
    }

    Connections {
        target: root.dialog
        onRejected: {
            accountFactory.cancel()
        }
    }

    Item {
        id: container
        anchors.fill: root

        DialogHeader {
            id: pageHeader
            //: Title of page for signing into a user account
            //% "Authentication"
            acceptText: qsTrId("components_accounts-he-oauth_authentication")
        }

        Column {
            anchors.centerIn: parent
            width: parent.width - theme.paddingLarge*2
            spacing: theme.paddingMedium

            Label {
                id: activityLabel

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

    AccountFactory {
        id: accountFactory

        function beginCreation() {
            // pass through the signon session data from the extension ui
            var params = {}
            for (var i in _signonSessionData) {
                params[i] = _signonSessionData[i]
            }

            // also ensure that we set up embedding / etc correctly:
            params["Title"] = root.provider.displayName
            params["InProcessServiceName"] = jolla_signon_ui_service.inProcessServiceName
            params["InProcessObjectPath"] = jolla_signon_ui_service.inProcessObjectPath
            jolla_signon_ui_service.inProcessParent = webViewContainer

            // and trigger signon / account creation
            accountFactory.createOAuthAccount(root.provider.name, _signonServiceName, params)
        }

        onError: {
            root.__errorMessage = message
            // the user must backstep now that an error has occurred.
        }

        onSuccess: {
            root.accountId = newAccountId // signal parameter
            postSignIn(responseData)      // call the post-sign-in hook
        }
    }

    function postSignIn(data) {
        // default implementation just calls finished function
        postSignInFinished()
    }

    function postSignInFinished() {
        accountFactory.signOut()
        root.dialog.canAccept = true
        root.dialog.accept()
    }
}
