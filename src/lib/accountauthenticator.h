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

#ifndef SAILFISH_ACCOUNTS__ACCOUNTAUTHENTICATOR_H
#define SAILFISH_ACCOUNTS__ACCOUNTAUTHENTICATOR_H

#include <QObject>
#include <QVariantMap>

class AccountAuthenticatorPrivate;

class Q_DECL_EXPORT AccountAuthenticatorCredentials
{
    Q_GADGET
    Q_PROPERTY(QString username MEMBER username)
    Q_PROPERTY(QString password MEMBER password)
    Q_PROPERTY(QString accessToken MEMBER accessToken)
    Q_PROPERTY(QVariantMap serviceSettings MEMBER serviceSettings)
public:
    QString username;
    QString password;
    QString accessToken;
    QVariantMap serviceSettings;
};

Q_DECLARE_METATYPE(AccountAuthenticatorCredentials)

class Q_DECL_EXPORT AccountAuthenticator : public QObject
{
    Q_OBJECT

public:
    AccountAuthenticator(QObject *parent = nullptr);

    Q_INVOKABLE void signIn(int accountId, const QString &serviceName);

    Q_INVOKABLE void sendAuthenticatedRequest(const QUrl &url,
                                              const AccountAuthenticatorCredentials &credentials,
                                              bool ignoreSslErrors);
    Q_INVOKABLE void sendOcsUserRequest(int accountId,
                                        const QString &serviceName,
                                        const AccountAuthenticatorCredentials &credentials,
                                        bool ignoreSslErrors);
    Q_INVOKABLE bool setCredentialsNeedUpdate(int accountId, const QString &serviceName);

Q_SIGNALS:
    void signInCompleted(int accountId,
                         const QString &serviceName,
                         const AccountAuthenticatorCredentials &credentials);
    void signInError(int accountId, const QString &serviceName, const QString &errorString);

    void authenticatedRequestFinished(bool success, const QString &errorString);
    void ocsUserRequestFinished(bool success, const QString &errorString);

private:
    friend class AccountAuthenticatorPrivate;
    AccountAuthenticatorPrivate *d;
};

#endif // SAILFISH_ACCOUNTS__ACCOUNTAUTHENTICATOR_H
