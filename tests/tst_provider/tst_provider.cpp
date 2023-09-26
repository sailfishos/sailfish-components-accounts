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

#include <QObject>
#include <QtTest>

#include "provider.h"

#include "accountmanager.h"
#include "account.h"

//libaccounts-qt
#include <Accounts/Manager>
#include <Accounts/Account>

class tst_Provider : public QObject
{
    Q_OBJECT

private slots:
    //properties
    void properties();
};

void tst_Provider::properties()
{
    QPointer<Provider> pi;
    {
        QScopedPointer<AccountManager> m(new AccountManager);
        m->classBegin();
        m->componentComplete();
        Provider *prv = m->provider("test-provider");
        QVERIFY(prv);
        QCOMPARE(prv->name(), QString(QLatin1String("test-provider")));
        QCOMPARE(prv->displayName(), QString(QLatin1String("Provider(test)")));
        QCOMPARE(prv->iconName(), QString(QLatin1String("test-icon")));
        QVERIFY(prv->serviceNames().contains(QString(QLatin1String("test-service2"))));
        QCOMPARE(prv->isSingleAccount(), false);
        pi = prv;
        QVERIFY(!pi.isNull());
    }
    QTRY_VERIFY(pi.isNull());
}

#include "tst_provider.moc"
QTEST_MAIN(tst_Provider)
