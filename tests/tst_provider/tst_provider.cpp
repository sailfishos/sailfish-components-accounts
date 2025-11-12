// SPDX-FileCopyrightText: 2013 - 2023 Jolla Ltd.
// SPDX-FileCopyrightText: 2017 Open Mobile Platform LLC.
// SPDX-FileCopyrightText: 2025 Jolla Mobile Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

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
