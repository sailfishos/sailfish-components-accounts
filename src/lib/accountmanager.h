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

#ifndef SAILFISH_ACCOUNTS__ACCOUNTMANAGER_H
#define SAILFISH_ACCOUNTS__ACCOUNTMANAGER_H

#include <QtCore/QObject>

#include <QtGlobal>
#include <QtQml>
#include <QQmlParserStatus>

#include <QtCore/QStringList>
#include <QtCore/QString>

class AccountManagerPrivate;
class ServiceType;
class Provider;
class Service;
class Account;

namespace Accounts {
class Provider;
class Service;
class ServiceType;
}

class Q_DECL_EXPORT AccountManager : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(QStringList serviceTypeNames READ serviceTypeNames NOTIFY serviceTypeNamesChanged)
    Q_PROPERTY(QStringList providerNames READ providerNames NOTIFY providerNamesChanged)
    Q_PROPERTY(QStringList serviceNames READ serviceNames NOTIFY serviceNamesChanged)
    Q_PROPERTY(QList<int> accountIdentifiers READ accountIdentifiers NOTIFY accountIdentifiersChanged)

public:
    AccountManager(QObject *parent = 0);
    ~AccountManager();

    // invokable api.
    Q_INVOKABLE QList<int> providerAccountIdentifiers(const QString &providerName);
    Q_INVOKABLE bool createAccount(const QString &providerName);

    Q_INVOKABLE ServiceType *serviceType(const QString &serviceTypeName) const;
    Q_INVOKABLE Service *service(const QString &serviceName) const;
    Q_INVOKABLE Provider *provider(const QString &providerName) const;
    Q_INVOKABLE Provider *providerForAccount(int accountId) const;
    Q_INVOKABLE Account *account(int accountId) const;
    Q_INVOKABLE bool credentialsNeedUpdate(int accountId);
    Q_INVOKABLE QList<int> enabledAccounts(const QString &providerName, const QString &serviceName);

    // property accessors and mutators.
    QStringList serviceTypeNames() const;
    QStringList providerNames() const;
    QStringList serviceNames() const;
    QList<int> accountIdentifiers() const;

    void classBegin();
    void componentComplete();

    // Provided for external use
    // Internally globaltranslatorcache_p.h should be used instead
    static QString translatedDisplayName(const Accounts::Provider &instance);
    static QString translatedDisplayName(const Accounts::Service &instance);
    static QString translatedDisplayName(const Accounts::ServiceType &instance);

Q_SIGNALS:
    void serviceTypeNamesChanged();
    void providerNamesChanged();
    void serviceNamesChanged();
    void accountIdentifiersChanged();

    void accountCreated(int accountId, const QString &providerName);
    void accountCreationFailed(const QString &message, const QString &providerName);

private:
    AccountManagerPrivate *d;
    friend class AccountManagerPrivate;
};

#endif
