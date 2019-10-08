/*
 * Copyright (C) 2015-2019 Jolla Ltd.
 * Copyright (C) 2019 Open Mobile Platform LLC
 * Contact: Chris Adams <chris.adams@jollamobile.com>
 *
 * License: Proprietary
 */

#include "cloudbackupsynctrigger.h"

#include <QFile>
#include <QFileInfo>
#include <QPair>
#include <QUrl>
#include <QUrlQuery>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QVariantMap>
#include <QVariantList>
#include <QtDebug>

// mlite5
#include <MGConfItem>

// Buteo
#include <ProfileEngineDefs.h>

// libaccounts-qt5
#include <Accounts/Manager>
#include <Accounts/Account>
#include <Accounts/Service>
#include <Accounts/AccountService>

// libsailfishkeyprovider
#include <sailfishkeyprovider.h>

namespace {
    const QString DefaultLocalPath = QStringLiteral("/home/nemo/.local/share/system/privileged/Backups");
}

CloudBackupSyncTrigger::CloudBackupSyncTrigger(QObject *parent)
    : QObject(parent)
    , m_accountSyncManager(new AccountSyncManager(this))
     , m_networkManager(new QNetworkAccessManager(this))
{
    connect(m_accountSyncManager, &AccountSyncManager::profileSyncStatusChanged,
            this, &CloudBackupSyncTrigger::handleProfileSyncStatusChanged);
}

bool CloudBackupSyncTrigger::downloadFromCloud(int accountId, const QString &localDir, const QString &remotePath, const QString &remoteFile)
{
    AccountSyncManager::BackupRestoreOptions options;
    options.operation = AccountSyncManager::BackupRestoreOptions::Download;
    options.localDirPath = localDir.isEmpty() ? DefaultLocalPath : localDir;
    options.remoteDirPath = remotePath;
    options.fileName = remoteFile;

    return startOperation(accountId, options);
}

bool CloudBackupSyncTrigger::uploadToCloud(int accountId, const QString &localDir, const QString &remotePath)
{
    AccountSyncManager::BackupRestoreOptions options;
    options.operation = AccountSyncManager::BackupRestoreOptions::Upload;
    options.localDirPath = localDir.isEmpty() ? DefaultLocalPath : localDir;
    options.remoteDirPath = remotePath;
    // No need to set options.fileName, just sync the entire local dir.

    return startOperation(accountId, options);
}

bool CloudBackupSyncTrigger::requestListing(int accountId)
{
    AccountSyncManager::BackupRestoreOptions options;
    options.operation = AccountSyncManager::BackupRestoreOptions::DirectoryListing;
    options.localDirPath = DefaultLocalPath;
    // use default empty remoteDirPath to get listing from root dir.
    options.fileName = QStringLiteral("directoryListing.txt");

    return startOperation(accountId, options);
}

void CloudBackupSyncTrigger::processDirectoryListResults()
{
    QFile dirListingFile(m_backupRestoreOptions.localDirPath + '/' + m_backupRestoreOptions.fileName);
    if (!dirListingFile.open(QFile::ReadOnly)) {
        emit requestListingFailed(m_accountId, "Failed to open: " + dirListingFile.fileName());
        return;
    }

    const QStringList fileNames = QString::fromUtf8(dirListingFile.readAll()).split('\n');
    QVariantList results;

    for (const QString &fileName : fileNames) {
        int lastSepIndex = fileName.lastIndexOf('/');
        QVariantMap data;
        if (lastSepIndex >= 0) {
            data.insert("parent", fileName.mid(0, lastSepIndex));
            data.insert("name", fileName.mid(lastSepIndex + 1));
        } else {
            data.insert("name", fileName);
        }
        results.append(data);
    }

    if (!dirListingFile.remove()) {
        qWarning() << "Unable to remove file during clean-up:" << dirListingFile.fileName();
    }
    emit requestedListing(m_accountId, results);
    resetState();
}

