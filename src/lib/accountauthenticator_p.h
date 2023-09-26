/****************************************************************************************
** Copyright (c) 2019 Open Mobile Platform LLC.
** Copyright (c) 2023 Jolla Ltd.
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

#ifndef SAILFISH_ACCOUNTS__ACCOUNTAUTHENTICATOR_P_H
#define SAILFISH_ACCOUNTS__ACCOUNTAUTHENTICATOR_P_H

#include "accountauthenticator.h"

#include <Accounts/Account>
#include <Accounts/Manager>
#include <Accounts/Service>
#include <Accounts/AccountService>
#include <SignOn/Identity>
#include <SignOn/Error>
#include <SignOn/SessionData>
#include <SignOn/AuthSession>

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QObject>
#include <QVector>

class AccountAuthenticatorPrivate : public QObject
{
    Q_OBJECT

public:
    AccountAuthenticatorPrivate(AccountAuthenticator *parent);
    ~AccountAuthenticatorPrivate();

    void signIn(int accountId, const QString &serviceName);
    void sendAuthenticatedRequest(const QUrl &url, const AccountAuthenticatorCredentials &credentials, bool ignoreSslErrors);
    void sendOcsUserRequest(int accountId,
                            const QString &serviceName,
                            const AccountAuthenticatorCredentials &credentials,
                            bool ignoreSslErrors);
    bool setCredentialsNeedUpdate(int accountId, const QString &serviceName);

private:
    void signOnResponse(const SignOn::SessionData &response);
    void signOnError(const SignOn::Error &error);

    void authenticatedRequestFinished();
    void authenticatedRequestSslErrors(const QList<QSslError> &errors);

    void ocsUserRequestFinished();
    void ocsUserRequestSslErrors(const QList<QSslError> &errors);

    QNetworkRequest networkRequest(const QUrl &serverAddress, const AccountAuthenticatorCredentials &credentials, const QString &path);
    QString parseUserIdFromOcsUserResponse(const QByteArray &ocsUserResponse);

    class AuthData
    {
    public:
        int accountId;
        QString serviceName;
        QString mechanism;
        QVariantMap signonSessionData;
        Accounts::Account *account;
        SignOn::Identity *identity;
        SignOn::AuthSession *authSession;
        AccountAuthenticatorCredentials credentials;
    };

    AccountAuthenticator *q;
    QNetworkAccessManager *m_networkAccessManager = nullptr;
    QVector<AuthData> m_authData;
};

#endif // SAILFISH_ACCOUNTS__ACCOUNTAUTHENTICATOR_P_H
