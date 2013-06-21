/*
 * Copyright (C) 2013 Jolla Ltd.
 * Contact: Chris Adams <chris.adams@jollamobile.com>
 *
 * License: Proprietary
 */

#ifndef SAILFISH_ACCOUNTS__SERVICETYPE_H
#define SAILFISH_ACCOUNTS__SERVICETYPE_H

#include <QtCore/QObject>
#include <QStringList>
#include <QString>

//libaccounts-qt
#include <Accounts/ServiceType>

class ServiceType : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString displayName READ displayName CONSTANT)
    Q_PROPERTY(QString iconName READ iconName CONSTANT)
    Q_PROPERTY(QStringList tags READ tags CONSTANT)

public:
    ServiceType(const Accounts::ServiceType &serviceType = Accounts::ServiceType(), QObject *parent = 0);
    ~ServiceType();

    QString name() const;
    QString displayName() const;
    QString iconName() const;
    QStringList tags() const;

private:
    Accounts::ServiceType m_serviceType;
};

#endif
