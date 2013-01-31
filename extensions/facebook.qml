import QtQuick 1.1
import Sailfish.Silica 1.0
import com.jolla.components.accounts 1.0
import org.nemomobile.accounts 1.0
import org.nemomobile.signon 1.0

OAuthAccountCreationPage {
    _needsMechParamsAndSettings: false
    _needsCaption: false

    // FOR TESTING PURPOSES ONLY
    _signonSessionData: {"ClientId":"122816351223028", "ClientSecret":"3fc2a0e80c67f8343dc38ee8ef9dd2ff", "Scope":"xmpp_login"}
    _signonServiceName: "facebook-im" // we will override the Scope to request access to everything
}
