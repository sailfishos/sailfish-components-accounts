/*
 * Copyright (C) 2015-2019 Jolla Ltd.
 * Copyright (C) 2019 Open Mobile Platform LLC
 * Contact: Chris Adams <chris.adams@jollamobile.com>
 *
 * License: Proprietary
 */

#include "cloudbackupsynctrigger.h"
#include "globalaccountmanager_p.h"

#include <QFile>
#include <QFileInfo>
#include <QVariantMap>
#include <QVariantList>
#include <QStandardPaths>
#include <QtDebug>

// Buteo
#include <ProfileEngineDefs.h>

// libaccounts-qt5
#include <Accounts/Manager>
#include <Accounts/Account>
#include <Accounts/Service>

// libsailfishkeyprovider
#include <sailfishkeyprovider.h>

namespace {

const QString DefaultLocalPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
        + QStringLiteral("/.local/share/system/privileged/Backups/cloud");

const QString ProfileMarkerBackup = QStringLiteral("Backup");
const QString ProfileMarkerBackupQuery = QStringLiteral("BackupQuery");
const QString ProfileMarkerBackupRestore = QStringLiteral("BackupRestore");

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
    options.localDirPath = localDir.isEmpty() ? DefaultLocalPath : localDir;
    options.remoteDirPath = remotePath;
    options.fileName = remoteFile;

    return startOperation(accountId, BackupRestore, options);
}

bool CloudBackupSyncTrigger::uploadToCloud(int accountId, const QString &localDir, const QString &remotePath)
{
    AccountSyncManager::BackupRestoreOptions options;
    options.localDirPath = localDir.isEmpty() ? DefaultLocalPath : localDir;
    options.remoteDirPath = remotePath;
    // No need to set options.fileName, just sync the entire local dir.

    return startOperation(accountId, Backup, options);
}

bool CloudBackupSyncTrigger::requestListing(int accountId)
{
    AccountSyncManager::BackupRestoreOptions options;
    options.localDirPath = DefaultLocalPath;
    // use default empty remoteDirPath to get listing from root dir.
    options.fileName = QStringLiteral("directoryListing.txt");

    return startOperation(accountId, BackupQuery, options);
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

bool CloudBackupSyncTrigger::startOperation(int accountId, Operation operation, const AccountSyncManager::BackupRestoreOptions &options)
{
    if (!m_currentSyncProfileId.isEmpty()) {
        qWarning() << "Currently busy syncing:" << m_currentSyncProfileId;
        return false;
    }

    // find appropriate sync profile for account
    QString syncProfileId;
    const QStringList profileIds = m_accountSyncManager->profileIds(accountId);
    Q_FOREACH (const QString &profileId, profileIds) {
        if (profileMatchesBackupOperation(profileId, operation)) {
            syncProfileId = profileId;
            break;
        }
    }

    if (syncProfileId.isEmpty()) {
        syncProfileId = createProfile(accountId, operation);
        if (syncProfileId.isEmpty()) {
            qWarning() << "Unable to create" << operation << "sync profile for account" << accountId;
            return false;
        }
    }

    if (operation == Backup) {
        QFileInfo fileInfo(options.localDirPath);
        if (!fileInfo.exists() || !fileInfo.isDir()) {
            qWarning() << "Local path:" << options.localDirPath << "does not exist or is not directory; cannot start sync!";
            return false;
        }
    }

    if (!m_accountSyncManager->updateBackupRestoreOptions(syncProfileId, options)) {
        qWarning() << "Cannot sync, unable to write backup/restore configuration for profile:" << syncProfileId;
        return false;
    }

    m_backupRestoreOptions = options;
    m_accountId = accountId;
    m_currentSyncProfileId = syncProfileId;

    // trigger sync
    m_accountSyncManager->syncProfile(m_currentSyncProfileId);

    return true;
}

QString CloudBackupSyncTrigger::createProfile(int accountId, Operation operation)
{
    Accounts::Manager *manager = globalAccountManager();
    Accounts::Account *account = manager->account(accountId);
    if (!account) {
        qWarning() << "No account found for account id" << accountId;
        return QString();
    }

    const QString serviceName = QString("%1-backup").arg(account->providerName());
    const QString templateProfileName = templateProfileForAccountOperation(account->providerName(), operation);

    return m_accountSyncManager->createProfile(templateProfileForAccountOperation(account->providerName(), operation),
                                               account,
                                               manager->service(serviceName),
                                               true);
}

void CloudBackupSyncTrigger::handleProfileSyncStatusChanged(const QString &profileId, int status, const QString &errorString)
{
    Operation operation = operationForProfileId(profileId);

    if (profileId == m_currentSyncProfileId) {
        // this is a status change to a profile we triggered.
        switch (status) {
        case AccountSyncManager::SyncStarted:
            qDebug() << "Started cloud" << operation << "operation with profile:"
                     << profileId;
            break;
        case AccountSyncManager::SyncFinished:
            qDebug() << "Successfully finished cloud" << operation
                     << "operation with profile:" << profileId;
            break;
        case AccountSyncManager::SyncError:
            qDebug() << "Error during cloud" << operation << "operation with profile:"
                     << profileId;
            break;
        case AccountSyncManager::SyncAborted:
            qDebug() << "Cloud" << operation << "operation with profile:" << profileId
                     << "aborted!";
            break;
        default:
            qWarning() << "Cloud" << operation << "operation with profile:" << profileId
                       << "has unknown status, ignoring signal...";
            return;
        }

        if (operation == CloudBackupSyncTrigger::BackupQuery
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

QString CloudBackupSyncTrigger::profileMarkerForOperation(CloudBackupSyncTrigger::Operation operation)
{
    switch (operation) {
    case CloudBackupSyncTrigger::InvalidOperation:
        return QString();
    case CloudBackupSyncTrigger::Backup:
        return ProfileMarkerBackup;
    case CloudBackupSyncTrigger::BackupQuery:
        return ProfileMarkerBackupQuery;
    case CloudBackupSyncTrigger::BackupRestore:
        return ProfileMarkerBackupRestore;
    }
    return QString();
}

bool CloudBackupSyncTrigger::profileMatchesBackupOperation(const QString &profileId, CloudBackupSyncTrigger::Operation operation)
{
    return profileId.contains(QString(".%1-").arg(profileMarkerForOperation(operation)));
}

QString CloudBackupSyncTrigger::templateProfileForAccountOperation(const QString &accountProviderName, CloudBackupSyncTrigger::Operation operation)
{
    return QString("%1.%2").arg(accountProviderName).arg(profileMarkerForOperation(operation));
}

CloudBackupSyncTrigger::Operation CloudBackupSyncTrigger::operationForProfileId(const QString &profileId)
{
    if (profileMatchesBackupOperation(profileId, Backup)) {
        return Backup;
    } else if (profileMatchesBackupOperation(profileId, BackupQuery)) {
        return BackupQuery;
    } else if (profileMatchesBackupOperation(profileId, BackupRestore)) {
        return BackupRestore;
    }
    return InvalidOperation;
}
