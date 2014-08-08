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
#include <QStandardPaths>

#include "accountmodifier_p.h"

static bool isInstalledApp(const QString &appName)
{
    return !QStandardPaths::locate(QStandardPaths::ApplicationsLocation, appName + QStringLiteral(".desktop")).isEmpty();
}

Q_DECL_EXPORT int main(int argc, char *argv[])
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
            "%1 --provider-available <true|false> -p <provider> [-t <service-type>]\n"
            "(For example, --provider-available false -p onlinesync -t caldav)\n"
            "\n"
            "To do as per the provider-available command, but set to to true/false depending on whether a specific app is installed:\n"
            "%1 --provider-available -a <app-name> -p <provider> [-t <service-type>]\n"
            "(For example, --provider-available -a jolla-calendar -p onlinesync -t caldav)\n"
            "\n"
            "To create profiles for all account services that require them: (i.e. services that specify profile templates but do not have the created profiles)\n"
            "%1 --create-profiles\n"
            "Note: msyncd must be running for this command to work!\n"
            "Or, to schedule this command to be run on next boot:\n"
            "%1 --create-profiles-on-boot\n"
            "\n"
            "To trigger all synchronization profiles of all accounts:\n"
            "%1 --trigger-profiles\n"
            "\n"
            "To backup all account information to the specified output file:\n"
            "%1 --backup-accounts <outfile>\n"
            "To restore accounts which were previously backed up to the specified input file:\n"
            "%1 --restore-accounts <infile>\n"
            "\n"
            "To run all command scheduled for the next boot:\n"
            "%1 --run-scheduled-commands\n"
            "\n"
            "Valid setting types are: %2\n\n")
        .arg(appName).arg(validSettingTypes.join(','));

    bool validParams = true;
    QString firstOption = args.value(1);
    bool scheduleForNextBoot = firstOption.endsWith(QStringLiteral("-on-boot"));
    if (firstOption.startsWith(QStringLiteral("--update-sync-services"))) {
        am.mode = AccountModifier::UpdateSyncServices;
        am.scheduleCommandForNextBoot = scheduleForNextBoot;
    } else if (firstOption == QStringLiteral("--provider-available")) {
        am.mode = AccountModifier::UpdateProviderAvailability;
        QString appOrAvailableStatus = args.value(2);
        int providerIndex = 0;
        if (appOrAvailableStatus == QStringLiteral("-a")) {
            am.providerAvailable = isInstalledApp(args.value(3));
            providerIndex = 5;
        } else {
            am.providerAvailable = (appOrAvailableStatus.toLower() == QStringLiteral("true"));
            providerIndex = 4;
        }
        validParams = args.value(providerIndex-1) == QStringLiteral("-p")
                && (args.length() == providerIndex+1 || args.value(providerIndex+1) == QStringLiteral("-t"));
        if (validParams) {
            am.providerName = args.value(providerIndex);
            am.serviceType = args.value(providerIndex + 2);
        }
    } else if (firstOption.startsWith(QStringLiteral("--create-profiles"))) {
        am.mode = AccountModifier::CreateProfiles;
        am.scheduleCommandForNextBoot = scheduleForNextBoot;
    } else if (firstOption == QStringLiteral("--run-scheduled-commands")) {
        am.runScheduledCommands = true;
    } else if (firstOption == QStringLiteral("--backup-accounts")) {
        am.mode = AccountModifier::BackupAccounts;
        am.backupFile = args.value(2);
    } else if (firstOption == QStringLiteral("--restore-accounts")) {
        am.mode = AccountModifier::RestoreAccounts;
        am.backupFile = args.value(2);
    } else if (firstOption == QStringLiteral("--trigger-profiles")) {
        am.mode = AccountModifier::TriggerProfiles;
    } else {
        am.mode = AccountModifier::ModifyServiceSettings;
        am.providerName = args.value(2);
        am.serviceName = args.value(4);
        am.modeSwitch = args.value(5);
        am.settingName = args.value(6);
        am.settingType = args.value(7);
        am.settingValue = args.value(8);
    }

    if (am.mode == AccountModifier::ModifyServiceSettings) {
        validParams = ((providerSwitch == QString::fromLatin1("-p")) &&
            (serviceSwitch == QString::fromLatin1("-s")) &&
            (!am.settingName.isEmpty()) &&
            ((am.modeSwitch == QString::fromLatin1("-r") && argc == 7) ||
             ((am.modeSwitch == QString::fromLatin1("-m") && argc == 9 &&
                     validSettingTypes.contains(am.settingType)))));
    } else if (am.mode == AccountModifier::BackupAccounts ||
               am.mode == AccountModifier::RestoreAccounts) {
        validParams = !am.backupFile.isEmpty();
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
