import QtQuick 1.1
import Sailfish.Silica 1.0
import org.nemomobile.accounts 1.0
import org.nemomobile.signon 1.0
import com.jolla.components.accounts.private 1.0

AccountAuthenticator {
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

    // Note: we can't set forwardNavigation: false otherwise the reject/pop navigation also fails...
    Component.onCompleted: {
        root.dialog.canAccept = false
        accountFactory.beginCreation()
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
            // XXX TODO: show error dialog.
            console.log("ERROR: " + message)
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
