/*
 * Copyright (C) 2015 Jolla Ltd.
 * Contact: Chris Adams <chris.adams@jollamobile.com>
 *
 * License: Proprietary
 */

#ifndef CLOUDBACKUPSYNCTRIGGER_H
#define CLOUDBACKUPSYNCTRIGGER_H

#include "accountsyncmanager.h"

#include <QObject>
#include <QString>

class Q_DECL_EXPORT CloudBackupSyncTrigger : public QObject
{
    Q_OBJECT

private:
    QString m_currentSyncProfileId;
    AccountSyncManager *m_accountSyncManager;

public:
    CloudBackupSyncTrigger(QObject *parent = 0);

    /* localDir and remotePath are optional.  The default localDir is /home/nemo/.local/share/system/privileged/Backups/ */
    Q_INVOKABLE bool downloadFromCloud(int accountId, const QString &localDir = QString(), const QString &remotePath = QString());
    Q_INVOKABLE bool uploadToCloud(int accountId, const QString &localDir = QString(), const QString &remotePath = QString());

    /* no signals for the previous operation will be emitted after calling this, but you can immediately start a new operation. */
    Q_INVOKABLE void resetState();

Q_SIGNALS:
    void cloudSyncProgress(int status, const QString &errorString);

private:
    bool syncWithCloud(int accountId, const QString &localDir, const QString &remotePath, const QString &direction);

private Q_SLOTS:
    void handleProfileSyncStatusChanged(const QString &profileId, int status, const QString &errorString);
};

#endif // CLOUDBACKUPSYNCTRIGGER_H
