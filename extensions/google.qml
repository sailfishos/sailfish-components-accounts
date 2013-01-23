import QtQuick 1.1
import com.jolla.components 1.0
import com.jolla.components.accounts 1.0
import org.nemomobile.accounts 1.0
import org.nemomobile.signon 1.0

OAuthAccountCreationPage {
    _needsMechParamsAndSettings: false
    _needsCaption: false

    // FOR TESTING PURPOSES ONLY
    _signonSessionData: {"ClientId":"785386950238.apps.googleusercontent.com", "ClientSecret":"AGvXY_RSUKMD3WYDVlwabcyF", "Scope":["https://docs.google.com/feeds/","https://www.googleapis.com/auth/googletalk","https://www.googleapis.com/auth/userinfo.email","https://www.googleapis.com/auth/userinfo.profile","https://picasaweb.google.com/data/","https://www.googleapis.com/auth/blogger","https://www.googleapis.com/auth/books","https://www.google.com/m8/feeds/","https://www.googleapis.com/auth/drive","https://www.googleapis.com/auth/drive.file","https://mail.google.com/mail/feed/atom","https://www.googleapis.com/auth/plus.me","https://spreadsheets.google.com/feeds/","https://www.googleapis.com/auth/tasks","https://gdata.youtube.com"]}
    _signonServiceName: "google-pim" // we override the Scope to request access to everything.
}
