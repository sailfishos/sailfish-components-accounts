/*
** Copyright (C) 2014 Jolla Ltd.
*/

#include "accountmodifier_p.h"
#include <Accounts/Manager>
#include <Accounts/Account>

#include <QtDebug>

// helper to perform conversion from argument passed via commandline into QVariant for storage/setting.
QVariant convertSettingValue(const QString &settingName, const QString &settingType, const QString &settingValue)
{
    QVariant variantSettingValue;
    if (settingType == QString::fromLatin1("bool")) {
        if (settingValue == QString::fromLatin1("false") ||
                settingValue == QString::fromLatin1("0")) {
            variantSettingValue = QVariant::fromValue<bool>(false);
        } else {
            variantSettingValue = QVariant::fromValue<bool>(true);
        }
    } else if (settingType == QString::fromLatin1("string")) {
        variantSettingValue = QVariant::fromValue<QString>(settingValue);
    } else if (settingType == QString::fromLatin1("int")) {
        int intSettingValue = settingValue.toInt();
        variantSettingValue = QVariant::fromValue<int>(intSettingValue);
    } else if (settingType == QString::fromLatin1("uint")) {
        uint uintSettingValue = settingValue.toUInt();
        variantSettingValue = QVariant::fromValue<uint>(uintSettingValue);
    } else if (settingType == QString::fromLatin1("longlong")) {
        qlonglong longlongSettingValue = settingValue.toLongLong();
        variantSettingValue = QVariant::fromValue<qlonglong>(longlongSettingValue);
    } else if (settingType == QString::fromLatin1("ulonglong")) {
        qulonglong ulonglongSettingValue = settingValue.toULongLong();
        variantSettingValue = QVariant::fromValue<qulonglong>(ulonglongSettingValue);
    } else {
        // not a valid setting type, return invalid variant.
    }

    return variantSettingValue;
}

void AccountModifier::start()
{
    Accounts::Manager mgr;
    Accounts::AccountIdList allAccounts = mgr.accountList();

    bool emptyProvider = providerName.isEmpty();
    bool emptyService = serviceName.isEmpty();
    bool modifyMode = modeSwitch == QString::fromLatin1("-m");

    if (emptyProvider && emptyService) {
        qWarning() << "Untested codepath: provider name and service name is empty!"
                   << settingName << "will be"
                   << (modifyMode ? "modified in" : "removed from")
                   << "all services of all accounts!";
    } else if (emptyProvider) {
        qWarning() << "Untested codepath: provider name is empty!"
                   << settingName << "will be"
                   << (modifyMode ? "modified in" : "removed from")
                   << serviceName << "of all accounts!";
    } else if (emptyService) {
        qWarning() << "Untested codepath: service name is empty!"
                   << settingName << "will be"
                   << (modifyMode ? "modified in" : "removed from")
                   << "all services of" << providerName << "accounts!";
    } else {
        qWarning() << settingName << "will be"
                   << (modifyMode ? "modified in" : "removed from")
                   << serviceName << "of" << providerName << "accounts!";
    }

    foreach (const Accounts::AccountId &accId, allAccounts) {
        Accounts::Account *acc = mgr.account(accId);
        if (providerName.isEmpty() || acc->providerName() == providerName) {
            QVariant variantSettingValue = convertSettingValue(settingName, settingType, settingValue);
            if (serviceName == QString::fromLatin1("--global")) {
                // modify/remove the global service setting only
                acc->selectService(Accounts::Service());
                if (modeSwitch == QString::fromLatin1("-r")) {
                    acc->remove(settingName);
                } else {
                    acc->setValue(settingName, variantSettingValue);
                }
            } else {
                // modify/remove the setting for a particular service or for all services
                Accounts::ServiceList allServices = acc->services();
                foreach (const Accounts::Service &srv, allServices) {
                    if (serviceName.isEmpty() || srv.name() == serviceName) {
                        acc->selectService(srv);
                        if (modeSwitch == QString::fromLatin1("-r")) {
                            acc->remove(settingName);
                        } else {
                            acc->setValue(settingName, variantSettingValue);
                        }
                        acc->selectService(Accounts::Service());
                    }
                }
            }
            acc->syncAndBlock();
        }
    }

    emit done();
}