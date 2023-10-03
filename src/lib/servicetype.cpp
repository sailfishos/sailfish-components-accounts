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

#include "servicetype.h"
#include "globaltranslatorcache_p.h"
#include <QTranslator>

/*!
    \qmltype ServiceType
    \instantiates ServiceType
    \inqmlmodule Sailfish.Accounts 1
    \brief Reports information about a particular service type

    This type provides information about a particular service type
    of a service provided by a provider.  For example, "IM" type
    services provide instant messaging capabilities.
*/

ServiceType::ServiceType(const Accounts::ServiceType &serviceType, QObject *parent)
    : QObject(parent), m_serviceType(serviceType)
{
}

ServiceType::~ServiceType()
{
}

/*!
    \qmlproperty string ServiceType::name
    This property holds the name of the service type
*/

QString ServiceType::name() const
{
    return m_serviceType.name();
}

/*!
    \qmlproperty string ServiceType::displayName
    This property holds the display name of the service type.
    This display name can be displayed in lists or
    dialogues in the UI of applications.
*/

QString ServiceType::displayName() const
{
    return SailfishAccounts::translatedDisplayName(m_serviceType);
}

/*!
    \qmlproperty string ServiceType::iconName
    This property holds the name of the icon associated with the service type, if it exists
*/

QString ServiceType::iconName() const
{
    return m_serviceType.iconName();
}

/*!
    \qmlproperty list ServiceType::tags
    This property holds the tags which have been associated with the service type.
*/

QStringList ServiceType::tags() const
{
    return m_serviceType.tags().toList();
}

