/*
 * Copyright (C) 2013 Jolla Ltd.
 * Contact: Chris Adams <chris.adams@jollamobile.com>
 *
 * License: Proprietary
 */

#include <QObject>
#include <QtTest>

#include "provider.h"

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
