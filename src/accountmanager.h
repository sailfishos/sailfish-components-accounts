/*
 * Copyright (C) 2013 Jolla Ltd.
 * Contact: Chris Adams <chris.adams@jollamobile.com>
 *
 * License: Proprietary
 */

#ifndef SAILFISH_ACCOUNTS__ACCOUNTMANAGER_H
#define SAILFISH_ACCOUNTS__ACCOUNTMANAGER_H

#include <QtCore/QObject>

#include <QtGlobal>
#if QT_VERSION_5
#include <QtQml>
#include <QQmlParserStatus>
#define QDeclarativeParserStatus QQmlParserStatus
#else
#include <qdeclarative.h>
#include <QDeclarativeParserStatus>
#endif

#include <QtCore/QStringList>
#include <QtCore/QString>

class AccountManagerPrivate;
class ServiceType;
class Provider;
class Service;
class Account;

class AccountManager : public QObject, public QDeclarativeParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QDeclarativeParserStatus)

    Q_PROPERTY(QStringList serviceTypeNames READ serviceTypeNames NOTIFY serviceTypeNamesChanged)
    Q_PROPERTY(QStringList providerNames READ providerNames NOTIFY providerNamesChanged)
    Q_PROPERTY(QStringList serviceNames READ serviceNames NOTIFY serviceNamesChanged)
    Q_PROPERTY(QList<int> accountIdentifiers READ accountIdentifiers NOTIFY accountIdentifiersChanged)

public:
    AccountManager(QObject *parent = 0);
    ~AccountManager();

    // invokable api.
    Q_INVOKABLE bool createAccount(const QString &providerName);

    Q_INVOKABLE ServiceType *serviceType(const QString &serviceTypeName) const;
    Q_INVOKABLE Service *service(const QString &serviceName) const;
    Q_INVOKABLE Provider *provider(const QString &providerName) const;
    Q_INVOKABLE Account *account(int accountId) const;

    // property accessors and mutators.
    QStringList serviceTypeNames() const;
    QStringList providerNames() const;
    QStringList serviceNames() const;
    QList<int> accountIdentifiers() const;

    void classBegin();
    void componentComplete();

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
