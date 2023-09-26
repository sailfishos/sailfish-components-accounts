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

#include "service.h"
#include "globaltranslatorcache_p.h"
#include <QTranslator>

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
    return SailfishAccounts::translatedDisplayName(m_service);
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
    \qmlproperty list Service::tags
    This property holds the tags which have been associated with the service.
*/

QStringList Service::tags() const
{
    return m_service.tags().toList();
}

