// SPDX-FileCopyrightText: 2013 - 2023 Jolla Ltd.
// SPDX-FileCopyrightText: 2025 Jolla Mobile Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <QObject>
#include <QtTest>

#include "service.h"

#include "accountmanager.h"
#include "account.h"

//libaccounts-qt
#include <Accounts/Manager>
#include <Accounts/Account>

class tst_Service : public QObject
{
    Q_OBJECT

private slots:
    //properties
    void properties();
};

void tst_Service::properties()
{
    QPointer<Service> si;
    {
        QScopedPointer<AccountManager> m(new AccountManager);
        m->classBegin();
        m->componentComplete();
        Service *srv = m->service("test-service2");
        QVERIFY(srv);
        QCOMPARE(srv->name(), QString(QLatin1String("test-service2")));
        QCOMPARE(srv->serviceType(), QString(QLatin1String("test-service-type2")));
        QCOMPARE(srv->displayName(), QString(QLatin1String("Test Service")));
        QCOMPARE(srv->providerName(), QString(QLatin1String("test-provider")));
        QCOMPARE(srv->iconName(), QString(QLatin1String("test")));
        QVERIFY(srv->tags().contains(QString(QLatin1String("testing"))));
        si = srv;
        QVERIFY(!si.isNull());
    }
    QTRY_VERIFY(si.isNull());
}

#include "tst_service.moc"
QTEST_MAIN(tst_Service)
