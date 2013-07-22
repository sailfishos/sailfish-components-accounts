/*
 * Copyright (C) 2013 Jolla Ltd.
 * Contact: Chris Adams <chris.adams@jollamobile.com>
 *
 * License: Proprietary
 */

#include "service.h"

/*!
    \qmltype Service
    \instantiates Service
    \inqmlmodule Sailfish.Accounts 1
    \brief Reports information about a particular service

    Every service is provided by a particular provider.
    Each account may be enabled with zero or more services.

    This type provides information about a particular service
    provided by a provider.
*/

Service::Service(const Accounts::Service &service, QObject *parent)
    : QObject(parent), m_service(service)
{
}

Service::~Service()
{
}

/*!
    \qmlproperty string Service::name
    This property holds the name of the service
*/

QString Service::name() const
{
    return m_service.name();
}

/*!
    \qmlproperty string Service::displayName
    This property holds the display name of the service.
    This display name can be displayed in lists or
    dialogues in the UI of applications.
*/

QString Service::displayName() const
{
    return m_service.displayName();
}

/*!
    \qmlproperty string Service::serviceType
    This property holds the type of the service
*/

QString Service::serviceType() const
{
    return m_service.serviceType();
}

/*!
    \qmlproperty string Service::providerName
    This property holds the name of the provider which provides the service
*/

QString Service::providerName() const
{
    return m_service.provider();
}

/*!
    \qmlproperty string Service::iconName
    This property holds the name of the icon associated with the service
*/

QString Service::iconName() const
{
    return m_service.iconName();
}

/*!
    \qmlproperty QStringList Service::tags
    This property holds the tags which have been associated with the service.
*/

QStringList Service::tags() const
{
    return m_service.tags().toList();
}

