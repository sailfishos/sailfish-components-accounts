/*
 * Copyright (C) 2013 Jolla Ltd.
 * Contact: Chris Adams <chris.adams@jollamobile.com>
 *
 * License: Proprietary
 */

#include <QObject>
#include <QColor>
#include <QtTest>

#include "account.h"
#include "signinparameters.h"
#include "globalaccountmanager_p.h"

//libaccounts-qt
#include <Accounts/Manager>
#include <Accounts/Account>

//libsignon-qt
#include <SignOn/SessionData>

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

// copied from account.cpp -> if that changes you must update these
#define CREDENTIALS_GROUP QLatin1String("segregated_credentials")
#define BUILD_CREDENTIALS_CONFIGURATION_KEY(appName, credName) QString(QLatin1String("%1/%2/%3")).arg(appName).arg(CREDENTIALS_GROUP).arg(credName)

class tst_Account : public QObject
{
    Q_OBJECT

public slots:
    void init();
    void cleanup();
    void trackAccountAdded(Accounts::AccountId id);
    void untrackAccountRemoved(Accounts::AccountId id);

private slots:
    //properties
    void enabled();
    void identifier();
    void providerName();
    void displayName();
    void supportedServiceNames();
    void status();
    void error();
    void errorMessage();
    //invokables
    void configurationValues();
    void serviceConfigurationValues();
    void enableDisableWithService();
    //signin-related
    void credentialsFunctions();
    void loadSavedAccount();

    // expected usage
    void expectedUsage();

private:
    QList<Accounts::AccountId> m_cleanupList;
};


void tst_Account::init()
{
    Accounts::Manager *gam = globalAccountManager();
    connect(gam, SIGNAL(accountCreated(Accounts::AccountId)),
            this, SLOT(trackAccountAdded(Accounts::AccountId)), Qt::UniqueConnection);
    connect(gam, SIGNAL(accountRemoved(Accounts::AccountId)),
            this, SLOT(untrackAccountRemoved(Accounts::AccountId)), Qt::UniqueConnection);
}

void tst_Account::cleanup()
{
    foreach (Accounts::AccountId idToRemove, m_cleanupList) {
        QScopedPointer<Account> doomed(new Account);
        doomed->classBegin();
        doomed->setIdentifier(idToRemove);
        doomed->componentComplete();
        QTRY_VERIFY(doomed->status() == Account::Initialized || doomed->status() == Account::Synced);
        doomed->remove();
        QTest::qWait(150); // wait for db sync to complete.
    }
    m_cleanupList.clear();
}

void tst_Account::trackAccountAdded(Accounts::AccountId id)
{
    m_cleanupList.append(id);
}

void tst_Account::untrackAccountRemoved(Accounts::AccountId id)
{
    m_cleanupList.removeAll(id);
}

void tst_Account::enabled()
{
    Accounts::Manager manager;
    QScopedPointer<Accounts::Account> newA(manager.createAccount("test-provider"));
    QSignalSpy newASyncedSpy(newA.data(), SIGNAL(synced()));
    QList<QVariant> spyArgs;
    newA->setDisplayName("test");
    newA->setEnabled(false);
    newA->sync();
    QTRY_VERIFY(newASyncedSpy.count() > 0);

    // first, without account creation / sync
    QScopedPointer<Account> account(new Account);
    account->classBegin();
    account->setIdentifier(newA->id());
    account->setDisplayName("test-display-name");
    QCOMPARE(account->enabled(), false);
    QSignalSpy spy(account.data(), SIGNAL(enabledChanged()));
    account->setEnabled(true);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(account->enabled(), true);
    account->setEnabled(false);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(account->enabled(), false);

    // now, with sync.
    account->setEnabled(true);
    account->sync(); // pending sync
    account->componentComplete(); // will construct new account.
    QTRY_COMPARE(account->status(), Account::Synced); // wait for sync.
    QVERIFY(account->enabled());

    // and ensure that it's globally enabled in the database.
    Accounts::Manager m;
    Accounts::Account *a = m.account(account->identifier());
    QVERIFY(a->enabled());

    // disable it
    account->setEnabled(false);
    account->sync();
    QTRY_COMPARE(account->status(), Account::Synced); // wait for sync.

    // ensure that it's globally disabled in the database.
    QVERIFY(!a->enabled());

    // cleanup.
    account->remove();
}

