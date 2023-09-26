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

#include "accountmanager.h"

#include "account.h"
#include "servicetype.h"
#include "service.h"
#include "provider.h"

//libaccounts-qt
#include <Accounts/Manager>
#include <Accounts/Account>

class tst_AccountManager : public QObject
{
    Q_OBJECT

private slots:
    //properties
    void serviceTypeNames();
    void providerNames();
    void serviceNames();
    void accountIdentifiers();

    //invokables
    void accounts();
    void services();
    void providers();
};

void tst_AccountManager::serviceTypeNames()
{
    // tested more thoroughly by serviceTypeFilter()
    QScopedPointer<AccountManager> m(new AccountManager);
    m->classBegin();
    m->componentComplete();
    QVERIFY(m->serviceTypeNames().contains("test-service-type2"));
}

void tst_AccountManager::providerNames()
{
    // tested more thoroughly by serviceTypeFilter()
    QScopedPointer<AccountManager> m(new AccountManager);
    m->classBegin();
    m->componentComplete();
    QVERIFY(m->providerNames().contains("test-provider"));
}

void tst_AccountManager::serviceNames()
{
    // tested more thoroughly by serviceTypeFilter()
    QScopedPointer<AccountManager> m(new AccountManager);
    m->classBegin();
    m->componentComplete();
    QVERIFY(m->serviceNames().contains("test-service2"));
}

void tst_AccountManager::accountIdentifiers()
{
    QScopedPointer<AccountManager> m(new AccountManager);
    m->classBegin();
    m->componentComplete();
    QSignalSpy spy(m.data(), SIGNAL(accountIdentifiersChanged()));

    // add an account out-of-band
    Accounts::Manager am;
    QScopedPointer<Accounts::Account> a(am.createAccount("test-provider"));
    a->setDisplayName("test-account");
    a->selectService(am.service("test-service2"));
    a->setEnabled(true);
    a->selectService(Accounts::Service());
    a->sync();

    // ensure that we can access it via our manager
    QTRY_COMPARE(spy.count(), 1);
    QVERIFY(a->id() > 0); // sync succeeded.
    int newAccountId = a->id();
    QVERIFY(m->accountIdentifiers().contains(newAccountId));

    // and via the per-provider accessor
    QVERIFY(m->providerAccountIdentifiers("test-provider").contains(newAccountId));

    // remove the account.
    a->remove();
    a->sync();

    // ensure it's gone.
    QTRY_COMPARE(spy.count(), 2);
    QVERIFY(!m->accountIdentifiers().contains(newAccountId));
    QVERIFY(!m->providerAccountIdentifiers("test-provider").contains(newAccountId));
}

// ---------------- Q_INVOKABLE api tests:

void tst_AccountManager::accounts()
{
    QPointer<Account> acc;
    QPointer<Account> returnedAcc;

    // test ownership semantics of createAccount()
    {
        QScopedPointer<AccountManager> m(new AccountManager);
        m->classBegin();
        m->componentComplete();
        QSignalSpy spy(m.data(), SIGNAL(accountCreated(int,QString)));
        QList<QVariant> spyArgs;
        QVERIFY(m->createAccount("test-provider"));
        QTRY_VERIFY(spy.count() > 0);
        spyArgs = spy.takeFirst();
        Account *newAcc = m->account(spyArgs.at(0).toInt());
        QVERIFY(newAcc);
        QCOMPARE(spyArgs.at(1).toString(), QLatin1String("test-provider"));
        acc = newAcc; // guard.
        QVERIFY(!acc.isNull());
    }
    QTRY_VERIFY(acc.isNull()); // manager owns the created account .

    // test create / sync / remove
    {
        QScopedPointer<AccountManager> m(new AccountManager);
        m->classBegin();
        m->componentComplete();
        QSignalSpy spy(m.data(), SIGNAL(accountCreated(int,QString)));
        QList<QVariant> spyArgs;
        QVERIFY(m->createAccount("test-provider"));
        QTRY_VERIFY(spy.count() > 0);
        spyArgs = spy.takeFirst();
        Account *newAcc = m->account(spyArgs.at(0).toInt());
        QVERIFY(newAcc);
        acc = newAcc; // guard.
        QVERIFY(!acc.isNull());
        QCOMPARE(spy.count(), 0); // not added until synced.

        // sync account to write it to db
        acc->setDisplayName("test-account");
        acc->enableWithService("test-service2");
        acc->setEnabled(true);
        QCOMPARE(acc->status(), Account::Initializing); // haven't returned to the event loop yet, so asyncQueryInfo() is pending.
        acc->sync();
        QTRY_COMPARE(acc->status(), Account::Synced);
        int newAccountId = acc->identifier();
        QVERIFY(newAccountId != 0);
        QVERIFY(m->accountIdentifiers().contains(newAccountId));

        // ensure the sync worked correctly
        Accounts::Manager aqMan;
        Accounts::Account *aqAcc = aqMan.account(acc->identifier());
        QVERIFY(aqAcc->enabled()); // globally enabled
        Accounts::Service aqSrv = aqMan.service("test-service2");
        QVERIFY(aqSrv.isValid());  // service exists
        aqAcc->selectService(aqSrv);
        QVERIFY(aqAcc->enabled()); // enabled with service

        // now ensure that account retrieval works.
        Account *tempAcc = m->account(newAccountId); // string id
        QVERIFY(tempAcc);
        QTRY_COMPARE(tempAcc->displayName(), acc->displayName());
        tempAcc = m->account(acc->identifier()); // int id
        QVERIFY(tempAcc);
        QTRY_COMPARE(tempAcc->displayName(), acc->displayName());
        returnedAcc = tempAcc;

        // remove account.
        acc->remove();
    }
    QTRY_VERIFY(returnedAcc.isNull()); // manager owns the returned account .   
}

