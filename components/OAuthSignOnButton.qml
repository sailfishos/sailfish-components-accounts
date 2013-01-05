import QtQuick 1.1
import com.jolla.components 1.0
import org.nemomobile.signon 1.0
import org.nemomobile.accounts 1.0

Button {
    id: root

    property string serviceName
    property int serviceAccountIdentifier
    property int serviceAccountIdentityIdentifier
    property variant sessionData // eg: ClientId, ClientSecret, etc.

    signal accessTokenReceived(string token, int expires)

    //--------------------------------------

    property bool _isInitialized: false

    onClicked: {
        if (_isInitialized) {
            var serviceAccount = acm.serviceAccount(serviceAccountIdentifier, serviceName)
            var adp = serviceAccount.authData.parameters
            for (var i in sessionData) {
                adp[i] = sessionData[i]
            }

            // begin sign on procedure.
            ident.signIn(serviceAccount.authData.method, serviceAccount.authData.mechanism, adp)
        }
    }

    AccountManager { id: acm }

    ServiceAccountIdentity {
        id: ident
        identifier: serviceAccountIdentityIdentifier
        onStatusChanged: {
            if (status == ServiceAccountIdentity.Initialized) {
                _isInitialized = true
            } else if (status == ServiceAccountIdentity.Error) {
                console.log("Error occurred during authentication: " + errorMessage)
            }
        }

        onResponseReceived: {
            root.accessTokenReceived(data["AccessToken"], data["ExpiresIn"])
        }
    }
}