void tst_Account::identifier()
{
    QScopedPointer<Account> account(new Account);

    Accounts::Manager manager;
    QScopedPointer<Accounts::Account> newA(manager.createAccount("test-provider"));
    QSignalSpy newASyncedSpy(newA.data(), SIGNAL(synced()));
    QList<QVariant> spyArgs;
    newA->setDisplayName("test-display-name");
    newA->setEnabled(false);
    newA->sync();
    QTRY_VERIFY(newASyncedSpy.count() > 0);

    // existing account identifier
    {
        QScopedPointer<Account> existing(new Account);
        existing->classBegin();
        existing->setIdentifier(newA->id());
        existing->componentComplete();
        QTRY_COMPARE(existing->status(), Account::Initialized);
        QCOMPARE(existing->displayName(), QLatin1String("test-display-name"));
    }

    // existing account identifier set after initialization
    {
        QScopedPointer<Account> existing(new Account);
        existing->classBegin();
        existing->componentComplete();
        QTRY_COMPARE(existing->status(), Account::Invalid);
        existing->setIdentifier(newA->id());
        QCOMPARE(existing->status(), Account::Initializing);
        QTRY_COMPARE(existing->status(), Account::Initialized);
        QCOMPARE(existing->displayName(), QLatin1String("test-display-name"));
    }

    // cleanup.
    account->remove();
}

void tst_Account::displayName()
{
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
    QCOMPARE(account->displayName(), QString(QLatin1String("")));
    QSignalSpy spy(account.data(), SIGNAL(displayNameChanged()));
    account->setDisplayName(QString(QLatin1String("test-display-name")));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(account->displayName(), QString(QLatin1String("test-display-name")));
    account->sync(); // pending sync.
    account->componentComplete();
    QTRY_COMPARE(account->status(), Account::Synced);
    account->setDisplayName(QString(QLatin1String("test-display-name-two")));
    account->sync();
    QTRY_COMPARE(account->status(), Account::Synced);
    QCOMPARE(account->displayName(), QString(QLatin1String("test-display-name-two")));
    account->remove();
}

void tst_Account::providerName()
{
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
    account->componentComplete();
    QTRY_VERIFY(account->providerName() == QLatin1String("test-provider"));
    account->remove();
}

void tst_Account::supportedServiceNames()
{
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
    account->componentComplete();
    QTRY_VERIFY(account->supportedServiceNames().contains(QString(QLatin1String("test-service2"))));
    account->remove();
}

void tst_Account::enableDisableWithService()
{
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
    account->setDisplayName("test-display-name");
    account->sync();
    account->componentComplete();
    QTRY_COMPARE(account->status(), Account::Synced);

    account->enableWithService(QString(QLatin1String("test-service2")));
    QCOMPARE(account->status(), Account::Modified);
    account->sync();
    QTRY_COMPARE(account->status(), Account::Synced);

    // ensure that the account really has been enabled as we expect.
    Accounts::Manager m;
    Accounts::Account *a = m.account(account->identifier());
    QVERIFY(a->enabledServices().contains(m.service(QLatin1String("test-service2"))));

    // now disable and test again
    account->disableWithService(QString(QLatin1String("test-service2")));
    account->sync();
    QTRY_COMPARE(account->status(), Account::Synced);

    // ensure that the account really has been disabled as we expect.
    QVERIFY(!a->enabledServices().contains(m.service(QLatin1String("test-service2"))));

    // cleanup
    account->remove();
}

