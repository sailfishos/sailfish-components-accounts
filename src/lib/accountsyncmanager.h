/*
 * Copyright (C) 2013 Jolla Ltd.
 * Contact: Bea Lam <bea.lam@jollamobile.com>
 *
 * License: Proprietary
 */

#ifndef ACCOUNTSYNCPROFILEMANAGER_P_H
#define ACCOUNTSYNCPROFILEMANAGER_P_H

#include <QObject>
#include <QVariantMap>

#include <Accounts/AccountService>

namespace Accounts {
    class Account;
}

class AccountSyncProfileManagerPrivate;

class Q_DECL_EXPORT AccountSyncManager : public QObject
{
    Q_OBJECT
    Q_ENUMS(SyncStatus)
public:
    enum SyncStatus {
        UnknownSyncStatus,
        SyncStarted,
        SyncFinished,
        SyncAborted,
        SyncError
    };

    AccountSyncManager(QObject *parent = 0);
    ~AccountSyncManager();

    Q_INVOKABLE void createProfile(const QString &templateProfileName, int accountId, const QString &serviceName);
    Q_INVOKABLE void updateProfile(const QString &profileId, const QVariantMap &properties);
    Q_INVOKABLE void syncProfile(const QString &profileId);

    Q_INVOKABLE QStringList profileIds(int accountId, const QString &serviceName) const;

    QString createProfile(const QString &templateProfileName,
                          Accounts::Account *account,
                          const Accounts::Service &srv,
                          bool enableProfile,
                          const QVariantMap &properties = QVariantMap());
    bool updateSyncProfile(const QString &profileId, const QVariantMap &properties);

    bool hasProfile(Accounts::Account *account, const Accounts::Service &srv) const;
    QStringList defaultTemplateProfiles(Accounts::Account *account, const Accounts::Service &srv) const;

signals:
    void profileCreated(const QString &profileId);
    void profileCreationError(int accountId, const QString &serviceName, const QString &errorString);
    void profileUpdated(const QString &profileId);
    void profileUpdateError(const QString &profileId, const QString &errorString);
    void profileSyncStatusChanged(const QString &profileId, int status, const QString &errorString);

private:
    AccountSyncProfileManagerPrivate *d;
};

#endif
