/*
** Copyright (C) 2014 Jolla Ltd.
*/

#ifndef ACCOUNTMODIFIER_P_H
#define ACCOUNTMODIFIER_P_H

#include <QObject>
#include <QString>
#include <QMutex>

#include <Accounts/Error>
#include <Accounts/Account>
#include <Accounts/Manager>

#include <SignOn/Identity>
#include <SignOn/IdentityInfo>
#include <SignOn/SessionData>
#include <SignOn/AuthSession>

#include <buteosyncfw5/SyncClientInterface.h>

#include "accountsyncmanager.h"
#include "accountbackuprestorer_p.h"

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
        CreateProfiles,
        BackupAccounts,
        RestoreAccounts,
        TriggerProfiles
    };

    QString providerName;
    QString serviceName;
    QString modeSwitch;
    QString settingName;
    QString settingType;
    QString settingValue;
    QString serviceType;
    QString backupFile;
    bool providerAvailable;
    bool scheduleCommandForNextBoot;
    bool runScheduledCommands;
    bool runningMultipleCommands;
    Mode mode;

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
    bool backupAccount(Accounts::Account *account);
    bool restoreAccounts();
    void triggerProfiles(Accounts::Account *account);

    static QString markerFilePath();

    Accounts::Manager *m_accountManager;
    Accounts::AccountIdList m_allAccountIds;
    Accounts::Account *m_currAccount;
    Buteo::SyncClientInterface *m_buteoClient;
    AccountSyncManager m_accountSyncManager;
    AccountBackupRestorer m_accountBackupRestorer;
    QList<Mode> m_commands;
    QMap<int, QMap<QString, QVariantMap> > m_restoredSyncProfileProperties;
    int m_currAccountIdx;
    bool m_error;
};

#endif