void tst_Account::configurationValues()
{
    QVariantMap testData;
    QString testKey(QLatin1String("test-key"));
    QVariant testStrListValue(QStringList() << QLatin1String("first") << QLatin1String("second"));
    QVariant testStrValue(QString(QLatin1String("test-value")));
    QVariant testBoolValue(true);
    QVariant testIntValue(-5);
    QVariant testQuintValue(quint64(0xaaaaaaaaaaaa));
    testData.insert(testKey, testStrValue);

    QVariantMap noValueTestData;
    noValueTestData.insert(testKey, QVariant());

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
    QCOMPARE(account->configurationValues(QString()), QVariantMap());
    account->setConfigurationValues(QString(), testData);
    QCOMPARE(account->configurationValues(QString()), testData);
    account->removeConfigurationValue(QString(), testKey);
    QCOMPARE(account->configurationValues(QString()), QVariantMap());

    // invalid values
    account->setConfigurationValue(QString(), testKey, QVariant(QColor(Qt::black)));
    QCOMPARE(account->configurationValues(QString()), QVariantMap()); // not set.
    account->setConfigurationValue(QString(), testKey, QVariant());
    QCOMPARE(account->configurationValues(QString()), QVariantMap()); // not set.

    // bool, int, quint64 and string should all work.
    account->setConfigurationValue(QString(), testKey, testBoolValue);
    QCOMPARE(account->configurationValues(QString()).value(testKey), testBoolValue);
    account->setConfigurationValue(QString(), testKey, testIntValue);
    QCOMPARE(account->configurationValues(QString()).value(testKey), testIntValue);
    account->setConfigurationValue(QString(), testKey, testQuintValue);
    QCOMPARE(account->configurationValues(QString()).value(testKey), testQuintValue);
    account->setConfigurationValue(QString(), testKey, testStrValue);
    QCOMPARE(account->configurationValues(QString()).value(testKey), testStrValue);
    account->setConfigurationValue(QString(), testKey, testStrListValue);
    QCOMPARE(account->configurationValues(QString()).value(testKey), testStrListValue);

    // ensure that configuration values can be saved.
    account->sync(); // pending sync.
    account->componentComplete(); // will create new account.
    QTRY_COMPARE(account->status(), Account::Synced);
    QCOMPARE(account->configurationValues(QString()).value(testKey), testStrListValue);
    account->setConfigurationValue(QString(), testKey, testStrValue);
    account->sync();
    QTRY_COMPARE(account->status(), Account::Synced);
    QCOMPARE(account->configurationValues(QString()).value(testKey), testStrValue);
    account->setConfigurationValue(QString(), testKey, testQuintValue);
    account->sync();
    QTRY_COMPARE(account->status(), Account::Synced);
    QCOMPARE(account->configurationValues(QString()).value(testKey), testQuintValue);
    account->setConfigurationValue(QString(), testKey, testIntValue);
    account->sync();
    QTRY_COMPARE(account->status(), Account::Synced);
    QCOMPARE(account->configurationValues(QString()).value(testKey), testIntValue);
    account->setConfigurationValue(QString(), testKey, testBoolValue);
    account->sync();
    QTRY_COMPARE(account->status(), Account::Synced);
    QCOMPARE(account->configurationValues(QString()).value(testKey), testBoolValue);

    // ensure that configuration values from subgroups are reported correctly.
    // and ensure that stringlist configuration values are reported correctly.
    QString testGroup = QLatin1String("test-group");
    Accounts::Manager m;
    Accounts::Account *a = m.account(account->identifier());
    QVERIFY(a != 0);
    a->selectService(Accounts::Service());
    a->beginGroup(testGroup);
    a->setValue(testKey, testStrValue);
    a->endGroup();
    a->setValue(testKey, testStrListValue);
    a->sync();
    QSignalSpy aSyncedSpy(a, SIGNAL(synced()));
    QTRY_VERIFY(aSyncedSpy.count() > 0);

    // account doesn't emit signals on configuration values changed...
    QScopedPointer<Account> existingAccount(new Account);
    existingAccount->classBegin();
    existingAccount->setIdentifier(account->identifier());
    existingAccount->componentComplete(); // will load existing account
    QTRY_COMPARE(existingAccount->status(), Account::Initialized);
    QCOMPARE(existingAccount->configurationValues(QString()).value(QString("%1/%2").arg(testGroup).arg(testKey)), testStrValue);
    QCOMPARE(existingAccount->configurationValues(QString()).value(testKey), testStrListValue);

    // and ensure that changes are really synced
    account->setConfigurationValue(QString(), testKey, testStrValue);
    QCOMPARE(account->status(), Account::Modified);
    account->sync();
    QTRY_COMPARE(account->status(), Account::Synced);
    QVariant expectString(QVariant::String);
    a->value(testKey, expectString); // expectString is an in-out argument.
    QCOMPARE(expectString, testStrValue);
    account->setConfigurationValue(QString(), testKey, testStrListValue);
    account->sync();
    QTRY_COMPARE(account->status(), Account::Synced);
    QVariant expectStringList(QVariant::StringList);
    a->value(testKey, expectStringList); // expectStringList is an in-out argument.
    QCOMPARE(expectStringList, testStrListValue);

    // cleanup.
    account->remove();
}