void CloudBackupSyncTrigger::resetState()
{
    m_accountId = -1;
    m_currentSyncProfileId.clear();
}

bool CloudBackupSyncTrigger::startOperation(int accountId, const AccountSyncManager::BackupRestoreOptions &options)
{
    if (!m_currentSyncProfileId.isEmpty()) {
        qWarning() << "Currently busy syncing:" << m_currentSyncProfileId;
        return false;
    }

    // find appropriate sync profile for account
    QString backupSyncProfile;
    const QStringList profileIds = m_accountSyncManager->profileIds(accountId);
    Q_FOREACH (const QString &profileId, profileIds) {
        if (profileId.contains("backup", Qt::CaseInsensitive)) {
            backupSyncProfile = profileId;
            break;
        }
    }

    if (backupSyncProfile.isEmpty()) {
        qWarning() << "No valid cloud storage profile found for account:" << accountId
                   << "out of profile ids:" << profileIds;
        return false;
    }

    if (options.operation == AccountSyncManager::BackupRestoreOptions::Upload) {
        QFileInfo fileInfo(options.localDirPath);
        if (!fileInfo.exists() || !fileInfo.isDir()) {
            qWarning() << "Local path:" << options.localDirPath << "does not exist or is not directory; cannot start sync!";
            return false;
        }
    }

    if (!m_accountSyncManager->updateBackupRestoreOptions(backupSyncProfile, options)) {
        qWarning() << "Cannot sync, unable to write backup/restore configuration for profile:" << backupSyncProfile;
        return false;
    }

    m_backupRestoreOptions = options;
    m_accountId = accountId;
    m_currentSyncProfileId = backupSyncProfile;

    // trigger sync
    m_accountSyncManager->syncProfile(m_currentSyncProfileId);

    return true;
}

void CloudBackupSyncTrigger::handleProfileSyncStatusChanged(const QString &profileId, int status, const QString &errorString)
{
    QString operationType;
    switch (m_backupRestoreOptions.operation) {
    case AccountSyncManager::BackupRestoreOptions::DirectoryListing:
        operationType = "directory listing";
        break;
    case AccountSyncManager::BackupRestoreOptions::Upload:
        operationType = "upload";
        break;
    case AccountSyncManager::BackupRestoreOptions::Download:
        operationType = "download";
        break;
    }

    if (profileId == m_currentSyncProfileId) {
        // this is a status change to a profile we triggered.
        switch (status) {
        case AccountSyncManager::SyncStarted:
            qDebug() << "Started cloud" << operationType << "operation with profile:"
                     << profileId;
            break;
        case AccountSyncManager::SyncFinished:
            qDebug() << "Successfully finished cloud" << operationType
                     << "operation with profile:" << profileId;
            break;
        case AccountSyncManager::SyncError:
            qDebug() << "Error during cloud" << operationType << "operation with profile:"
                     << profileId;
            break;
        case AccountSyncManager::SyncAborted:
            qDebug() << "Cloud" << operationType << "operation with profile:" << profileId
                     << "aborted!";
            break;
        default:
            qWarning() << "Cloud" << operationType << "operation with profile:" << profileId
                       << "has unknown status, ignoring signal...";
            return;
        }

        if (m_backupRestoreOptions.operation == AccountSyncManager::BackupRestoreOptions::DirectoryListing
                && status == AccountSyncManager::SyncFinished) {
            processDirectoryListResults();
        } else {
            // re-emit with queued, to ensure that it happens after we return from the download/upload functions in error case.
            QMetaObject::invokeMethod(this,
                                      "cloudSyncProgress",
                                      Qt::QueuedConnection,
                                      Q_ARG(int, m_accountId),
                                      Q_ARG(int, status),
                                      Q_ARG(QString, errorString));
        }

        if (status == AccountSyncManager::SyncFinished
                || status == AccountSyncManager::SyncError
                || status == AccountSyncManager::SyncAborted) {
            resetState();
        }
    }
}
