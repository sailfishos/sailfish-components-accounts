import QtQuick 1.1
import Sailfish.Silica 1.0
import com.jolla.components.accounts 1.0
import org.nemomobile.accounts 1.0
import org.nemomobile.signon 1.0

OAuthAccountCreationPage {
    _needsMechParamsAndSettings: false
    _needsCaption: false

    // FOR TESTING PURPOSES ONLY
    _signonSessionData: {"ConsumerKey":"5ea4e103502e0c7d562e015d9cf78e6f", "ConsumerSecret":"a8b26581454fe8f4"}
    _signonServiceName: "flickr-sharing"
}
