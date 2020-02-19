/*
 * Copyright (C) 2013-2019 Jolla Ltd.
 * Copyright (C) 2019 Open Mobile Platform LLC
 * Contact: Bea Lam <bea.lam@jollamobile.com>
 *
 * License: Proprietary
 */

#ifndef ACCOUNTSYNCPROFILEMANAGER_P_H
#define ACCOUNTSYNCPROFILEMANAGER_P_H

#include <QObject>
#include <QVariantMap>

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
class CloudBackupSyncTrigger;

class Q_DECL_EXPORT AccountSyncManager : public QObject
{
    Q_OBJECT
public:
    class BackupRestoreOptions
    {
    public:
        QStringList localDirFileNames() const;

        // DirListing: the local dir to which fileName will be saved with the directory listing text file.
        // Up/Download: The local dir to sync to/from.
        QString localDirPath;

        // DirListing: the remote dir for which the listing is to be fetched. Empty to fetch from root.
        // Up/Download: The remote dir to sync to/from. Empty to sync to/from default directories.
        QString remoteDirPath;

        // DirListing: the name of the file in localDirPath which will contain the directory listing data.
        // Up/Download: The local/remote file to be synced. Empty to sync all files in the local/remote dir.
        QString fileName;
    };

    enum SyncStatus {
        UnknownSyncStatus,
        SyncStarted,
        SyncFinished,
        SyncAborted,
        SyncError
    };
    Q_ENUM(SyncStatus)

    AccountSyncManager(QObject *parent = 0);
    ~AccountSyncManager();

    Q_INVOKABLE void createProfile(const QString &templateProfileName, int accountId, const QString &serviceName);
    Q_INVOKABLE void updateProfile(const QString &profileId, const QVariantMap &properties, AccountSyncOptions *options);
    Q_INVOKABLE void syncProfile(const QString &profileId);

    Q_INVOKABLE int createAllProfiles(int accountId);
    Q_INVOKABLE QStringList profileIds(int accountId, const QString &serviceName = QString()) const;
    Q_INVOKABLE AccountSyncOptions *accountSyncOptions(const QString &profileId);

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
    bool updateBackupRestoreOptions(const QString &profileId, const BackupRestoreOptions &options);

    QMap<QString, QString> profileProperties(const QString &profileId) const;
    QString syncScheduleXml(const QString &profileId) const;
    BackupRestoreOptions backupRestoreOptions(const QString &profileId, bool *ok) const;

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

    static QString backupDeviceName();

signals:
    void profileCreated(const QString &profileId);
    void profileCreationError(int accountId, const QString &serviceName, const QString &errorString);
    void profileUpdated(const QString &profileId);
    void profileUpdateError(const QString &profileId, const QString &errorString);
    void profileSyncStatusChanged(const QString &profileId, int status, const QString &errorString);

    void allProfilesCreated(int accountId, const QStringList &profileIds);
    void allProfileCreationError(int accountId, const QString &errorString);

private:
    friend class CloudBackupSyncTrigger;
    AccountSyncProfileManagerPrivate *d;
    Accounts::Manager *accountManager() const;
};

#endif
