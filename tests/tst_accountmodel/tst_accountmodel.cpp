/*
 * Copyright (C) 2013 Jolla Ltd.
 * Contact: Chris Adams <chris.adams@jollamobile.com>
 *
 * License: Proprietary
 */

#include <QObject>
#include <QtTest>

#include "accountmodel.h"
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

class tst_AccountModel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void addAccount();
    void removeAccount();
    void updateAccount();
};

Q_DECLARE_METATYPE(QModelIndex)

void tst_AccountModel::initTestCase()
{
    qRegisterMetaType<QModelIndex>("QModelIndex");
}

void tst_AccountModel::addAccount()
{
    AccountModel model;
    int prevCount = model.rowCount();
    int matchedIndex;

    Accounts::Manager manager;
    QScopedPointer<Accounts::Account> newA(manager.createAccount("test-provider"));
    QSignalSpy newASyncedSpy(newA.data(), SIGNAL(synced()));
    QList<QVariant> spyArgs;
    newA->setDisplayName("test");
    newA->setEnabled(false);
    newA->sync();
    QTRY_VERIFY(newASyncedSpy.count() > 0);

    QScopedPointer<Account> account(new Account);
    account->classBegin();
    account->setIdentifier(newA->id());
    account->setDisplayName("some display name");
    account->sync();
    account->componentComplete();
    QTRY_COMPARE(account->status(), Account::Synced);
    QCOMPARE(model.rowCount(), prevCount+1);

    Accounts::Manager m;
    Accounts::Provider provider = m.provider(account->providerName());

    for (matchedIndex = 0; matchedIndex < model.rowCount(); matchedIndex++) {
        if (model.data(model.index(matchedIndex), AccountModel::AccountIdRole).toInt() == account->identifier()) {
            break;
        }
    }
    QCOMPARE(model.data(model.index(matchedIndex), AccountModel::AccountIdRole).toInt(), account->identifier());
    QCOMPARE(model.data(model.index(matchedIndex), AccountModel::AccountDisplayNameRole).toString(), account->displayName());
    QCOMPARE(model.data(model.index(matchedIndex), AccountModel::AccountIconRole).toString(), provider.iconName());
    QCOMPARE(model.data(model.index(matchedIndex), AccountModel::ProviderNameRole).toString(), account->providerName());
    QCOMPARE(model.data(model.index(matchedIndex), AccountModel::ProviderDisplayNameRole).toString(), provider.displayName());
    account->remove();
}

void tst_AccountModel::removeAccount()
{
    AccountModel model;
    int prevCount = model.rowCount();

    Accounts::Manager manager;
    QScopedPointer<Accounts::Account> newA(manager.createAccount("test-provider"));
    QSignalSpy newASyncedSpy(newA.data(), SIGNAL(synced()));
    QList<QVariant> spyArgs;
    newA->setDisplayName("test");
    newA->setEnabled(false);
    newA->sync();
    QTRY_VERIFY(newASyncedSpy.count() > 0);

    QScopedPointer<Account> account(new Account);
    account->classBegin();
    account->setIdentifier(newA->id());
    account->sync();
    account->componentComplete();
    QTRY_COMPARE(account->status(), Account::Synced);
    QCOMPARE(model.rowCount(), prevCount+1);

    account->remove();
    QTRY_COMPARE(model.rowCount(), prevCount);
}

void tst_AccountModel::updateAccount()
{
    AccountModel model;
    int prevCount = model.rowCount();

    Accounts::Manager manager;
    QScopedPointer<Accounts::Account> newA(manager.createAccount("test-provider"));
    QSignalSpy newASyncedSpy(newA.data(), SIGNAL(synced()));
    QList<QVariant> spyArgs;
    newA->setDisplayName("test");
    newA->setEnabled(false);
    newA->sync();
    QTRY_VERIFY(newASyncedSpy.count() > 0);

    QScopedPointer<Account> account(new Account);
    account->classBegin();
    account->setIdentifier(newA->id());
    account->sync();
    account->componentComplete();
    QTRY_COMPARE(account->status(), Account::Synced);
    QCOMPARE(model.rowCount(), prevCount+1);

    QSignalSpy spyDataChanged(&model, SIGNAL(dataChanged(QModelIndex,QModelIndex)));

    account->setDisplayName("blah blah");
    account->sync();
    QTRY_COMPARE(account->status(), Account::Synced);
    QTRY_COMPARE(spyDataChanged.count(), 1);

    QList<QVariant> arguments = spyDataChanged.takeFirst(); // take the signal

    QCOMPARE(model.data(model.index(arguments.at(0).value<QModelIndex>().row()), AccountModel::AccountDisplayNameRole).toString(), account->displayName());

    QVERIFY(!account->enabled());
    account->setEnabled(true);
    account->sync();
    QTRY_COMPARE(account->status(), Account::Synced);
    QTRY_COMPARE(spyDataChanged.count(), 1);
    QVERIFY(model.data(model.index(arguments.at(0).value<QModelIndex>().row()), AccountModel::AccountEnabledRole).toBool());

    account->remove();
}

#include "tst_accountmodel.moc"
QTEST_MAIN(tst_AccountModel)