void tst_Account::serviceConfigurationValues()
{
    QVariantMap testData;
    QString testKey(QLatin1String("service-test-key")); // different to key in previous test, to avoid overlap.
    QVariant testStrListValue(QStringList() << QLatin1String("first") << QLatin1String("second"));
    QVariant testStrValue(QString(QLatin1String("test-value")));
    QVariant testBoolValue(true);
    QVariant testIntValue(-5);
    QVariant testQuintValue(quint64(0xaaaaaaaaaaaa));
    testData.insert(testKey, testStrValue);
    QString testServiceName = QLatin1String("test-service2");

    QVariantMap noValueTestData;
    noValueTestData.insert(testKey, QVariant());

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
    account->componentComplete();
    QTRY_COMPARE(account->status(), Account::Initialized);

    account->setConfigurationValues(testServiceName, testData);
    QCOMPARE(account->configurationValues(testServiceName), testData);
    account->removeConfigurationValue(testServiceName, testKey);
    QCOMPARE(account->configurationValues(testServiceName), QVariantMap());

    // invalid values
    account->setConfigurationValue(testServiceName, testKey, QVariant(QColor(Qt::black)));
    QCOMPARE(account->configurationValues(testServiceName), QVariantMap()); // not set.
    account->setConfigurationValue(testServiceName, testKey, QVariant());
    QCOMPARE(account->configurationValues(testServiceName), QVariantMap()); // not set.

    // bool, int, quint64 and string should all work.
    account->setConfigurationValue(testServiceName, testKey, testBoolValue);
    QCOMPARE(account->configurationValues(testServiceName).value(testKey), testBoolValue);
    account->setConfigurationValue(testServiceName, testKey, testIntValue);
    QCOMPARE(account->configurationValues(testServiceName).value(testKey), testIntValue);
    account->setConfigurationValue(testServiceName, testKey, testQuintValue);
    QCOMPARE(account->configurationValues(testServiceName).value(testKey), testQuintValue);
    account->setConfigurationValue(testServiceName, testKey, testStrValue);
    QCOMPARE(account->configurationValues(testServiceName).value(testKey), testStrValue);
    account->setConfigurationValue(testServiceName, testKey, testStrListValue);
    QCOMPARE(account->configurationValues(testServiceName).value(testKey), testStrListValue);

    // ensure that configuration values can be saved.
    account->sync(); // pending sync.
    QTRY_COMPARE(account->status(), Account::Synced);
    QCOMPARE(account->configurationValues(testServiceName).value(testKey), testStrListValue);
    account->setConfigurationValue(testServiceName, testKey, testStrValue);
    account->sync();
    QTRY_COMPARE(account->status(), Account::Synced);
    QCOMPARE(account->configurationValues(testServiceName).value(testKey), testStrValue);
    account->setConfigurationValue(testServiceName, testKey, testQuintValue);
    account->sync();
    QTRY_COMPARE(account->status(), Account::Synced);
    QCOMPARE(account->configurationValues(testServiceName).value(testKey), testQuintValue);
    account->setConfigurationValue(testServiceName, testKey, testIntValue);
    account->sync();
    QTRY_COMPARE(account->status(), Account::Synced);
    QCOMPARE(account->configurationValues(testServiceName).value(testKey), testIntValue);
    account->setConfigurationValue(testServiceName, testKey, testBoolValue);
    account->sync();
    QTRY_COMPARE(account->status(), Account::Synced);
    QCOMPARE(account->configurationValues(testServiceName).value(testKey), testBoolValue);

    // ensure that configuration values from subgroups are reported correctly.
    // and ensure that stringlist configuration values are reported correctly.
    QString testGroup = QLatin1String("test-group");
    Accounts::Manager m;
    Accounts::Account *a = m.account(account->identifier());
    QVERIFY(a != 0);
    Accounts::Service s = m.service(testServiceName);
    QVERIFY(s.isValid());
    a->selectService(s);
    a->beginGroup(testGroup);
    a->setValue(testKey, testStrValue);
    a->endGroup();
    a->setValue(testKey, testStrListValue);
    a->sync();
    QSignalSpy aSyncedSpy(a, SIGNAL(synced()));
    QTRY_VERIFY(aSyncedSpy.count() > 0);

    // account doesn't emit signals on configuration values changed...
    // we really need a "refresh" function, similar to the one in Identity.
    QScopedPointer<Account> existingAccount(new Account);
    existingAccount->classBegin();
    existingAccount->setIdentifier(account->identifier());
    existingAccount->componentComplete(); // will load existing account
    QTRY_COMPARE(existingAccount->status(), Account::Initialized);
    QCOMPARE(existingAccount->configurationValues(testServiceName).value(QString("%1/%2").arg(testGroup).arg(testKey)), testStrValue);
    QCOMPARE(existingAccount->configurationValues(testServiceName).value(testKey), testStrListValue);

    // and ensure that changes are really synced
    account->setConfigurationValue(testServiceName, testKey, testStrValue);
    account->sync();
    QTRY_COMPARE(account->status(), Account::Synced);
    QVariant expectString(QVariant::String);
    a->value(testKey, expectString); // expectString is an in-out parameter.
    QCOMPARE(expectString, testStrValue);
    account->setConfigurationValue(testServiceName, testKey, testStrListValue);
    account->sync();
    QTRY_COMPARE(account->status(), Account::Synced);
    QVariant expectStringList(QVariant::StringList);
    a->value(testKey, expectStringList); // expectStringList is an in-out parameter.
    QCOMPARE(expectStringList, testStrListValue);

    // cleanup.
    account->remove();
}

