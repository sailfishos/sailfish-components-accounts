import QtQuick 1.1
import Sailfish.Silica 1.0
import org.nemomobile.accounts 1.0
import org.nemomobile.signon 1.0
import Sailfish.Accounts.private 1.0

AccountCreationDialog {
    id: root

    // The following are for extension plugins to set.
    property variant _signonSessionData // ClientId/ClientKey, ConsumerKey/ConsumerSecret, etc
    property string _signonServiceName  // Which service should be signed onto by default

    // Extension plugins may also implement: function postSignIn(variant signInResponseData) {}
    // If they implement this function, they _must_ call the postSignInFinished() function when finished.
    // The implementation of postSignIn may (for example) set the user name appropriately.

    //--------------------------------

    property string __errorMessage

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
        width: parent.width - theme.paddingLarge*2
        spacing: theme.paddingMedium

        Label {
            id: activityLabel

            //: Message displayed when waiting for authentication web page to be loaded
            //% "Loading web page..."
            property string loadingString: qsTrId("components_accounts-la-oauth_loading_web_page")

            //: Message displayed when storing credentials after web page signon has succeeded
            //% "Storing credentials..."
            property string storingString: qsTrId("components_accounts-la-storing_credentials")

            //: Message displayed when error occurs during user authentication
            //% "Error while loading:"
            property string errorString: qsTrId("components_accounts-la-oauth_error")

            property bool hasLoadedWebPage: false
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            font.family: theme.fontFamilyHeading
            font.pixelSize: theme.fontSizeLarge
            text: root.__errorMessage === "" ? (hasLoadedWebPage ? storingString : loadingString) : errorString
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

    AccountFactory {
        id: accountFactory

        function beginCreation() {
            // pass through the signon session data from the extension ui
            var params = {}
            for (var i in _signonSessionData) {
                params[i] = _signonSessionData[i]
            }

            // also ensure that we set up embedding / etc correctly:
            params["Title"] = root.accountProvider.displayName
            params["InProcessServiceName"] = jolla_signon_ui_service.inProcessServiceName
            params["InProcessObjectPath"] = jolla_signon_ui_service.inProcessObjectPath
            jolla_signon_ui_service.inProcessParent = webViewContainer

            // and trigger signon / account creation
            accountFactory.createOAuthAccount(root.accountProvider.name, _signonServiceName, params)
        }

        onError: {
            root.__errorMessage = message
            // the user must backstep now that an error has occurred.
        }

        onSuccess: {
            // set the accountId for the settings page
            root.acceptDestinationInstance.accountId = newAccountId
            postSignIn(responseData)      // call the post-sign-in hook
        }

        onStartedSignon: {
            changeBannerTimer.running = true
        }

        property Timer changeBannerTimer: Timer {
            repeat: false
            interval: 4000 // enough time for the web page to load
            running: false
            onTriggered: activityLabel.hasLoadedWebPage = true // change the banner
        }
    }

    function postSignIn(data) {
        // default implementation just calls finished function
        postSignInFinished()
    }

    function postSignInFinished() {
        accountFactory.signOut()
        canAccept = true
        accept()
    }
}
