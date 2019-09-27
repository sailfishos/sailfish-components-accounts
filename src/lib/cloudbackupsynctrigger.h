/*
 * Copyright (C) 2015-2019 Jolla Ltd.
 * Copyright (C) 2019 Open Mobile Platform LLC
 * Contact: Chris Adams <chris.adams@jollamobile.com>
 *
 * License: Proprietary
 */

#ifndef CLOUDBACKUPSYNCTRIGGER_H
#define CLOUDBACKUPSYNCTRIGGER_H

#include "accountsyncmanager.h"

#include <QObject>
#include <QStringList>
#include <QNetworkAccessManager>

class Q_DECL_EXPORT CloudBackupSyncTrigger : public QObject
{
    Q_OBJECT
public:
    CloudBackupSyncTrigger(QObject *parent = 0);

    /* localDir and remotePath are optional.  The default localDir is /home/nemo/.local/share/system/privileged/Backups/ */
    Q_INVOKABLE bool downloadFromCloud(int accountId, const QString &localDir = QString(), const QString &remotePath = QString(), const QString &remoteFile = QString());
    Q_INVOKABLE bool uploadToCloud(int accountId, const QString &localDir = QString(), const QString &remotePath = QString());
    Q_INVOKABLE bool requestListing(int accountId);

    /* no signals for the previous operation will be emitted after calling this, but you can immediately start a new operation. */
    Q_INVOKABLE void resetState();

Q_SIGNALS:
    void cloudSyncProgress(int accountId, int status, const QString &errorString);
    void requestedListing(int accountId, const QVariantList &listing);
    void requestListingFailed(int accountId, const QString &message);

private:
    bool startOperation(int accountId, const AccountSyncManager::BackupRestoreOptions &options);
    bool syncWithCloud(int accountId, const QString &localDir, const QString &remotePath, const QString &remoteFile, const QString &direction);
    void performListingRequest(int accountId, const QString &accessToken, const QString &cloudName, const QString &remotePath);
    void processDirectoryListResults();
    void handleProfileSyncStatusChanged(const QString &profileId, int status, const QString &errorString);

    int m_accountId = -1;
    AccountSyncManager::BackupRestoreOptions m_backupRestoreOptions;
    QString m_currentSyncProfileId;
    QString m_defaultRemoteBackupsDirectory;
    QStringList m_deviceDirectories;
    QVariantList m_dirListing;
    AccountSyncManager *m_accountSyncManager = nullptr;
    QNetworkAccessManager *m_networkManager = nullptr;
};

#endif // CLOUDBACKUPSYNCTRIGGER_H
