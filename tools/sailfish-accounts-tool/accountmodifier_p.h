/*
** Copyright (C) 2014 Jolla Ltd.
*/

#ifndef ACCOUNTMODIFIER_P_H
#define ACCOUNTMODIFIER_P_H

#include <QObject>
#include <QString>

#include <Accounts/Error>
#include <Accounts/Account>
#include <Accounts/Manager>

#include "accountsyncmanager.h"

class AccountModifier : public QObject
{
    Q_OBJECT

public:
    enum Mode {
        UnknownMode,
        ModifyServiceSettings,
        UpdateSyncServices
    };

    QString providerName;
    QString serviceName;
    QString modeSwitch;
    QString settingName;
    QString settingType;
    QString settingValue;
    Mode mode;
    AccountSyncManager accountSyncManager;

    AccountModifier(QObject *parent = 0);
    ~AccountModifier();

    bool errorOccurred() const;

public Q_SLOTS:
    void start();
    void next();
    void error(Accounts::Error err);

Q_SIGNALS:
    void done();

private:
    void checkServiceSettingArgs();
    bool applyServiceSettingChanges();
    bool applySyncUpdateChanges();

    Accounts::Manager *m_accountManager;
    Accounts::AccountIdList m_allAccountIds;
    Accounts::Account *m_currAccount;
    int m_currAccountIdx;
    bool m_error;
};

#endif
