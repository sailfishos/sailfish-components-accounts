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

#include <QQmlExtensionPlugin>
#include <QQmlEngine>
#include <QtQml>

#include <QTranslator>
#include <QGuiApplication>
#include <QLocale>

// impl detail
#include "globalaccountmanager_p.h"

// public types
#include "account.h"
#include "accountmanager.h"
#include "accountmodel.h"
#include "provider.h"
#include "providermodel.h"
#include "service.h"
#include "servicetype.h"
#include "servicemodel.h"
#include "signinparameters.h"
#include "accountsyncmanager.h"
#include "accountsyncoptions.h"
#include "accountauthenticator.h"
#include "accountvalue.h"

// using custom translator so it gets properly removed from qApp when engine is deleted
class AppTranslator: public QTranslator
{
    Q_OBJECT
public:
    AppTranslator(QObject *parent)
        : QTranslator(parent)
    {
        qApp->installTranslator(this);
    }

    virtual ~AppTranslator()
    {
        qApp->removeTranslator(this);
    }
};


class SailfishAccountsPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.sailfish.components.accounts")

public:

    void initializeEngine(QQmlEngine *engine, const char *uri)
    {
        Q_UNUSED(uri)
        Q_ASSERT(QLatin1String(uri) == QLatin1String("Sailfish.Accounts"));

        AppTranslator *engineeringEnglish = new AppTranslator(engine);
        AppTranslator *translator = new AppTranslator(engine);
        engineeringEnglish->load("sailfish_components_accounts_qt5_eng_en", "/usr/share/translations");
        translator->load(QLocale(), "sailfish_components_accounts_qt5", "-", "/usr/share/translations");
    }

    virtual void registerTypes(const char *uri)
    {
        Q_UNUSED(uri)
        Q_ASSERT(QLatin1String(uri) == QLatin1String("Sailfish.Accounts"));

        qmlRegisterType<Account>("Sailfish.Accounts", 1, 0, "Account");
        qmlRegisterType<AccountManager>("Sailfish.Accounts", 1, 0, "AccountManager");
        qmlRegisterType<AccountModel>("Sailfish.Accounts", 1, 0, "AccountModel");
        qmlRegisterType<Provider>("Sailfish.Accounts", 1, 0, "Provider");
        qmlRegisterType<ProviderModel>("Sailfish.Accounts", 1, 0, "ProviderModel");
        qmlRegisterType<Service>("Sailfish.Accounts", 1, 0, "Service");
        qmlRegisterType<ServiceType>("Sailfish.Accounts", 1, 0, "ServiceType");
        qmlRegisterType<ServiceModel>("Sailfish.Accounts", 1, 0, "ServiceModel");
        qmlRegisterType<SignInParameters>("Sailfish.Accounts", 1, 0, "SignInParameters");
        qmlRegisterType<AccountSyncManager>("Sailfish.Accounts", 1, 0, "AccountSyncManager");
        qmlRegisterType<AccountSyncOptions>("Sailfish.Accounts", 1, 0, "AccountSyncOptions");
        qmlRegisterType<AccountSyncSchedule>("Sailfish.Accounts", 1, 0, "AccountSyncSchedule");
        qmlRegisterType<AccountAuthenticator>("Sailfish.Accounts", 1, 0, "AccountAuthenticator");
        qmlRegisterUncreatableType<AccountAuthenticatorCredentials>("Sailfish.Accounts", 1, 0,
                                                                    "AccountAuthenticatorCredentials", "");
        qmlRegisterType<AccountValue>("Sailfish.Accounts", 1, 0, "AccountValue");
    }
};

#include "plugin.moc"

