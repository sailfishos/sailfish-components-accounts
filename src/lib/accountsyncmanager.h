/****************************************************************************************
** Copyright (c) 2013 - 2023 Jolla Ltd.
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

#ifndef ACCOUNTSYNCPROFILEMANAGER_P_H
#define ACCOUNTSYNCPROFILEMANAGER_P_H

#include <QObject>
#include <QVariantMap>
#include <QDateTime>

#include <Accounts/AccountService>

namespace Buteo {
    class SyncProfile;
}

namespace Accounts {
    class Account;
    class Manager;
}

class AccountSyncProfileManagerPrivate;
class AccountSyncOptions;

class Q_DECL_EXPORT AccountSyncManager : public QObject
{
    Q_OBJECT
public:
    enum SyncStatus {
        UnknownSyncStatus,
        SyncStarted,
        SyncFinished,
        SyncAborted,
        SyncError
    };
    Q_ENUM(SyncStatus)

    enum BackupOperationType {
        InvalidOperation,
        Backup,
        BackupQuery,
        BackupRestore
    };
    Q_ENUM(BackupOperationType)

    AccountSyncManager(QObject *parent = 0);
    ~AccountSyncManager();

    Q_INVOKABLE void createProfile(const QString &templateProfileName, int accountId, const QString &serviceName);
    Q_INVOKABLE void updateProfile(const QString &profileId, const QVariantMap &properties, AccountSyncOptions *options);
    Q_INVOKABLE void syncProfile(const QString &profileId);
    Q_INVOKABLE void abortProfileSync(const QString &profileId);

    Q_INVOKABLE int createAllProfiles(int accountId);
    Q_INVOKABLE QStringList profileIds(int accountId, const QString &serviceName = QString()) const;
    Q_INVOKABLE AccountSyncOptions *accountSyncOptions(const QString &profileId);

    Q_INVOKABLE QDateTime nextSyncTime(const QString &profileId);
    Q_INVOKABLE QDateTime lastSyncTime(const QString &profileId);
    Q_INVOKABLE QDateTime lastSuccessfulSyncTime(const QString &profileId);

    Q_INVOKABLE bool templateProfilesAvailable(const QStringList &templateProfiles) const;
    Q_INVOKABLE QStringList defaultTemplateProfiles(int accountId, const QString &serviceName) const;

    QString createProfile(const QString &templateProfileName,
                          Accounts::Account *account,
                          const Accounts::Service &srv,
                          bool enableProfile,
                          const QVariantMap &properties = QVariantMap());
    bool checkProfile(const QString &templateProfileName,
                      Accounts::Account *account,
                      const Accounts::Service &srv);
    bool updateSyncProfile(const QString &profileId, const QVariantMap &properties, AccountSyncOptions *options);

    Q_INVOKABLE QVariantMap profileProperties(const QString &profileId) const;
    QString syncScheduleXml(const QString &profileId) const;

    Q_INVOKABLE QString findBackupOperationProfile(int accountId, BackupOperationType operation);
    static BackupOperationType backupOperationTypeForProfileId(const QString &profileId);

    QString accountDisplayName(int accountId);

    bool hasProfile(Accounts::Account *account, const Accounts::Service &srv) const;
    bool hasProfile(Accounts::Account *account, const Accounts::Service &srv, const QString &templateProfile) const;
    QStringList defaultTemplateProfiles(Accounts::Account *account, const Accounts::Service &srv) const;

    Buteo::SyncProfile *newProfileFromTemplate(const QString &templateProfileName,
                                               Accounts::Account *account,
                                               const Accounts::Service &srv,
                                               bool enableProfile,
                                               const QVariantMap &properties = QVariantMap());
    Buteo::SyncProfile *newProfileFromTemplate(const QString &templateProfileName,
                                               Accounts::Account *account,
                                               const Accounts::Service &srv,
                                               bool enableProfile,
                                               const QVariantMap &properties,
                                               const QString &scheduleXml);

signals:
    void profileCreated(const QString &profileId);
    void profileCreationError(int accountId, const QString &serviceName, const QString &errorString);
    void profileUpdated(const QString &profileId);
    void profileUpdateError(const QString &profileId, const QString &errorString);
    void profileSyncStatusChanged(const QString &profileId, int status, const QString &errorString);

    void allProfilesCreated(int accountId, const QStringList &profileIds);
    void allProfileCreationError(int accountId, const QString &errorString);

private:
    AccountSyncProfileManagerPrivate *d;
    Accounts::Manager *accountManager() const;
};

#endif
