/*
 * Copyright (C) 2013 Jolla Ltd.
 * Contact: Chris Adams <chris.adams@jollamobile.com>
 *
 * License: Proprietary
 */

#include <QObject>
#include <QtTest>

#include "service.h"

#include "accountmanager.h"
#include "account.h"

//libaccounts-qt
#include <Accounts/Manager>
#include <Accounts/Account>

// Will try to wait for the condition while allowing event processing
#ifndef QTRY_VERIFY
#define QTRY_VERIFY(__expr) \
    do { \
        const int __step = 50; \
        const int __timeout = 5000; \
        if (!(__expr)) { \
            QTest::qWait(0); \
        } \
        for (int __i = 0; __i < __timeout && !(__expr); __i+=__step) { \
            QTest::qWait(__step); \
        } \
        QVERIFY(__expr); \
    } while (0)
#endif

// Will try to wait for the condition while allowing event processing
#ifndef QTRY_COMPARE
#define QTRY_COMPARE(__expr, __expected) \
    do { \
        const int __step = 50; \
        const int __timeout = 5000; \
        if ((__expr) != (__expected)) { \
            QTest::qWait(0); \
        } \
        for (int __i = 0; __i < __timeout && ((__expr) != (__expected)); __i+=__step) { \
            QTest::qWait(__step); \
        } \
        QCOMPARE(__expr, __expected); \
    } while (0)
#endif

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
