/*
 * Copyright (C) 2015 Jolla Ltd.
 * Contact: Chris Adams <chris.adams@jollamobile.com>
 *
 * License: Proprietary
 */

#include "cloudbackupsynctrigger.h"
#include <QFileInfo>
#include <QtDebug>
#include <MGConfItem>
#include <ProfileEngineDefs.h> // Buteo

CloudBackupSyncTrigger::CloudBackupSyncTrigger(QObject *parent)
    : QObject(parent)
    , m_accountSyncManager(new AccountSyncManager(this))
{
    connect(m_accountSyncManager, SIGNAL(profileSyncStatusChanged(QString,int,QString)),
            this, SLOT(handleProfileSyncStatusChanged(QString,int,QString)));
}

bool CloudBackupSyncTrigger::syncWithCloud(int accountId, const QString &localDir, const QString &remotePath, const QString &direction)
{
    if (!m_currentSyncProfileId.isEmpty()) {
        qWarning() << "Currently busy syncing:" << m_currentSyncProfileId;
        return false;
    }

    // find appropriate sync profile for account
    QString cloudName;
    QString backupSyncProfile;
    QStringList profileIds = m_accountSyncManager->profileIds(accountId);
    Q_FOREACH (const QString &profileId, profileIds) {
        if (profileId.contains("backup", Qt::CaseInsensitive)) {
            // have valid backup sync profile
            backupSyncProfile = profileId;
            if (backupSyncProfile.contains("onedrive", Qt::CaseInsensitive)) {
                cloudName = QStringLiteral("OneDrive");
            } else if (backupSyncProfile.contains("dropbox", Qt::CaseInsensitive)) {
                cloudName = QStringLiteral("Dropbox");
            }
            break;
        }
    }

    if (backupSyncProfile.isEmpty() || cloudName.isEmpty()) {
        qWarning() << "No valid cloud storage profile for account:" << accountId << ":" << profileIds;
        return false;
    }

    // set config keys as appropriate
    {
        static const QString vaultConfKeyTemplate = QStringLiteral("/SailfishOS/vault/%1/%2");

        QString localPath = localDir.size() ? localDir : QStringLiteral("/home/nemo/.local/share/system/privileged/Backups");
        if (direction == Buteo::VALUE_TO_REMOTE) {
            if (!QFileInfo::exists(localPath) || !QFileInfo(localPath).isDir()) {
                qWarning() << "Local path:" << localPath << "does not exist or is not directory; cannot upsync to cloud!";
                return false;
            }
        }

        // set local dir key
        MGConfItem localPathConf(vaultConfKeyTemplate.arg(cloudName).arg(QStringLiteral("localPath")));
        localPathConf.set(localPath);

        // set remote path key
        MGConfItem remotePathConf(vaultConfKeyTemplate.arg(cloudName).arg(QStringLiteral("remotePath")));
        remotePathConf.set(remotePath); // if empty, will be set empty.  Sync plugin will figure out what it should be.

        // set sync direction key
        MGConfItem directionConf(vaultConfKeyTemplate.arg(cloudName).arg(QStringLiteral("direction")));
        directionConf.set(direction);
    }

    // trigger sync
    m_currentSyncProfileId = backupSyncProfile;
    m_accountSyncManager->syncProfile(backupSyncProfile);
    return true;
}

bool CloudBackupSyncTrigger::downloadFromCloud(int accountId, const QString &localDir, const QString &remotePath)
{
    return syncWithCloud(accountId, localDir, remotePath, Buteo::VALUE_FROM_REMOTE);
}

bool CloudBackupSyncTrigger::uploadToCloud(int accountId, const QString &localDir, const QString &remotePath)
{
    return syncWithCloud(accountId, localDir, remotePath, Buteo::VALUE_TO_REMOTE);
}

void CloudBackupSyncTrigger::resetState()
{
    m_currentSyncProfileId.clear();
}

void CloudBackupSyncTrigger::handleProfileSyncStatusChanged(const QString &profileId, int status, const QString &errorString)
{
    if (profileId == m_currentSyncProfileId) {
        // this is a status change to a profile we triggered.
        switch (status) {
        case AccountSyncManager::SyncStarted:
            qDebug() << "Started cloud sync with profile:" << profileId;
            break;
        case AccountSyncManager::SyncFinished:
            qDebug() << "Successfully finished cloud sync with profile:" << profileId;
            m_currentSyncProfileId = QString(); // finished syncing this profile.
            break;
        case AccountSyncManager::SyncError:
            qDebug() << "Error during cloud sync with profile:" << profileId;
            m_currentSyncProfileId = QString(); // finished syncing this profile.
            break;
        case AccountSyncManager::SyncAborted:
            qDebug() << "Cloud sync with profile:" << profileId << "aborted!";
            m_currentSyncProfileId = QString(); // finished syncing this profile.
            break;
        default:
            qWarning() << "Cloud sync with profile:" << profileId << "has unknown status, ignoring signal...";
            return;
        }

        // re-emit with queued, to ensure that it happens after we return from the download/upload functions in error case.
        QMetaObject::invokeMethod(this,
                                  "cloudSyncProgress",
                                  Qt::QueuedConnection,
                                  Q_ARG(int, status),
                                  Q_ARG(QString, errorString));
    }
}
