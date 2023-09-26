/****************************************************************************************
** Copyright (c) 2014 - 2023 Jolla Ltd.
** Copyright (c) 2020 Open Mobile Platform LLC.
**
** All rights reserved.
**
** This file is part of Sailfish Accounts components package.
**
** You may use this file under the terms of BSD license as follows:
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
** 1. Redistributions of source code must retain the above copyright notice, this
**    list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above copyright notice,
**    this list of conditions and the following disclaimer in the documentation
**    and/or other materials provided with the distribution.
**
** 3. Neither the name of the copyright holder nor the names of its
**    contributors may be used to endorse or promote products derived from
**    this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
** SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
** CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
** OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
****************************************************************************************/

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
    static QString oldMarkerFilePath(); // FIXME: Remove when no longer needed

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
