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

    am.providerName = args.value(2);
    am.serviceName = args.value(4);
    am.modeSwitch = args.value(5);
    am.settingName = args.value(6);
    am.settingType = args.value(7);
    am.settingValue = args.value(8);

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
            "Valid setting types are: %2\n\n")
        .arg(appName).arg(validSettingTypes.join(','));

    if ((providerSwitch == QString::fromLatin1("-p")) &&
        (serviceSwitch == QString::fromLatin1("-s")) &&
        (!am.settingName.isEmpty()) &&
        ((am.modeSwitch == QString::fromLatin1("-r") && argc == 7) ||
         ((am.modeSwitch == QString::fromLatin1("-m") && argc == 9 &&
                 validSettingTypes.contains(am.settingType))))) {
        QObject::connect(&am, SIGNAL(done()), &qca, SLOT(quit()));
        QTimer::singleShot(5, &am, SLOT(start()));
        qca.exec();
        return 0;
    }

    qWarning() << usageString;
    ::exit(1);
}