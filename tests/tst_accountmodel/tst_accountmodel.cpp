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

#include "accountmodel.h"
#include "account.h"

//libaccounts-qt
#include <Accounts/Manager>
#include <Accounts/Account>

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
    QCOMPARE(model.data(model.index(matchedIndex), AccountModel::ProviderValidRole).toBool(), true);
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
