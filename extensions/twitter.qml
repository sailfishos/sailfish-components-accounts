import QtQuick 1.1
import Sailfish.Silica 1.0
import com.jolla.components.accounts 1.0
import org.nemomobile.accounts 1.0
import org.nemomobile.signon 1.0

OAuthAccountCreationPage {
    _needsMechParamsAndSettings: false
    _needsCaption: false

    // FOR TESTING PURPOSES ONLY
    _signonSessionData: {"ConsumerKey":"FxVs00m3hPvdC4tpla1yHA", "ConsumerSecret":"yxfwTU17VXcrtYPqWD941bQRsaHvgKdje6RlqOq07yA"}
    _signonServiceName: "twitter-microblog"
    _signonUserNameKey: "ScreenName"
}
