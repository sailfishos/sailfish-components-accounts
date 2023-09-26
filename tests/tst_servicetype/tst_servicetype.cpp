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

#include "servicetype.h"

#include "accountmanager.h"

//libaccounts-qt
#include <Accounts/Manager>
#include <Accounts/Account>

class tst_ServiceType : public QObject
{
    Q_OBJECT

private slots:
    //properties
    void properties();
};

void tst_ServiceType::properties()
{
    QPointer<ServiceType> si;
    {
        QScopedPointer<AccountManager> m(new AccountManager);
        m->classBegin();
        m->componentComplete();
        ServiceType *srv = m->serviceType("test-service-type2");
        QVERIFY(srv);
        QCOMPARE(srv->name(), QString(QLatin1String("test-service-type2")));
        QCOMPARE(srv->displayName(), QString(QLatin1String("Test IM")));
        QCOMPARE(srv->iconName(), QString(QLatin1String("message")));
        QVERIFY(srv->tags().contains(QString(QLatin1String("testing"))));
        si = srv;
        QVERIFY(!si.isNull());
    }
    QTRY_VERIFY(si.isNull());
}

#include "tst_servicetype.moc"
QTEST_MAIN(tst_ServiceType)
