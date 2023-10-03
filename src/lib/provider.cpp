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

#include "provider.h"
#include "globalaccountmanager_p.h"
#include "globaltranslatorcache_p.h"
#include <QTranslator>

//libaccounts-qt
#include <Accounts/Manager>

/*!
    \qmltype Provider
    \instantiates Provider
    \inqmlmodule Sailfish.Accounts 1
    \brief Reports information about a given provider

    Every account is specified for a particular provider.
    Every provider can have one or more services associated
    with it.  Each account may be enabled with zero or more
    services from the provider.

    This type is purely informational, and reports information
    about the provider of the account.  The information is
    specified in the \c{.provider} file installed by the
    \l {https://docs.sailfishos.org/Reference/Core_Areas_and_APIs/Apps_and_MW/Accounts_and_SSO/Providers_and_Services} {account provider plugin}.
*/

Provider::Provider(const Accounts::Provider &provider, QObject *parent)
    : QObject(parent), m_provider(provider)
{
    // first time fetch of service names.
    Accounts::Manager *m = globalAccountManager();
    Accounts::ServiceList services = m->serviceList();
    foreach (const Accounts::Service &srv, services) {
        if (srv.provider() == m_provider.name()) {
            m_serviceNames.append(srv.name());
        }
    }
}

Provider::~Provider()
{
}

/*!
    \qmlproperty string Provider::name
    The name of the provider.
*/

QString Provider::name() const
{
    return m_provider.name();
}

/*!
    \qmlproperty string Provider::displayName
    The display name of the provider.  This display name
    can be displayed in lists or dialogues in the UI.
*/

QString Provider::displayName() const
{
    return SailfishAccounts::translatedDisplayName(m_provider);
}

/*!
    \qmlproperty string Provider::iconName
    The name of the icon associated with the provider.
*/

QString Provider::iconName() const
{
    return m_provider.iconName();
}

/*!
    \qmlproperty list Provider::serviceNames
    The names of services provided by this provider.
*/

QStringList Provider::serviceNames() const
{
    return m_serviceNames;
}

/*!
    \qmlproperty bool Provider::singleAccount
    Whether the provider supports creating one account at most.
*/

bool Provider::isSingleAccount() const
{
    return m_provider.isSingleAccount();
}