void tst_Account::status()
{
    Accounts::Manager manager;
    QScopedPointer<Accounts::Account> newA(manager.createAccount("test-provider"));
    QSignalSpy newASyncedSpy(newA.data(), SIGNAL(synced()));
    QList<QVariant> spyArgs;
    newA->setDisplayName("test");
    newA->setEnabled(false);
    newA->sync();
    QTRY_VERIFY(newASyncedSpy.count() > 0);

    QScopedPointer<Account> account(new Account);
    QSignalSpy spy(account.data(), SIGNAL(statusChanged()));
    account->classBegin();
    QCOMPARE(account->status(), Account::Initializing);
    account->setIdentifier(newA->id());
    account->setDisplayName(QString(QLatin1String("test-display-name")));
    QCOMPARE(account->status(), Account::Initializing); // despite modifications, should still be initializing, until componentComplete().
    account->sync(); // trigger pending sync().
    account->componentComplete();
    // now we return to event loop.
    // Status should transition: (Initializing) -> Initialized -> Modified
    //                         -> SyncInProgress -> Synced.
    QTRY_COMPARE(spy.count(), 4);
    QCOMPARE(account->status(), Account::Synced);
    account->setDisplayName(QString(QLatin1String("test-display-name-two")));
    QCOMPARE(spy.count(), 5);
    QCOMPARE(account->status(), Account::Modified);
    account->sync();
    QTRY_COMPARE(spy.count(), 7); // SyncInProgress->Synced.
    QCOMPARE(account->status(), Account::Synced);
    QVERIFY(account->identifier() > 0); // should have saved the account successfully.

    // cleanup.
    Accounts::Manager m;
    Accounts::Account *a = m.account(account->identifier());
    QVERIFY(a != 0);
    a->remove();
    a->sync();

    QTRY_COMPARE(spy.count(), 8); // Invalid.
    QCOMPARE(account->status(), Account::Invalid);
}

void tst_Account::error()
{
    // XXX TODO
}

void tst_Account::errorMessage()
{
    // XXX TODO
}

