/*
 * Copyright (c) 2014-2020 Jolla Ltd.
 * Copyright (c) 2020 Open Mobile Platform LLC.
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
        QueryServiceSettings,
        UpdateSyncServices,
        UpdateProviderAvailability,
        CreateProfiles,
        BackupAccounts,
        RestoreAccounts,
        TriggerProfiles,
        CreateAndTriggerProfiles,
        DeleteAccounts,
        MigrateCalDavPerProvider,
        MigrateCalDavPerProviderBackup,
        MigrateCalDavPerProviderRestore,
        RemoveProfile
    };

    QString providerName;
    QString serviceName;
    QString modeSwitch;
    QString settingName;
    QString settingType;
    QString settingValue;
    QString serviceType;
    QString backupFile;
    QString profileName;
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
    bool queryServiceSetting();
    bool applySyncUpdateChanges();
    bool applyProviderAvailabilityChanges();
    bool createProfiles(bool triggerSync);
    void addScheduledCommand(Mode command);
    QList<Mode> loadScheduledCommands(QFile *file);
    bool cdavAccountNeedsMigration(Accounts::Account *account);
    bool backupAccount(Accounts::Account *account);
    bool restoreAccounts();
    void triggerProfiles(Accounts::Account *account);
    bool profileDirReadable() const;
    void removeProfile();
    QString formatValue(const QString key) const;
    QString formatAllValues() const;

    static QString markerFilePath();

    Accounts::Manager *m_accountManager;
    Accounts::AccountIdList m_allAccountIds;
    Accounts::Account *m_currAccount;
    Buteo::SyncClientInterface *m_buteoClient;
    QFile *m_tempBackupFile;
    AccountSyncManager m_accountSyncManager;
    AccountBackupRestorer m_accountBackupRestorer;
    QList<Mode> m_commands;
    QList<uint> m_accountIdsToDelete;
    QMap<int, QMap<QString, QVariantMap> > m_restoredSyncProfileProperties;
    QMap<int, QMap<QString, QString> > m_restoredSyncScheduleXml;
    int m_currAccountIdx;
    bool m_migratingCalDav;
    bool m_error;
};

#endif
