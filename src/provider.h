/*
 * Copyright (C) 2013 Jolla Ltd.
 * Contact: Chris Adams <chris.adams@jollamobile.com>
 *
 * License: Proprietary
 */

#ifndef SAILFISH_ACCOUNTS__PROVIDER_H
#define SAILFISH_ACCOUNTS__PROVIDER_H

#include <QtCore/QObject>
#include <QStringList>
#include <QString>

//libaccounts-qt
#include <Accounts/Provider>

class Provider : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString displayName READ displayName CONSTANT)
    Q_PROPERTY(QString iconName READ iconName CONSTANT)
    Q_PROPERTY(QStringList serviceNames READ serviceNames CONSTANT)

public:
    Provider(const Accounts::Provider &provider = Accounts::Provider(), QObject *parent = 0);
    ~Provider();

    QString name() const;
    QString displayName() const;
    QString iconName() const;
    QStringList serviceNames() const;

private:
    Accounts::Provider m_provider;
    QStringList m_serviceNames;
};

#endif
