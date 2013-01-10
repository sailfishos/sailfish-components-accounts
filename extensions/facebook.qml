import QtQuick 1.1
import com.jolla.components 1.0
import com.jolla.components.accounts 1.0
import org.nemomobile.accounts 1.0
import org.nemomobile.signon 1.0

OAuthAccountCreationPage {
    _needsMechParamsAndSettings: false
    _needsCaption: false

    // FOR TESTING PURPOSES ONLY - USES MEEGO CS and CId
    _signonSessionData: {"ClientId":"213156715390803", "ClientSecret":"bf89c2d9de5e929fe5c5921e9a1f2924"}
    _signonServiceName: "facebook-sharing"
}
