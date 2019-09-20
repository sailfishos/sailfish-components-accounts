/*
 * Copyright (C) 2015 Jolla Ltd.
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
    const QString OneDriveAccount = QStringLiteral("OneDrive");
    const QString DropboxAccount = QStringLiteral("Dropbox");

    const QString SyncConfKeyTemplate = QStringLiteral("/SailfishOS/vault/%1/%2");

    const QString OperationType = QStringLiteral("operationType");
    const QString OperationList = QStringLiteral("list");
    const QString OperationListResult = QStringLiteral("listResult");
    const QString OperationSync = QStringLiteral("sync");

    const QString DefaultLocalPath = QStringLiteral("/home/nemo/.local/share/system/privileged/Backups");
    const QString DefaultDirListLocalPath = DefaultLocalPath + "/directoryListing.txt";

    QString cloudServiceName(const QStringList &accountSyncProfileIds, QString *backupSyncProfile) {
        Q_FOREACH (const QString &profileId, accountSyncProfileIds) {
            if (profileId.contains("backup", Qt::CaseInsensitive)) {
                // have valid backup sync profile
                *backupSyncProfile = profileId;
                if (backupSyncProfile->contains("onedrive", Qt::CaseInsensitive)) {
                    return OneDriveAccount;
                } else if (backupSyncProfile->contains("dropbox", Qt::CaseInsensitive)) {
                    return DropboxAccount;
                }
                QString name = profileId.split('.').value(0);
                if (name.isEmpty()) {
                    qDebug() << "Cannot read cloud service name from profile ID" << profileId << ", ignoring.";
                }
                return name;
            }
        }
        return QString();
    }
}

CloudBackupSyncTrigger::CloudBackupSyncTrigger(QObject *parent)
    : QObject(parent)
    , m_accountSyncManager(new AccountSyncManager(this))
     , m_networkManager(new QNetworkAccessManager(this))
{
    m_defaultRemoteBackupsDirectory = QStringLiteral("Backups");
    connect(m_accountSyncManager, &AccountSyncManager::profileSyncStatusChanged,
            this, &CloudBackupSyncTrigger::handleProfileSyncStatusChanged);
}

QString CloudBackupSyncTrigger::defaultRemoteBackupsDirectory() const
{
    return m_defaultRemoteBackupsDirectory;
}

bool CloudBackupSyncTrigger::syncWithCloud(int accountId, const QString &localDir, const QString &remotePath, const QString &remoteFile, const QString &direction)
{
    if (!m_currentSyncProfileId.isEmpty()) {
        qWarning() << "Currently busy syncing:" << m_currentSyncProfileId;
        return false;
    }

    if (!initOperation(accountId, OperationSync)) {
        return false;
    }

    // set config keys as appropriate
    {
        QString localPath = localDir.size() ? localDir : DefaultLocalPath;
        if (direction == Buteo::VALUE_TO_REMOTE) {
            if (!QFileInfo::exists(localPath) || !QFileInfo(localPath).isDir()) {
                qWarning() << "Local path:" << localPath << "does not exist or is not directory; cannot upsync to cloud!";
                return false;
            }
        }

        // set local dir key
        MGConfItem localPathConf(SyncConfKeyTemplate.arg(m_cloudServiceName).arg(QStringLiteral("localPath")));
        localPathConf.set(localPath);

        // set remote path key
        MGConfItem remotePathConf(SyncConfKeyTemplate.arg(m_cloudServiceName).arg(QStringLiteral("remotePath")));
        remotePathConf.set(remotePath); // if empty, will be set empty.  Sync plugin will figure out what it should be.

        // set remote path key
        MGConfItem remoteFileConf(SyncConfKeyTemplate.arg(m_cloudServiceName).arg(QStringLiteral("remoteFile")));
        remoteFileConf.set(remoteFile); // if empty, will be set empty, which denotes "all files from the remote path"

        // set sync direction key
        MGConfItem directionConf(SyncConfKeyTemplate.arg(m_cloudServiceName).arg(QStringLiteral("direction")));
        directionConf.set(direction);
    }

    // trigger sync
    m_accountSyncManager->syncProfile(m_currentSyncProfileId);
    return true;
}

bool CloudBackupSyncTrigger::downloadFromCloud(int accountId, const QString &localDir, const QString &remotePath, const QString &remoteFile)
{
    return syncWithCloud(accountId, localDir, remotePath, remoteFile, Buteo::VALUE_FROM_REMOTE);
}

bool CloudBackupSyncTrigger::uploadToCloud(int accountId, const QString &localDir, const QString &remotePath)
{
    return syncWithCloud(accountId, localDir, remotePath, QString(), Buteo::VALUE_TO_REMOTE);
}

bool CloudBackupSyncTrigger::requestListing(int accountId)
{
    if (!m_currentSyncProfileId.isEmpty()) {
        qWarning() << "Currently busy syncing:" << m_currentSyncProfileId;
        return false;
    }

    if (!initOperation(accountId, OperationList)) {
        return false;
    }

    if (!QFileInfo::exists(DefaultLocalPath) || !QFileInfo(DefaultLocalPath).isDir()) {
        qWarning() << "Local path:" << DefaultLocalPath << "does not exist or is not directory; cannot request dir listing!";
        return false;
    }

    MGConfItem localPathConf(SyncConfKeyTemplate.arg(m_cloudServiceName).arg(QStringLiteral("localPath")));
    localPathConf.set(DefaultLocalPath);

    // Set key for saving the directory listing result
    QFile::remove(DefaultDirListLocalPath);
    MGConfItem listResultLocalPathConf(SyncConfKeyTemplate.arg(m_cloudServiceName)
                                       .arg(QStringLiteral("listResultLocalPath")));
    listResultLocalPathConf.set(DefaultDirListLocalPath);

    // trigger sync
    m_accountSyncManager->syncProfile(m_currentSyncProfileId);
    return true;
}

void CloudBackupSyncTrigger::processDirectoryListResults()
{
    QFile dirListingFile(DefaultDirListLocalPath);
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

    QFile::remove(DefaultDirListLocalPath);
    emit requestedListing(m_accountId, results);
    resetState();
}

void CloudBackupSyncTrigger::resetState()
{
    m_currentOperation.clear();
    m_accountId = -1;
    m_cloudServiceName.clear();
    m_currentSyncProfileId.clear();
}

bool CloudBackupSyncTrigger::initOperation(int accountId, const QString &operation)
{
    // find appropriate sync profile for account
    QString backupSyncProfile;
    QStringList profileIds = m_accountSyncManager->profileIds(accountId);
    QString cloudName = cloudServiceName(profileIds, &backupSyncProfile);
    if (backupSyncProfile.isEmpty() || cloudName.isEmpty()) {
        qWarning() << "No valid cloud storage profile for account:" << accountId << ":" << profileIds;
        return false;
    }

    // set operation type
    MGConfItem operationTypeConf(SyncConfKeyTemplate.arg(cloudName).arg(OperationType));
    operationTypeConf.set(operation);

    m_currentOperation = operation;
    m_accountId = accountId;
    m_cloudServiceName = cloudName;
    m_currentSyncProfileId = backupSyncProfile;
    return true;
}

void CloudBackupSyncTrigger::handleProfileSyncStatusChanged(const QString &profileId, int status, const QString &errorString)
{
    if (profileId == m_currentSyncProfileId) {
        // this is a status change to a profile we triggered.
        switch (status) {
        case AccountSyncManager::SyncStarted:
            qDebug() << "Started cloud" << m_currentOperation << "operation with profile:"
                     << profileId;
            break;
        case AccountSyncManager::SyncFinished:
            qDebug() << "Successfully finished cloud" << m_currentOperation
                     << "operation with profile:" << profileId;
            break;
        case AccountSyncManager::SyncError:
            qDebug() << "Error during cloud" << m_currentOperation << "operation with profile:"
                     << profileId;
            break;
        case AccountSyncManager::SyncAborted:
            qDebug() << "Cloud" << m_currentOperation << "operation with profile:" << profileId
                     << "aborted!";
            break;
        default:
            qWarning() << "Cloud" << m_currentOperation << "operation with profile:" << profileId
                       << "has unknown status, ignoring signal...";
            return;
        }

        if (m_currentOperation == OperationList && status == AccountSyncManager::SyncFinished) {
            processDirectoryListResults();
        } else if (m_currentOperation == OperationSync) {
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
