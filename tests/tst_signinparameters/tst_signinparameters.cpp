/*
 * Copyright (C) 2013 Jolla Ltd.
 * Contact: Chris Adams <chris.adams@jollamobile.com>
 *
 * License: Proprietary
 */

#include <QObject>
#include <QtTest>

#include "signinparameters.h"

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

class tst_SignInParameters : public QObject
{
    Q_OBJECT

private slots:
    //properties
    void properties();
};

void tst_SignInParameters::properties()
{
    QVariantMap params;
    params.insert("testKey", "testValue");
    QPointer<SignInParameters> si(new SignInParameters("test-service", "test-method", "test-mechanism", params, "user", "pass", this));
    QCOMPARE(si->serviceName(), QLatin1String("test-service"));
    QCOMPARE(si->method(), QLatin1String("test-method"));
    QCOMPARE(si->mechanism(), QLatin1String("test-mechanism"));
    QCOMPARE(si->parameters(), params);
    QCOMPARE(si->username(), QLatin1String("user"));
    QCOMPARE(si->password(), QLatin1String("pass"));

    QSignalSpy spy(si.data(), SIGNAL(parametersChanged()));
    int count = spy.count();
    params.insert("testKey2", "testValue2");
    si->setParameters(params);
    QTRY_COMPARE(spy.count(), count+1);
    QCOMPARE(si->parameters(), params);
}

#include "tst_signinparameters.moc"
QTEST_MAIN(tst_SignInParameters)
