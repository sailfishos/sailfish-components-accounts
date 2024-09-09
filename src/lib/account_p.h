/****************************************************************************************
** Copyright (c) 2013 - 2023 Jolla Ltd.
** Copyright (c) 2020 Open Mobile Platform LLC.
**
** All rights reserved.
**
** This file is part of Sailfish Accounts components package.
**
** You may use this file under the terms of BSD license as follows:
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
** 1. Redistributions of source code must retain the above copyright notice, this
**    list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above copyright notice,
**    this list of conditions and the following disclaimer in the documentation
**    and/or other materials provided with the distribution.
**
** 3. Neither the name of the copyright holder nor the names of its
**    contributors may be used to endorse or promote products derived from
**    this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
** SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
** CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
** OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
****************************************************************************************/
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
#include <SignOn/Identity>
#include <SignOn/IdentityInfo>
#include <SignOn/AuthSession>

static const auto AccountReadOnlyKey = QStringLiteral("readonly");
static const auto AccountProvisionedKey = QStringLiteral("provisioned"); // created by MDM.
static const auto AccountLimitedKey = QStringLiteral("limited");

struct SignInCredentials {
    bool creatingSignInCredentials;
    bool updatingSignInCredentials;
    bool signingInWithCredentials;
    bool storingEncryptedTokens;
    bool forcingCredentialsRefresh;
    bool haveForcedCredentialsExpiry;
    bool canceling;

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
    enum ConfigState {
        Default,
        Change,
        Remove
    };
    typedef QMap<QString, ConfigState> ConfigStateMap;

    AccountPrivate(Account *parent, Accounts::Account *acc, bool queryInfo = true);
    ~AccountPrivate();

    void setAccount(Accounts::Account *acc, bool queryInfo);
    void setStatus(Account::Status newStatus);
    bool prepareSync();

    void cancelCredentialsOperation(bool removeIdentity);

    void setUserName(const QString &user);

    void updateStoreRepositories(bool enable);

    void updateServiceKeys(const Accounts::Service &service, ConfigStateMap &states, const QVariantMap &values, QVariantMap &baseline);
    void updateServiceKey(const ConfigStateMap &states, const QVariantMap &values, QVariantMap &baseline, const QString &key);

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
    QString defaultCredentialsUserName;
    QVariantMap configurationValues;
    QMap<QString, QVariantMap> serviceConfigurationValues;

    QVariantMap baselineValues;
    QMap<QString, QVariantMap> serviceBaselineValues;
    ConfigStateMap configurationStates;
    QMap<QString, ConfigStateMap> serviceConfigurationStates;

    QStringList supportedServiceNames;
    QMap<QString, bool> serviceEnabledChanges;
    bool enabledPendingInit;
    bool displayNamePendingInit;
    bool configurationValuesPendingInit;
    bool constructedWithAccountPtr;

    bool provisioned;
    bool readonly;
    bool limited;

    Account::Status status;
    Account::ErrorType error;
    SignOn::AuthSession::AuthSessionState sessionState;
    QString errorMessage;

public:
    QVariantMap plainTextResponseData(const QString &method,
                                      const QVariantMap &encryptedResponseData,
                                      const QString &symmetricKey,
                                      const QString &applicationName,
                                      bool *succeeded) const;
    bool serviceReadyForEdits(const QString &serviceName) const;
    void setModified(bool &pendingInit);

public Q_SLOTS:
    void enabledHandler(const QString &, bool);
    void displayNameChangedHandler();
    void invalidate();
    void handleRemoved();
    void handleSynced();
    void asyncQueryInfo();

    // for credentials creation / sign-in
    void handleCredentialsStored(quint32);
    void handleCredentialsInfo(const SignOn::IdentityInfo &info);
    void handleCredentialsFailed(const SignOn::Error &err);
    void handleResponse(const SignOn::SessionData &data);
    void handleExpiryTimeout();
    void handleSignOnError(const SignOn::Error &err);
    void handleStateChanged(SignOn::AuthSession::AuthSessionState state, const QString &message);
    void handleAccountError();
};

#endif
