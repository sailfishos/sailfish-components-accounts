/*
 * Copyright (C) 2014 Jolla Ltd.
 * Contact: Chris Adams <chris.adams@jollamobile.com>
 *
 * License: Proprietary
 */

#ifndef SAILFISH_ACCOUNTS__GLOBALTRANSLATORCACHE_P_H
#define SAILFISH_ACCOUNTS__GLOBALTRANSLATORCACHE_P_H

#include <QTranslator>
#include <QString>

#include <Accounts/Provider>
#include <Accounts/Service>
#include <Accounts/ServiceType>

namespace SailfishAccounts
{
    void initLibTranslator();

    QTranslator *cachedTranslator(const QString &trCatalog);
    QString translatedDisplayName(const Accounts::Provider &provider);
    QString translatedDisplayName(const Accounts::Service &service);
    QString translatedDisplayName(const Accounts::ServiceType &serviceType);
}

#endif