void tst_AccountManager::services()
{
    QPointer<Service> si;
    QPointer<ServiceType> sti;

    // test service type
    {
        QScopedPointer<AccountManager> m(new AccountManager);
        m->classBegin();
        m->componentComplete();
        ServiceType *retSti = m->serviceType("test-service-type2");
        QVERIFY(retSti);
        sti = retSti; // guard.
        QCOMPARE(sti->name(), QString("test-service-type2"));
        QVERIFY(sti->tags().contains("testing"));
    }
    QTRY_VERIFY(sti.isNull()); // manager owns returned service type .

    // test service
    {
        QScopedPointer<AccountManager> m(new AccountManager);
        m->classBegin();
        m->componentComplete();
        Service *retSi = m->service("test-service2");
        QVERIFY(retSi);
        si = retSi; // guard.
        QCOMPARE(si->name(), QString("test-service2"));
        QCOMPARE(si->providerName(), QString("test-provider"));
        QCOMPARE(si->serviceType(), QString("test-service-type2"));
    }
    QTRY_VERIFY(si.isNull()); // manager owns returned service .

    // nonexistent
    {
        QScopedPointer<AccountManager> m(new AccountManager);
        m->classBegin();
        m->componentComplete();
        Service *retSi = m->service("nonexistent-service-test");
        QVERIFY(retSi);
        si = retSi; // guard.
        QCOMPARE(si->name(), QString());
        QCOMPARE(si->providerName(), QString());
        QCOMPARE(si->serviceType(), QString());
        ServiceType *retSti = m->serviceType("nonexistent-service-type-test");
        QVERIFY(retSti);
        sti = retSti; // guard.
        QCOMPARE(sti->name(), QString());
    }
    QTRY_VERIFY(si.isNull()); // manager owns returned service .
}

void tst_AccountManager::providers()
{
    QPointer<Provider> pi;
    {
        QScopedPointer<AccountManager> m(new AccountManager);
        m->classBegin();
        m->componentComplete();
        Provider *retPi = m->provider("test-provider");
        QVERIFY(retPi);
        pi = retPi; // guard.
        QCOMPARE(pi->name(), QString("test-provider"));
        QVERIFY(pi->serviceNames().contains(QString("test-service2"))); // it may contain other (old test) services
    }
    QTRY_VERIFY(pi.isNull()); // manager owns returned provider .

    // nonexistent
    {
        QScopedPointer<AccountManager> m(new AccountManager);
        m->classBegin();
        m->componentComplete();
        Provider *retPi = m->provider("nonexistent-provider-test");
        QVERIFY(retPi);
        pi = retPi; // guard.
        QCOMPARE(pi->name(), QString());
        QCOMPARE(pi->serviceNames(), QStringList());
    }
    QTRY_VERIFY(pi.isNull()); // manager owns returned provider .
}

#include "tst_accountmanager.moc"
QTEST_MAIN(tst_AccountManager)
