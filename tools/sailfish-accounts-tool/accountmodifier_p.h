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

#include <buteosyncfw5/SyncClientInterface.h>

#include "accountsyncmanager.h"

class QFile;

class AccountModifier : public QObject
{
    Q_OBJECT

public:
    enum Mode {
        UnknownMode,
        ModifyServiceSettings,
        UpdateSyncServices,
        UpdateProviderAvailability,
        CreateProfiles
    };

    QString providerName;
    QString serviceName;
    QString modeSwitch;
    QString settingName;
    QString settingType;
    QString settingValue;
    QString serviceType;
    bool providerAvailable;
    bool scheduleCommandForNextBoot;
    bool runScheduledCommands;
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
    bool saveProfileViaMsyncd(Accounts::Account *account,
                              const Accounts::Service &srv,
                              Buteo::SyncProfile *profile,
                              const QString &templateProfile);
    bool applyServiceSettingChanges();
    bool applySyncUpdateChanges();
    bool applyProviderAvailabilityChanges();
    bool createProfiles();
    void addScheduledCommand(Mode command);
    QList<Mode> loadScheduledCommands(QFile *file);

    static QString markerFilePath();

    Accounts::Manager *m_accountManager;
    Accounts::AccountIdList m_allAccountIds;
    Accounts::Account *m_currAccount;
    Buteo::SyncClientInterface *m_buteoClient;
    QList<Mode> m_commands;
    int m_currAccountIdx;
    bool m_error;
};

#endif
