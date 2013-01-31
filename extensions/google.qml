import QtQuick 1.1
import Sailfish.Silica 1.0
import com.jolla.components.accounts 1.0
import org.nemomobile.accounts 1.0
import org.nemomobile.signon 1.0

OAuthAccountCreationPage {
    _needsMechParamsAndSettings: false
    _needsCaption: false

    // FOR TESTING PURPOSES ONLY
    _signonSessionData: {"ClientId":"785386950238.apps.googleusercontent.com", "ClientSecret":"AGvXY_RSUKMD3WYDVlwabcyF", "Scope":"https://www.googleapis.com/auth/googletalk"}
    _signonServiceName: "google-talk"
}