void tst_Account::credentialsFunctions()
{
    quint32 nullCredentials = 0;

    // Create account
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
    account->setDisplayName("test-display-name");
    account->sync();
    account->componentComplete();
    QTRY_COMPARE(account->status(), Account::Synced);

    // set up our spies.
    QSignalSpy siccSpy(account.data(), SIGNAL(signInCredentialsCreated(QVariantMap)));
    QSignalSpy sirSpy(account.data(), SIGNAL(signInResponse(QVariantMap)));
    QSignalSpy sieSpy(account.data(), SIGNAL(signInError(QString)));
    int siccCount = siccSpy.count();
    int sirCount = sirSpy.count();
    int sieCount = sieSpy.count();

/* XXX TODO: enable this test once the Store Provided Tokens functionality works!

    // Create credentials (oauth2)
    SignInParameters *sip = account->signInParameters("test-service-oauth");
    QVariantMap params = sip->parameters();
    params.insert("ClientId", "TestClientId");
    // testing only: we set the tokens to store, so that the signond doesn't attempt to actually log into the test service.
    QVariantMap providedTokens;
    providedTokens.insert("AccessToken", "TestAccessToken");
    providedTokens.insert("RefreshToken", "TestRefreshToken");
    params.insert("ProvidedTokens", providedTokens);
    sip->setParameters(params);
    account->createSignInCredentials("test", "test", sip);
    QCOMPARE(account->status(), Account::SigningIn);

    // ensure success
    QTRY_COMPARE(siccSpy.count(), siccCount+1);
    siccCount = siccSpy.count();

    // ensure returned tokens are the expected values
    QVariantMap responseData = siccSpy.takeFirst().at(0).toMap();
    QCOMPARE(responseData.value("AccessToken").toString(), QString(QLatin1String("TestAccessToken")));
    QCOMPARE(responseData.value("RefreshToken").toString(), QString(QLatin1String("TestRefreshToken")));

    // check that the identity id is valid - stored into the account configuration settings.
    QString configKey = BUILD_CREDENTIALS_CONFIGURATION_KEY("test", "test");
    quint32 firstOAuthIdentityId = account->configurationValues("").value(configKey).toInt();
    QVERIFY(firstOAuthIdentityId != nullCredentials);

    // Create new credentials (oauth2)
    SignInParameters *sip2 = account->signInParameters("test-service-oauth");
    QVariantMap paramsTwo = sip2->parameters();
    paramsTwo.insert("ClientId", "TestClientIdTwo");
    // testing only: we set the tokens to store, so that the signond doesn't attempt to actually log into the test service.
    QVariantMap providedTokensTwo;
    providedTokensTwo.insert("AccessToken", "TestAccessTokenTwo");
    providedTokensTwo.insert("RefreshToken", "TestRefreshTokenTwo");
    paramsTwo.insert("ProvidedTokens", providedTokens);
    sip2->setParameters(paramsTwo);
    account->createSignInCredentials("testTwo", "testTwo", sip2);
    QCOMPARE(account->status(), Account::SigningIn);

    // ensure success
    QTRY_COMPARE(siccSpy.count(), siccCount+1);
    siccCount = siccSpy.count();
    QVariantMap responseDataTwo = siccSpy.takeFirst().at(0).toMap();
    QCOMPARE(responseDataTwo.value("AccessToken").toString(), QString(QLatin1String("TestAccessTokenTwo")));
    QCOMPARE(responseDataTwo.value("RefreshToken").toString(), QString(QLatin1String("TestRefreshTokenTwo")));

    // should reuse the same identity id (as oauth is segregated in signond via ClientId)
    QString configKeyTwo = BUILD_CREDENTIALS_CONFIGURATION_KEY("testTwo", "testTwo");
    quint32 secondOAuthIdentityId = account->configurationValues("").value(configKeyTwo).toInt();
    QCOMPARE(secondOAuthIdentityId, firstOAuthIdentityId);
    QCOMPARE(newA->credentialsId(), firstOAuthIdentityId); // default set

    // ensure that signing in with the first one, still returns the first tokens.
    params.remove("ProvidedTokens"); // want signond to returned the cached ones.
    sip->setParameters(params);
    account->signIn("test", "test", sip);
    QCOMPARE(account->status(), Account::SigningIn);
    QTRY_COMPARE(sirSpy.count(), sirCount+1);
    sirCount = sirSpy.count();
    responseData = sirSpy.takeFirst().at(0).toMap();
    QCOMPARE(responseData.value("AccessToken").toString(), QString(QLatin1String("TestAccessToken")));
    QCOMPARE(responseData.value("RefreshToken").toString(), QString(QLatin1String("TestRefreshToken")));

    // sign out + signin specifying NoUserInteraction should return empty map (sign out clears tokens)
    account->signOut("test", "test"); // clears all tokens
    params.insert("UiPolicy", SignOn::NoUserInteractionPolicy);
    sip->setParameters(params);
    account->signIn("test", "test", sip);
    QCOMPARE(account->status(), Account::SigningIn);
    QTRY_COMPARE(sirSpy.count(), sirCount+1);
    sirCount = sirSpy.count();
    responseData = sirSpy.takeFirst().at(0).toMap();
    QCOMPARE(responseData.value("AccessToken").toString(), QString());
    QCOMPARE(responseData.value("RefreshToken").toString(), QString());

    // remove second oauth2 credentials -> should NOT result in the default being unset.
    account->removeSignInCredentials("testTwo", "testTwo");
    QCOMPARE(account->status(), Account::SyncInProgress);
    QTRY_COMPARE(account->status(), Account::Synced);
    firstOAuthIdentityId = account->configurationValues("").value(configKey).toInt();
    secondOAuthIdentityId = account->configurationValues("").value(configKeyTwo).toInt();
    QCOMPARE(secondOAuthIdentityId, nullCredentials); // removed
    QVERIFY(firstOAuthIdentityId != nullCredentials); // not removed
    QCOMPARE(newA->credentialsId(), firstOAuthIdentityId); // default still exists

    // remove first oauth2 credentials -> should result in the default being unset, as no more usages.
    account->removeSignInCredentials("test", "test");
    QCOMPARE(account->status(), Account::SyncInProgress);
    QTRY_COMPARE(account->status(), Account::Synced);
    firstOAuthIdentityId = account->configurationValues("").value(configKey).toInt();
    secondOAuthIdentityId = account->configurationValues("").value(configKeyTwo).toInt();
    QCOMPARE(secondOAuthIdentityId, nullCredentials); // removed
    QCOMPARE(firstOAuthIdentityId, nullCredentials); // removed
    QCOMPARE(newA->credentialsId(), nullCredentials); // default removed

Store Provided Tokens ^^ */

    //--------------------------------------------------

    // Create credentials (non-oauth2) with symmetric key
    SignInParameters *sip3 = account->signInParameters("test-service2", "user", "pass");
    account->createSignInCredentials("testThree", "testThree", sip3, "symmetricKey");
    QCOMPARE(account->status(), Account::SigningIn);

    // ensure success
    QTRY_COMPARE(siccSpy.count(), siccCount+1);
    siccCount = siccSpy.count();

    // ensure returned username/pass matches expectation
    QVariantMap responseDataThree = siccSpy.takeFirst().at(0).toMap();
    QCOMPARE(responseDataThree.value("UserName").toString(), QString(QLatin1String("user")));
    QCOMPARE(responseDataThree.value("Secret").toString(), QString(QLatin1String("pass")));

    // ensure no "default" was set (as symmetric key was given)
    Accounts::Service whichSrv = manager.service("test-service2");
    newA->selectService(whichSrv);
    QCOMPARE(newA->credentialsId(), nullCredentials);
    newA->selectService(Accounts::Service());

    // ensure that sign in succeeds if the correct symmetric key is given
    account->signIn("testThree", "testThree", sip3, "SymmetricKey");
    QTRY_COMPARE(sirSpy.count(), sirCount+1);
    sirCount = sirSpy.count();
    QTRY_COMPARE(account->status(), Account::Synced); // should transition to synced after either successful or failed sign in

    // ensure that sign in fails if the wrong symmetric key is given
    sieCount = sieSpy.count();
    account->signIn("testThree", "testThree", sip3, "WrongSymmetricKey");
    QTRY_COMPARE(sieSpy.count(), sieCount+1);
    sieCount = sieSpy.count();
    QTRY_COMPARE(account->status(), Account::Synced); // should transition to synced after either successful or failed sign in

    // Create credentials (non-oauth2) without symmetric key
    siccCount = siccSpy.count();
    SignInParameters *sip4 = account->signInParameters("test-service2", "userFour", "passFour");
    account->createSignInCredentials("testFour", "testFour", sip4);
    QCOMPARE(account->status(), Account::SigningIn);

    // ensure returned username/pass matches expectation
    QTRY_COMPARE(siccSpy.count(), siccCount+1);
    siccCount = siccSpy.count();
    QVariantMap responseDataFour = siccSpy.takeFirst().at(0).toMap();
    QCOMPARE(responseDataFour.value("UserName").toString(), QString(QLatin1String("userFour")));
    QCOMPARE(responseDataFour.value("Secret").toString(), QString(QLatin1String("passFour")));

    // ensure that it was set as default for the service
    newA->selectService(whichSrv);
    QVERIFY(newA->credentialsId() != nullCredentials);
    newA->selectService(Accounts::Service());

    // remove credentials
    account->removeSignInCredentials("testFour", "testFour");
    account->removeSignInCredentials("testThree", "testThree");

    // remove account.
    account->remove();
}

