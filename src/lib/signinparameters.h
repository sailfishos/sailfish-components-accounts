/****************************************************************************************
** Copyright (c) 2013 - 2023 Jolla Ltd.
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

#ifndef SAILFISH_ACCOUNTS__SIGNINPARAMETERS_H
#define SAILFISH_ACCOUNTS__SIGNINPARAMETERS_H

#include <QObject>
#include <QVariantMap>
#include <QString>
#include <QStringList>
#include <QVariant>

#include <SignOn/SessionData>

class Account;
class Q_DECL_EXPORT SignInParameters : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString serviceName READ serviceName CONSTANT)
    Q_PROPERTY(QString method READ method CONSTANT)
    Q_PROPERTY(QString mechanism READ mechanism CONSTANT)
    Q_PROPERTY(QVariantMap parameters READ parameters NOTIFY parametersChanged)
    Q_PROPERTY(QString username READ username CONSTANT)
    Q_PROPERTY(QString password READ password CONSTANT)

    Q_ENUMS(UiPolicy)
    Q_ENUMS(CredentialsPolicy)

public:
    enum UiPolicy {
        DefaultPolicy           = SignOn::DefaultPolicy,
        RequestPasswordPolicy   = SignOn::RequestPasswordPolicy,
        NoUserInteractionPolicy = SignOn::NoUserInteractionPolicy,
        ValidationPolicy        = SignOn::ValidationPolicy
    };

    enum CredentialsPolicy {
        UseCachedCredentialsPolicy = 0,
        RefreshCredentialsPolicy = 1
    };

public:
    ~SignInParameters();
    SignInParameters(const QString &serviceName = QString(),
                     const QString &method = QString(),
                     const QString &mechanism = QString(),
                     const QVariantMap &parameters = QVariantMap(),
                     const QString &username = QString(),
                     const QString &password = QString(),
                     QObject *parent = 0);

    // property accessors.
    QString serviceName() const;
    QString method() const;
    QString mechanism() const;
    QVariantMap parameters() const;
    QString username() const;
    QString password() const;

    // invokable api.
    Q_INVOKABLE void setParameters(const QVariantMap &parameters);
    Q_INVOKABLE void setParameter(const QString &parameterName, const QVariant &parameterValue);
    Q_INVOKABLE void setParameter(const QString &parameterName, const QString &parameterValue);
    Q_INVOKABLE void setParameter(const QString &parameterName, const QStringList &parameterValue);
    Q_INVOKABLE void setParameter(const QString &parameterName, const QUrl &parameterValue);
    Q_INVOKABLE void setParameter(const QString &parameterName, int parameterValue);
    Q_INVOKABLE void setParameter(const QString &parameterName, bool parameterValue);
    Q_INVOKABLE void setParameter(const QString &parameterName, const QVariantMap &parameterValue);
    Q_INVOKABLE void removeParameter(const QString &parameterName);

Q_SIGNALS:
    void parametersChanged();

private:
    QString m_serviceName;
    QString m_method;
    QString m_mechanism;
    QVariantMap m_parameters;
    QString m_username;
    QString m_password;
};

#endif
