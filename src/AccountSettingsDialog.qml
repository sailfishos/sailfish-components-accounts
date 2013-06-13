import QtQuick 2.0
import Sailfish.Silica 1.0
import org.nemomobile.accounts 1.0

Dialog {
    property Provider accountProvider
    property int accountId
    property bool isNewAccount

    property bool __sailfish_account_settings_dialog

    acceptDestinationAction: PageStackAction.Replace

    function _serviceTranslations() {
        // Name of the "Email" service type for an account.
        //% "Email"
        QT_TRID_NOOP("components_accounts-la-service_name_email")

        //: Describes the effect of enabling the "email" service for a particular user account.
        //% "Allow apps to send and receive email with your %1 account."
        QT_TRID_NOOP("components_accounts-la-service_description_email")

        // Name of the "Instant Messaging" service type for an account.
        //% "Instant Messaging"
        QT_TRID_NOOP("components_accounts-la-service_name_im")

        //: Describes the effect of enabling the "Instant Messaging" service for a particular user account.
        //% "Allow apps to chat using your %1 IM account and change your online status."
        QT_TRID_NOOP("components_accounts-la-service_description_im")

        // Name of the "Microblogging" service type (e.g. Twitter updates) for an account.
        //% "Microblogging"
        QT_TRID_NOOP("components_accounts-la-service_name_microblogging")

        //: Describes the effect of enabling the "Microblogging" service for a particular user account (e.g. to enable apps to post Twitter updates).
        //% "Allow apps to post and read your %1 updates."
        QT_TRID_NOOP("components_accounts-la-service_description_microblogging")

        // Name of the "Sharing" service type for an account (enables photo sharing, video sharing, etc.)
        //% "Sharing"
        QT_TRID_NOOP("components_accounts-la-service_name_sharing")

        //: Describes the effect of enabling the "Sharing" service for a particular user account.
        //% "Allow apps to show and update your %1 photos, videos and other content."
        QT_TRID_NOOP("components_accounts-la-service_description_sharing")

        // Name of the "Synchronization" service type for an account (enables contact synchronization, calendar synchronization, etc.)
        //% "Synchronization"
        QT_TRID_NOOP("components_accounts-la-service_name_sync")

        //: Describes the effect of enabling the "Synchronization" service for a particular user account.
        //% "Allow synchronization of contacts, calendar events and other details between %1 and your device."
        QT_TRID_NOOP("components_accounts-la-service_description_sync")
    }
}