void tst_Account::loadSavedAccount()
{
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
    account->setDisplayName("test-display-name");
    account->sync();
    account->componentComplete();
    QTRY_COMPARE(account->status(), Account::Synced);
    QTRY_VERIFY(account->supportedServiceNames().contains(QString(QLatin1String("test-service2"))));
    account->enableWithService(QString(QLatin1String("test-service2")));
    account->sync();
    QTRY_COMPARE(account->status(), Account::Synced);

    QScopedPointer<Account> readOnlyAccount(new Account);
    QSignalSpy spyProviderName(readOnlyAccount.data(), SIGNAL(providerNameChanged()));
    QSignalSpy spyDisplayName(readOnlyAccount.data(), SIGNAL(displayNameChanged()));
    QSignalSpy spySupportedServiceNames(readOnlyAccount.data(), SIGNAL(supportedServiceNamesChanged()));

    readOnlyAccount->classBegin();
    readOnlyAccount->setIdentifier(account->identifier());
    readOnlyAccount->componentComplete();
    QTRY_COMPARE(readOnlyAccount->status(), Account::Initialized);
    QCOMPARE(readOnlyAccount->identifier(), account->identifier());
    QCOMPARE(readOnlyAccount->providerName(), account->providerName());
    QCOMPARE(readOnlyAccount->displayName(), account->displayName());
    QCOMPARE(readOnlyAccount->supportedServiceNames(), account->supportedServiceNames());

    QCOMPARE(spyProviderName.count(), 1);
    QCOMPARE(spyDisplayName.count(), 1);
    QCOMPARE(spySupportedServiceNames.count(), 1);

    account->remove();
}



