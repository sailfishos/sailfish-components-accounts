import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Silica.theme 1.0
import org.nemomobile.accounts 1.0
import org.nemomobile.signon 1.0
import Sailfish.Accounts.private 1.0

AccountCreationDialog {
    id: root

    // The following are for extension plugins to set.
    property variant _signonSessionData // ClientId/ClientKey, ConsumerKey/ConsumerSecret, etc
    property string _signonServiceName  // Which service should be signed onto by default

    // Extension plugins may also implement: function postSignIn(variant signInResponseData) {}
    // When OAuthAccountCreationDialog creates an account successfully, it will emit accountCreated()
    // with the new account ID, then call postSignIn() with the OAuth response parameters.
    // If an extension plugin implements postSignIn(), it _must_ call the postSignInFinished() function when finished.
    // The implementation of postSignIn may (for example) set the account user name appropriately.


    //--------------------------------

    function _creationFailed() {
        // if creation fails immediately we still have to wait for the page to become inactive first
        if (postCreationDialog === null) {
            postCreationDialogChanged.connect(_creationFailed)
            return
        }
        postCreationDialog.minimumBusyDuration = 0
        if (status === PageStatus.Active) {
            canAccept = true
            accept()
        }
        accountCreationError()
    }

    anchors.fill: parent
    canAccept: false

    onRejected: {
        accountFactory.cancel()
    }

    Component.onCompleted: {
        accountFactory.beginCreation()
    }

    DialogHeader {
        id: pageHeader
        //: Title of page for signing into a user account
        //% "Authentication"
        acceptText: qsTrId("components_accounts-he-oauth_authentication")
    }

    Column {
        anchors.centerIn: parent
        spacing: Theme.paddingLarge

        Label {
            id: statusLabel
            width: root.width - Theme.paddingLarge*2
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap

            //: Message displayed when waiting for authentication web page to be loaded
            //% "Loading web page..."
            text: qsTrId("components_accounts-la-oauth_loading_web_page")
        }

        BusyIndicator {
            id: busyIndicator
            anchors.horizontalCenter: parent.horizontalCenter
            size: BusyIndicatorSize.Large
            running: true
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

    AccountFactory {
        id: accountFactory

        function beginCreation() {
            // pass through the signon session data from the extension ui
            var params = {}
            for (var i in _signonSessionData) {
                params[i] = _signonSessionData[i]
            }

            // also ensure that we set up embedding / etc correctly:
            if (typeof jolla_signon_ui_service !== "undefined") {
                params["Title"] = root.accountProvider.displayName
                params["InProcessServiceName"] = jolla_signon_ui_service.inProcessServiceName
                params["InProcessObjectPath"] = jolla_signon_ui_service.inProcessObjectPath
                jolla_signon_ui_service.inProcessParent = webViewContainer
            }

            // and trigger signon / account creation
            accountFactory.createOAuthAccount(root.accountProvider.name, _signonServiceName, params)
        }

        onError: {
            console.log("OAuthAccountCreationDialog: error while creating",
                        root.accountProvider.name, "account:", message)
            _creationFailed()
        }

        onSuccess: {
            root.accountCreated(newAccountId)
            postSignIn(responseData)      // call the post-sign-in hook
        }

        onStartedSignon: {
            changeBannerTimer.running = true
        }

        property Timer changeBannerTimer: Timer {
            repeat: false
            interval: 4000 // enough time for the web page to load
            running: false
            onTriggered: {
                //: Message displayed when storing credentials after web page signon has succeeded
                //% "Storing credentials..."
                statusLabel = qsTrId("components_accounts-la-storing_credentials")
            }
        }
    }

    function postSignIn(data) {
        // default implementation just accepts to the postCreationDialog and calls postSignInFinished()
        if (status === PageStatus.Active) {
            canAccept = true
            accept()
        }
        postSignInFinished()
    }

    function postSignInFinished() {
        accountFactory.signOut()
    }
}
