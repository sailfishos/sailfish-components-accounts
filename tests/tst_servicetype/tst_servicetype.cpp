// SPDX-FileCopyrightText: 2013 - 2023 Jolla Ltd.
// SPDX-FileCopyrightText: 2025 Jolla Mobile Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

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
