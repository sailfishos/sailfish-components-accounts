/*
 * Copyright (C) 2013 Jolla Ltd.
 * Contact: Chris Adams <chris.adams@jollamobile.com>
 *
 * License: Proprietary
 */

#ifndef SAILFISH_ACCOUNTS__ACCOUNT_P_H
#define SAILFISH_ACCOUNTS__ACCOUNT_P_H

#include "account.h"

#include <QtCore/QObject>
#include <QStringList>
#include <QString>

//libaccounts-qt
#include <Accounts/Account>
#include <Accounts/Manager>

//libsignon-qt
#include <SignOn/IdentityInfo>

struct SignInCredentials {
    bool creatingSignInCredentials;
    bool updatingSignInCredentials;
    bool signingInWithCredentials;
    bool storingEncryptedTokens;

    SignOn::Identity *identity;
    SignOn::AuthSession *session;

    SignOn::IdentityInfo identityInfo;

    QString applicationName;
    QString symmetricKey;
    QString serviceName;
    QString method;
    QString mechanism;
    QVariantMap sessionData;
    QString credentialsName;
    QString username;
    QString password;

    QVariantMap responseData;

    void cleanup(bool removeIdentity = false);
};

class AccountPrivate : public QObject
{
    Q_OBJECT

public:
    AccountPrivate(Account *parent, Accounts::Account *acc, bool queryInfo = true);
    ~AccountPrivate();

    void setAccount(Accounts::Account *acc, bool queryInfo);
    void setStatus(Account::Status newStatus);

    Account *q;
    Accounts::Manager *manager;
    Accounts::Account *account;

    SignInCredentials signInCredentials;

    bool pendingSync;
    bool pendingInitModifications;

    int identifier;
    QString providerName;
    bool enabled;
    QString displayName;
    QVariantMap configurationValues;
    QMap<QString, QVariantMap> serviceConfigurationValues;
    QStringList supportedServiceNames;
    QMap<QString, bool> serviceEnabledChanges;
    bool identifierPendingInit;
    bool enabledPendingInit;
    bool displayNamePendingInit;
    bool configurationValuesPendingInit;
    bool constructedWithAccountPtr;

    Account::Status status;
    Account::ErrorType error;
    QString errorMessage;

public:
    QVariantMap plainTextResponseData(const QString &method,
                                      const QVariantMap &encryptedResponseData,
                                      const QString &symmetricKey,
                                      const QString &applicationName,
                                      bool *succeeded) const;

public Q_SLOTS:
    void enabledHandler(const QString &, bool);
    void displayNameChangedHandler();
    void invalidate();
    void handleSynced();
    void asyncQueryInfo();

    // for credentials creation / sign-in
    void handleCredentialsStored(quint32);
    void handleCredentialsInfo(const SignOn::IdentityInfo &info);
    void handleCredentialsFailed(const SignOn::Error &err);
    void handleResponse(const SignOn::SessionData &data);
    void handleSignOnError(const SignOn::Error &err);
    void handleAccountError();
};

#endif