void tst_Account::expectedUsage()
{
    // it's meant to be used in QML as a creatable type.
    // generally, they'll do:
    // Item {
    //     Account {
    //         id: account
    //         identifier: 15 // from an AccountManager
    //         displayName: "new account name"
    //         onStatusChanged: {
    //             if (status == Account.Synced) {
    //                 console.log("updated display name of account")
    //             }
    //         }
    //     }
    //
    //     Component.onCompleted: account.sync() // usually triggered by button press/etc
    // }

    Accounts::Manager manager;
    QScopedPointer<Accounts::Account> newA(manager.createAccount("test-provider"));
    QSignalSpy newASyncedSpy(newA.data(), SIGNAL(synced()));
    QList<QVariant> spyArgs;
    newA->setDisplayName("test");
    newA->setEnabled(false);
    newA->sync();
    QTRY_VERIFY(newASyncedSpy.count() > 0);

    // first, with a new account
    QScopedPointer<Account> newAccount(new Account); // we reuse it in the existing test
    newAccount->classBegin();
    newAccount->setIdentifier(newA->id());
    newAccount->setDisplayName("test-account");
    QCOMPARE(newAccount->status(), Account::Initializing);
    newAccount->componentComplete();
    QTRY_COMPARE(newAccount->status(), Account::Modified); // outstanding modifications.
    newAccount->enableWithService("test-service2");
    newAccount->sync(); // common case is to trigger sync after completes.
    QTRY_COMPARE(newAccount->status(), Account::Synced);
    QCOMPARE(newAccount->providerName(), QString(QLatin1String("test-provider")));
    QCOMPARE(newAccount->displayName(), QString(QLatin1String("test-account")));
    QCOMPARE(newAccount->enabled(), false); // disabled by default.
    QCOMPARE(newAccount->isEnabledWithService(QLatin1String("test-service2")), true);
    QVERIFY(newAccount->identifier() > 0); // saved; valid id.

    // and again, with the same account
    QScopedPointer<Account> existingAccount(new Account);
    existingAccount->classBegin();
    existingAccount->setIdentifier(newAccount->identifier());
    existingAccount->setDisplayName("test-account-modified");
    QCOMPARE(existingAccount->status(), Account::Initializing);
    existingAccount->componentComplete();
    QTRY_COMPARE(existingAccount->status(), Account::Modified);
    existingAccount->sync(); // common case is to trigger sync before completes.
    QTRY_COMPARE(existingAccount->status(), Account::Synced);
    QCOMPARE(existingAccount->identifier(), newAccount->identifier()); // same id
    QCOMPARE(existingAccount->providerName(), QString(QLatin1String("test-provider")));
    QCOMPARE(existingAccount->displayName(), QString(QLatin1String("test-account-modified")));

    // now, it should signal the change to this account.
    QTRY_COMPARE(newAccount->displayName(), QString(QLatin1String("test-account-modified")));

    // clean them both up.
    newAccount->remove(); // removing one should remove both.
    QTRY_COMPARE(newAccount->status(), Account::Invalid);
    QTRY_COMPARE(existingAccount->status(), Account::Invalid);
}

#include "tst_account.moc"
QTEST_MAIN(tst_Account)
