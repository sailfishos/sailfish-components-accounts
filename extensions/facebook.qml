import QtQuick 1.1
import com.jolla.components 1.0
import com.jolla.components.accounts 1.0
import org.nemomobile.accounts 1.0
import org.nemomobile.signon 1.0

OAuthAccountCreationPage {
    _needsMechParamsAndSettings: false
    _needsCaption: false

    // FOR TESTING PURPOSES ONLY
    _signonSessionData: {"ClientId":"122816351223028", "ClientSecret":"3fc2a0e80c67f8343dc38ee8ef9dd2ff", "Scope":["publish_stream","read_stream","status_update","user_photos","friends_photos","xmpp_login"]}
    _signonServiceName: "facebook-sharing" // we override the Scope to request access to everything
}
