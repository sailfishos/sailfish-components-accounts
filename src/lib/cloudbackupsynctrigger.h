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
#include <QNetworkAccessManager>
#include <QVariantList>

#include <SignOn/SessionData>
#include <SignOn/Error>

class Q_DECL_EXPORT CloudBackupSyncTrigger : public QObject
{
    Q_OBJECT

private:
    QString m_currentSyncProfileId;
    AccountSyncManager *m_accountSyncManager;
    QNetworkAccessManager *m_networkManager;

public:
    CloudBackupSyncTrigger(QObject *parent = 0);

    /* localDir and remotePath are optional.  The default localDir is /home/nemo/.local/share/system/privileged/Backups/ */
    Q_INVOKABLE bool downloadFromCloud(int accountId, const QString &localDir = QString(), const QString &remotePath = QString(), const QString &remoteFile = QString());
    Q_INVOKABLE bool uploadToCloud(int accountId, const QString &localDir = QString(), const QString &remotePath = QString());
    Q_INVOKABLE bool requestListing(int accountId, const QString &remotePath = QString());

    /* no signals for the previous operation will be emitted after calling this, but you can immediately start a new operation. */
    Q_INVOKABLE void resetState();

Q_SIGNALS:
    void cloudSyncProgress(int accountId, int status, const QString &errorString);
    void requestedListing(int accountId, const QVariantList &listing);
    void requestListingFailed(int accountId, const QString &message);

private:
    bool syncWithCloud(int accountId, const QString &localDir, const QString &remotePath, const QString &remoteFile, const QString &direction);
    void performListingRequest(int accountId, const QString &accessToken, const QString &cloudName, const QString &remotePath);

private Q_SLOTS:
    void handleProfileSyncStatusChanged(const QString &profileId, int status, const QString &errorString);
    void signOnResponse(const SignOn::SessionData &responseData);
    void signOnError(const SignOn::Error &error);
    void handleListingResponse();
};

#endif // CLOUDBACKUPSYNCTRIGGER_H
