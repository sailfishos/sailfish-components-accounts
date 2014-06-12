/*
** Utility to ease migration of existing accounts
** Copyright (C) 2014 Jolla Ltd.
*/

#include <QtDebug>

#include <QCoreApplication>
#include <QVariantMap>
#include <QMap>
#include <QVariant>
#include <QStringList>
#include <QString>
#include <QTimer>

#include "accountmodifier_p.h"

int main(int argc, char *argv[])
{
    QCoreApplication qca(argc, argv);
    AccountModifier am;
    QStringList args = qca.arguments();

    QString appName = args.value(0);
    QString providerSwitch = args.value(1);
    QString serviceSwitch = args.value(3);

    QStringList validSettingTypes;
    validSettingTypes << "bool" << "string" << "int" << "uint" << "longlong" << "ulonglong";

    QString usageString = QString::fromLatin1(
            "Usage:\n"
            "To modify the value of a setting, use -m switch:\n"
            "%1 -p <provider_name> -s <service_name> -m <setting_name> <setting_type> <setting_value>\n"
            "To remove a setting, use -r switch:\n"
            "%1 -p <provider_name> -s <service_name> -r <setting_name>\n"
            "Examples:\n"
            "1) Adding a new boolean property 'new_property' to the 'sync' service of all facebook accounts:\n"
            "%1 -p facebook -s sync -m new_property bool true\n"
            "2) Modifying 'existing_property' value to false for the 'sync' service of all facebook accounts:\n"
            "%1 -p facebook -s sync -m existing_property bool false\n"
            "3) Removing 'obsolete_property' for the 'sync' service of all facebook accounts:\n"
            "%1 -p facebook -s sync -r obsolete_property\n"
            "4) Modifying 'existing property' for the global service of all facebook accounts:\n"
            "%1 -p facebook -s --global -m existing_property bool false \n"
            "\n"
            "To update per-data sync services to have the correct sync settings and sync profiles:\n"
            "%1 --update-sync-services\n"
            "Note: msyncd must be running for this command to work!\n"
            "Or, to schedule this command to be run on next boot:\n"
            "%1 --update-sync-services-on-boot\n"
            "\n"
            "To update the provider-available and enabled status of all accounts of type <provider> with a particular service that is enabled:\n"
            "%1 --provider-available false -p <provider> [-t <service-type>]\n"
            "(For example, --provider-available false -p onlinesync -t caldav)\n"
            "\n"
            "To create profiles for all account services that require them: (i.e. services that specify profile templates but do not have the created profiles)\n"
            "%1 --create-profiles\n"
            "Note: msyncd must be running for this command to work!\n"
            "Or, to schedule this command to be run on next boot:\n"
            "%1 --create-profiles-on-boot\n"
            "\n"
            "To run all command scheduled for the next boot:\n"
            "%1 --run-scheduled-commands\n"
            "\n"
            "Valid setting types are: %2\n\n")
        .arg(appName).arg(validSettingTypes.join(','));

    QString firstOption = args.value(1);
    bool scheduleForNextBoot = firstOption.endsWith(QStringLiteral("-on-boot"));
    if (firstOption.startsWith(QStringLiteral("--update-sync-services"))) {
        am.mode = AccountModifier::UpdateSyncServices;
        am.scheduleCommandForNextBoot = scheduleForNextBoot;
    } else if (firstOption == QStringLiteral("--provider-available")) {
        am.mode = AccountModifier::UpdateProviderAvailability;
        am.providerAvailable = args.value(2) == QStringLiteral("true");
        am.providerName = args.value(4);
        am.serviceType = args.value(6);
    } else if (firstOption.startsWith(QStringLiteral("--create-profiles"))) {
        am.mode = AccountModifier::CreateProfiles;
        am.scheduleCommandForNextBoot = scheduleForNextBoot;
    } else if (firstOption == QStringLiteral("--run-scheduled-commands")) {
        am.runScheduledCommands = true;
    } else {
        am.mode = AccountModifier::ModifyServiceSettings;
        am.providerName = args.value(2);
        am.serviceName = args.value(4);
        am.modeSwitch = args.value(5);
        am.settingName = args.value(6);
        am.settingType = args.value(7);
        am.settingValue = args.value(8);
    }

    bool validParams = true;
    if (am.mode == AccountModifier::UpdateProviderAvailability) {
        validParams = args.value(3) == QStringLiteral("-p")
                && (args.length() == 5 || args.value(5) == QStringLiteral("-t"));
    } else if (am.mode == AccountModifier::ModifyServiceSettings) {
        validParams = ((providerSwitch == QString::fromLatin1("-p")) &&
            (serviceSwitch == QString::fromLatin1("-s")) &&
            (!am.settingName.isEmpty()) &&
            ((am.modeSwitch == QString::fromLatin1("-r") && argc == 7) ||
             ((am.modeSwitch == QString::fromLatin1("-m") && argc == 9 &&
                     validSettingTypes.contains(am.settingType)))));
    }

    if (validParams) {
        QObject::connect(&am, SIGNAL(done()), &qca, SLOT(quit()));
        QTimer::singleShot(5, &am, SLOT(start()));
        qca.exec();
        if (am.errorOccurred()) {
            return -1;
        }
        return 0;
    }

    qWarning() << usageString;
    ::exit(1);
}
