/*
 * Copyright (C) 2013 Jolla Ltd.
 * Contact: Chris Adams <chris.adams@jollamobile.com>
 *
 * License: Proprietary
 */

#include "account.h"
#include "account_p.h"

#include "signinparameters.h"
#include "globalaccountmanager_p.h"
#include "accountvalueencoding_p.h"
#include "accountvalueencryption_p.h"

#include <QtDebug>

//libaccounts-qt
#include <Accounts/Manager>
#include <Accounts/Account>
#include <Accounts/Service>
#include <Accounts/AccountService>
#include <Accounts/AuthData>

//libsignon-qt
#include <SignOn/Identity>
#include <SignOn/SessionData>
#include <SignOn/AuthSession>

#define CREDENTIALS_GROUP QLatin1String("segregated_credentials")
#define BUILD_CREDENTIALS_CONFIGURATION_KEY(appName, credName) QString(QLatin1String("%1/%2/%3")).arg(appName).arg(CREDENTIALS_GROUP).arg(credName)

/*
    High-level description:

    Credentials are segregated by application.
    Each Account will contain configuration values of the form:

        application_name/segregated_credentials/credentials_name=IdentityId

    For example:

        cool_email/segregated_credentials/smtp=5
        cool_email/segregated_credentials/imap=6
        uber_social/segregated_credentials/facebook=7
        uber_social/segregated_credentials/twitter=8
        chatterbox/segregated_credentials/jabber=9

    When the credentials are created, if they are a username/password, then
    they are encrypted using a symmetricKey prior to storing to database.
    Client applications who wish to retrieve the plaintext credentials must
    pass in the correct symmetricKey.

    If the credentials are OAuth1.0a or OAuth2 based, the OAuth tokens are
    stored in a per-ClientId (or per-ConsumerKey) map.  Clients are therefore
    unable to use or access tokens associated with a different application.
*/

// encrypts the given plaintext string with the given key, encodes the result in base64.
static QString b64_encrypted_string(const QString &plaintext, const QString &key)
{
    QByteArray ptBA = plaintext.toUtf8();
    QByteArray kBA = key.toUtf8();

    QByteArray encryptedData;
    if (key.isEmpty()) {
        // no key: don't encrypt.
        encryptedData = ptBA;
    } else {
        encryptedData = aes_encrypt_plaintext(ptBA, kBA);
        if (encryptedData.size() == 0) {
            qWarning() << Q_FUNC_INFO << "encryption failed";
            return QString();
        }
    }

    QByteArray b64encryptedData = encryptedData.toBase64();
    return QString::fromLatin1(b64encryptedData);
}

// decodes the given ciphertext from base64 into encrypted data, and decrypts it.
static QString decrypted_string_b64(const QString &ciphertext, const QString &key)
{
    QByteArray b64encryptedData = ciphertext.toLatin1();
    QByteArray encryptedData = QByteArray::fromBase64(b64encryptedData);
    QByteArray kBA = key.toUtf8();

    QByteArray decryptedData;
    if (key.isEmpty()) {
        // no key: don't decrypt
        decryptedData = encryptedData;
    } else {
        decryptedData = aes_decrypt_ciphertext(encryptedData, kBA);
        if (decryptedData.size() == 0) {
            qWarning() << Q_FUNC_INFO << "decryption failed";
            return QString();
        }
    }

    QString decryptedString = QString::fromUtf8(decryptedData);
    return decryptedString;
}



void SignInCredentials::cleanup(bool removeIdentity)
{
    if (identity != NULL) {
        if (session != NULL) {
            identity->destroySession(session);
        }

        if (removeIdentity == true) {
            identity->signOut();
            identity->remove();
        }

        identity->deleteLater();
    }

    creatingSignInCredentials = false;
    signingInWithCredentials = false;
    storingEncryptedTokens = false;

    identityInfo = SignOn::IdentityInfo();

    applicationName = QString();
    symmetricKey = QString();
    method = QString();
    mechanism = QString();
    sessionData = QVariantMap();
    credentialsName = QString();
    username = QString();
    password = QString();

    responseData = QVariantMap();
}

AccountPrivate::AccountPrivate(Account *parent, Accounts::Account *acc, bool queryInfo)
    : QObject(parent)
    , q(parent)
    , manager(globalAccountManager())
    , account(0)
    , pendingSync(false)
    , pendingInitModifications(false)
    , identifier(0)
    , enabled(false)
    , identifierPendingInit(false)
    , enabledPendingInit(false)
    , displayNamePendingInit(false)
    , configurationValuesPendingInit(false)
    , status(Account::Initializing)
    , error(Account::NoError)
{
    // initialize the signInCredentials struct
    signInCredentials.creatingSignInCredentials = false;
    signInCredentials.signingInWithCredentials = false;
    signInCredentials.storingEncryptedTokens = false;
    signInCredentials.identity = NULL;
    signInCredentials.session = NULL;

    // set up the account
    if (acc) {
        setAccount(acc, queryInfo);
    }
}

AccountPrivate::~AccountPrivate()
{
}

void AccountPrivate::setAccount(Accounts::Account *acc, bool queryInfo)
{
    if (!acc) {
        qWarning() << "Account: setAccount() called with null account! Aborting operation.";
        return;
    }

    if (account) {
        qWarning() << "Account: setAccount() called but account already set! Aborting operation.";
        return;
    }

    account = acc;

    // connect up our signals.
    connect(account, SIGNAL(enabledChanged(QString,bool)), this, SLOT(enabledHandler(QString,bool)));
    connect(account, SIGNAL(displayNameChanged(QString)), this, SLOT(displayNameChangedHandler()));
    connect(account, SIGNAL(synced()), this, SLOT(handleSynced()));
    connect(account, SIGNAL(removed()), this, SLOT(invalidate()));
    connect(account, SIGNAL(destroyed()), this, SLOT(invalidate()));

    // grab the supported service list: this is necessary for enablement etc.
    Accounts::ServiceList supportedServices = account->services();
    for (int i = 0; i < supportedServices.size(); ++i) {
        const Accounts::Service &currService(supportedServices.at(i));
        supportedServiceNames.append(currService.name());
    }

    if (queryInfo) {
        // first time read from db.  we should be in Initializing state to begin with.
        // QueuedConnection to ensure that clients have a chance to connect to state changed signals.
        QMetaObject::invokeMethod(this, "asyncQueryInfo", Qt::QueuedConnection);

        // NOTE: we can't escape the asyncQueryInfo (eg, via a "queryInfoOnInit: false" property
        // or similar, simply because the application-segregation requires the configuration settings
        // to be read for the account (and, presumably, the service enablement statuses).
    } else {
        // We can skip it via the AccountFactory, however, as we know that the account is "new".
        // Thus we have a (private) queryInfo=false bool for the AccountFactory case only.
        setStatus(Account::Initialized);
    }
}

void AccountPrivate::asyncQueryInfo()
{
    if (!account) {
        qWarning() << "Account: no account set!  Maybe you forgot to call componentComplete()?";
        setStatus(Account::Invalid);
        return;
    }

    // note that the account doesn't have a queryInfo() like Identity
    // so we just read the values directly.

    int newIdentifier = account->id();
    if (identifier != newIdentifier) {
        identifier = account->id();
        emit q->identifierChanged();
    }

    if (providerName != account->providerName()) {
        providerName = account->providerName();
        emit q->providerNameChanged();
    }

    // supported service names
    Accounts::ServiceList supportedServices = account->services();
    for (int i = 0; i < supportedServices.size(); ++i) {
        const Accounts::Service &currService(supportedServices.at(i));
        supportedServiceNames.append(currService.name());
    }
    emit q->supportedServiceNamesChanged();

    // enabled
    if (enabledPendingInit) {
        pendingInitModifications = true;
    } else if (enabled != account->enabled()) {
        enabled = account->enabled();
        emit q->enabledChanged();
    }

    // display name
    if (displayNamePendingInit) {
        pendingInitModifications = true;
    } else if (displayName != account->displayName()) {
        displayName = account->displayName();
        emit q->displayNameChanged();
    }

    // configuration values
    if (configurationValuesPendingInit) {
        pendingInitModifications = true;
    } else {
        // enumerate the global configuration values
        account->selectService(Accounts::Service());
        QVariantMap gsValues;
        QStringList gsKeys = account->allKeys();
        foreach (const QString &key, gsKeys) {
            gsValues.insert(key, account->value(key, QVariant(), 0));
        }

        // also enumerate configuration values for all supported services.
        for (int i = 0; i < supportedServices.size(); ++i) {
            const Accounts::Service &currService(supportedServices.at(i));
            account->selectService(currService);
            QVariantMap serviceValues;
            QStringList serviceKeys = account->allKeys();
            foreach (const QString &key, serviceKeys) {
                serviceValues.insert(key, account->value(key, QVariant(), 0));
            }
            QVariantMap existingServiceValues = serviceConfigurationValues.value(currService.name());
            if (serviceValues != existingServiceValues) {
                serviceConfigurationValues.insert(currService.name(), serviceValues);
            }
        }
        account->selectService(Accounts::Service());

        // populate the "global service" configuration values
        if (configurationValues != gsValues) {
            configurationValues = gsValues;
        }
    }

    // do sync if required.
    if (status == Account::Invalid || status == Account::Error) {
        // error occurred during initialization, or was removed.
        // do nothing - the client will have already been notified.
    } else {
        // initialization completed successfully.
        setStatus(Account::Initialized);
        if (pendingInitModifications) {
            setStatus(Account::Modified); // modifications occurred prior to initialization completion.
        }
        if (pendingSync) {
            pendingSync = false;
            q->sync(); // the user requested sync() while we were initializing.
        }
    }
}

void AccountPrivate::enabledHandler(const QString &serviceName, bool newEnabled)
{
    // check to see if it's the "global" service name (generated by libaccounts-qt)
    // or an actual service name
    if (serviceName.isEmpty() || serviceName == QString(QLatin1String("global"))) {
        if (!enabledPendingInit && newEnabled != enabled) {
            enabled = newEnabled;
            emit q->enabledChanged();
        }
    }
}

void AccountPrivate::displayNameChangedHandler()
{
    if (displayNamePendingInit)
        return; // ignore, we have local changes which will overwrite this.

    if (displayName != account->displayName()) {
        displayName = account->displayName();
        emit q->displayNameChanged();
    }
}

void AccountPrivate::invalidate()
{
    // NOTE: the Accounts::Manager instance ALWAYS owns the account pointer.
    // If the manager gets deleted while the Account instance is
    // alive, we need to ensure that invalidate() gets called also.
    // We also invalidate the interface if the account itself gets removed
    // from the accounts database.
    if (account) {
        disconnect(account);
    }
    account = 0;
    setStatus(Account::Invalid);
}

void AccountPrivate::handleSynced()
{
    if (status == Account::SyncInProgress || status == Account::SigningIn) {
        if (!account) {
            qWarning() << Q_FUNC_INFO << "Account not valid";
            return;
        }

        // check to see if the id was updated
        int newIdentifier = account->id();
        if (identifier != newIdentifier) {
            identifier = account->id();
            emit q->identifierChanged();
        }

        // check to see if the providerName was updated
        if (providerName != account->providerName()) {
            providerName = account->providerName();
            emit q->providerNameChanged();
        }

        // check to see if the configuration values were updated
        QVariantMap allValues;
        QStringList allKeys = account->allKeys();
        foreach (const QString &key, allKeys) {
            allValues.insert(key, account->value(key, QVariant(), 0));
        }
        if (configurationValues != allValues) {
            configurationValues = allValues;
        }

        // and update our status.
        if (signInCredentials.creatingSignInCredentials) {
            if (signInCredentials.storingEncryptedTokens) {
                // "password" method - we must decrypt the (cached) encrypted response data.
                bool success = true;
                QVariantMap responseData = plainTextResponseData(signInCredentials.method,
                                                                 signInCredentials.responseData,
                                                                 signInCredentials.symmetricKey,
                                                                 signInCredentials.applicationName,
                                                                 &success);
                if (success) {
                    signInCredentials.cleanup();
                    setStatus(Account::Synced);
                    emit q->signInCredentialsCreated(responseData);
                } else {
                    signInCredentials.cleanup(true); // and remove identity
                    //: Error emitted if unable to decrypt the stored encrypted credentials
                    //% "Unable to decrypt stored credentials - aborting credentials creation"
                    emit q->signInError(qtTrId("sailfish_accounts-account-decryption_failed"));
                    setStatus(Account::Synced);
                }
            } else {
                // "oauth2" method - we just emit the cached response data.
                QVariantMap responseData = signInCredentials.responseData;
                signInCredentials.cleanup();
                setStatus(Account::Synced);
                emit q->signInCredentialsCreated(responseData);
            }
        } else {
            setStatus(Account::Synced);
        }
    }
}

void AccountPrivate::handleCredentialsStored(quint32 id)
{
    if (signInCredentials.identity->id() != id) {
        signInCredentials.cleanup(true);
        //: Error emitted if an error occurred while storing credentials
        //% "Unable to store credentials - invalid id"
        emit q->signInError(qtTrId("sailfish_accounts-account-identity_id_failed"));
        setStatus(Account::Synced);
        return;
    }

    signInCredentials.session = signInCredentials.identity->createSession(signInCredentials.method);
    if (!signInCredentials.session) {
        signInCredentials.cleanup(true);
        //: Error emitted if an error occurred while creating a signon session
        //% "Unable to create signon session"
        emit q->signInError(qtTrId("sailfish_accounts-account-session_create_failed"));
        setStatus(Account::Synced);
        return;
    }

    connect(signInCredentials.session, SIGNAL(response(SignOn::SessionData)), this, SLOT(handleResponse(SignOn::SessionData)));
    connect(signInCredentials.session, SIGNAL(error(SignOn::Error)), this, SLOT(handleSignOnError(SignOn::Error)));

    signInCredentials.session->process(SignOn::SessionData(signInCredentials.sessionData), signInCredentials.mechanism);
}

void AccountPrivate::handleResponse(const SignOn::SessionData &data)
{
    if (signInCredentials.creatingSignInCredentials) {
        // first, cache the response data, so that we can emit it after account->sync() finishes.
        signInCredentials.responseData.clear();
        QStringList keys = data.propertyNames();
        foreach (const QString &key, keys) {
            signInCredentials.responseData.insert(key, data.getProperty(key));
        }

        // then update the account with the credentials information.
        QString credName = signInCredentials.credentialsName.isEmpty() ? QLatin1String("default") : signInCredentials.credentialsName;
        QString configurationValueKey = BUILD_CREDENTIALS_CONFIGURATION_KEY(signInCredentials.applicationName, credName);
        account->selectService(Accounts::Service());
        account->setValue(configurationValueKey, signInCredentials.identity->id());

        // and write the changes to the accounts database
        connect(account, SIGNAL(error(Accounts::Error)), this, SLOT(handleAccountError()), Qt::UniqueConnection);
        /* have already connected account->synced() to handleSynced() */
        account->sync();
    } else if (signInCredentials.signingInWithCredentials) {
        // if it is "password" method, then the username/password are encrypted, and we need to decrypt.
        // if it is "oauth" (oauth1.0a / oauth2) then we just emit the tokens immediately,
        // as the security is provided by signond (and the fact that the client needs to know the clientid).
        if (signInCredentials.method.startsWith(QLatin1String("oauth"))) {
            QVariantMap responseData = signInCredentials.responseData;
            signInCredentials.cleanup();
            emit q->signInResponse(responseData);
        } else {
            // decrypt the (encrypted) response data, and emit
            bool success = true;
            QVariantMap responseData = plainTextResponseData(signInCredentials.method,
                                                             signInCredentials.responseData,
                                                             signInCredentials.symmetricKey,
                                                             signInCredentials.applicationName,
                                                             &success);
            signInCredentials.cleanup();
            if (success) {
                emit q->signInResponse(responseData);
            } else {
                //: Error emitted if unable to decrypt the stored encrypted credentials
                //% "Unable to decrypt stored credentials - aborting credentials creation"
                emit q->signInError(qtTrId("sailfish_accounts-account-decryption_failed"));
            }
        }
    }
}

void AccountPrivate::handleCredentialsFailed(const SignOn::Error &err)
{
    if (signInCredentials.creatingSignInCredentials) {
        QString providerName = account->providerName();
        signInCredentials.cleanup(true);
        //: Error emitted if account credentials creation failed at the database level
        //% "Unable to save credentials for %1 account in database: %2"
        emit q->signInError(qtTrId("sailfish_accounts-account-credentials_database_failed").arg(providerName).arg(err.message()));
        setStatus(Account::Synced);
    }
}

void AccountPrivate::handleSignOnError(const SignOn::Error &err)
{
    if (signInCredentials.creatingSignInCredentials) {
        signInCredentials.cleanup(true);
        QString errMess = err.message();
        if (errMess == QLatin1String("userActionFinished error: 5")) {
            //: Error emitted if signon failed due to network connection failure
            //% "Network connection failure"
            emit q->signInError(qtTrId("sailfish_accounts-account-network_failed"));
        } else {
            emit q->signInError(errMess);
        }
        setStatus(Account::Synced);
    }
}

void AccountPrivate::handleAccountError()
{
    if (signInCredentials.creatingSignInCredentials) {
        QString providerName = account->providerName();
        signInCredentials.cleanup(true);
        //: Error emitted if account creation failed at the database level
        //% "Unable to save %1 account in database"
        emit q->signInError(qtTrId("sailfish_accounts-account-account_database_failed").arg(providerName));
        setStatus(Account::Synced);
    } else {
        qWarning() << Q_FUNC_INFO << "sync failed or other error occurred"; // XXX TODO: properly manage this (eg, emit error)
    }
}

QVariantMap AccountPrivate::plainTextResponseData(const QString &method, const QVariantMap &encryptedResponseData, const QString &symmetricKey, const QString &applicationName, bool *succeeded) const
{
    QVariantMap retn;
    *succeeded = true;
    if (method.toLower() == QLatin1String("password")) {
        // Username and Password will be encrypted.
        foreach (const QString &key, encryptedResponseData.keys()) {
            if (key.toLower() == QLatin1String("password") || key.toLower() == QLatin1String("secret")) {
                QString decryptedPassword = decrypted_string_b64(encryptedResponseData.value(key).toString(), symmetricKey);
                if (decryptedPassword.endsWith(applicationName)) {
                    decryptedPassword.chop(applicationName.length());
                    retn.insert(key, decryptedPassword);
                } else {
                    // else, they supplied the wrong decryption key.
                    *succeeded = false;
                }
            } else if (key.toLower() == QLatin1String("username")) {
                QString decryptedUsername = decrypted_string_b64(encryptedResponseData.value(key).toString(), symmetricKey);
                if (decryptedUsername.endsWith(applicationName)) {
                    decryptedUsername.chop(applicationName.length());
                    retn.insert(key, decryptedUsername);
                } else {
                    // else, they supplied the wrong decryption key.
                    *succeeded = false;
                }
            } else {
                qWarning() << Q_FUNC_INFO << "unknown password key:" << key << "=" << encryptedResponseData.value(key).toString();
                retn.insert(key, encryptedResponseData.value(key));
            }
        }
    } else if (method.toLower() == QLatin1String("oauth2")) {
        // encryption is handled by signond.  The response data should not be encrypted.
        retn = encryptedResponseData;
    } else {
        qWarning() << Q_FUNC_INFO << "unknown method:" << method;
        retn = encryptedResponseData;
    }

    return retn;
}

void AccountPrivate::setStatus(Account::Status newStatus)
{
    if (status == Account::Invalid) {
        return; // once invalid, cannot be restored.
    }

    if (status != newStatus) {
        status = newStatus;
        emit q->statusChanged();
    }
}

//-----------------------------------

/*!
    \qmltype Account
    \instantiates Account
    \inqmlmodule Sailfish.Accounts 1
    \brief Allows clients to modify or sign into an account with a service provider

    The Account type is a non-visual type which allows
    the details of an account to be specified, and saved to the system
    accounts database.

    Any modifications to any property of an account will have no effect
    until the modifications are saved to the database by calling sync().

    \qml
        import Sailfish.Accounts 1.0

        Item {
            id: root

            Account {
                id: account
                identifier: 12 // retrieved from AccountManager or AccountModel

                // we will be updating the following two properties
                displayName: "inactive example account"
                enabled: false

                onStatusChanged: {
                    if (status == Account.Initialized) {
                        sync() // trigger database write
                    } else if (status == Account.Error) {
                        // handle error
                    } else if (status == Account.Synced) {
                        // successfully written to database
                        // for example purposes, we may want to remove the account.
                        remove() // trigger database write
                    } else if (status == Account.Invalid) {
                        // successfully removed from database.
                    }
                }
            }
        }
    \endqml

    An Account can also be used to sign into a service.

    Each application must create signon credentials in the account,
    and may sign into the account using those credentials.  If the
    service uses OAuth1.0a or OAuth2 for authentication, the client
    must pass a valid ConsumerKey or ClientId in the parameters to
    the authentication request.

    The following example shows how per-application credentials can
    be added to an existing account for an OAuth2 service:

    \qml
        import Sailfish.Accounts 1.0

        Account {
            id: account
            identifier: 12 // example: Facebook account id retrieved from AccountManager or AccountModel

            onStatusChanged: {
                if (status == Account.Initialized) {
                    var siParams = signInParameters("facebook-sharing")
                    siParams.setParameter("ClientId", "123456789abcdef")
                    if (!hasSignInCredentials("MyApp", "MyCredentials")) {
                        createSignInCredentials("MyApp", "SharingCredentials", siParams)
                    } else {
                        signIn("MyApp", "SharingCredentials", siParams)
                    }
                }
            }

            onSignInCredentialsCreated: {
                for (var i in data) console.log(i+"="+data[i]) // will contain AccessToken
            }

            onSignInResponse: {
                for (var i in data) console.log(i+"="+data[i]) // will contain AccessToken
            }
        }
    \endqml

    If the authentication method is password based, the client must pass in
    a symmetric key which is used to encrypt and decrypt the credentials.

    \qml
        import Sailfish.Accounts 1.0

        Account {
            id: account
            identifier: 13 // example: Jabber account id retrieved from AccountManager or AccountModel

            onStatusChanged: {
                if (status == Account.Initialized) {
                    var siParams = signInParameters("jabber-im")
                    if (!hasSignInCredentials("MyApp", "MyCredentials")) {
                        createSignInCredentials("MyApp", "JabberCredentials", "MySecretKey", siParams)
                    } else {
                        signIn("MyApp", "JabberCredentials", "MySecretKey", siParams)
                    }
                }
            }

            onSignInCredentialsCreated: {
                for (var i in data) console.log(i+"="+data[i]) // will contain Username + Password
            }

            onSignInResponse: {
                for (var i in data) console.log(i+"="+data[i]) // will contain Username + Password
            }
        }
    \endqml

    To create an account, use the AccountManager type:

    \qml
        import Sailfish.Accounts 1.0

        QtObject {
            id: root

            AccountManager {
                id: manager
                Component.onCompleted: manager.createAccount("facebook")
                onAccountCreated: account.identifier = accountId
            }

            Account {
                id: account

                onStatusChanged: {
                    if (status == Account.Initialized) {
                        console.log("Successfully created account")
                        var siParams = signInParameters("facebook-sharing")
                        siParams.setParameter("ClientId", "123456789abcdef")
                        createSignInCredentials("MyApp", "SharingCredentials", siParams)
                    }
                }

                onSignInCredentialsCreated: {
                    console.log("Successfully created credentials")
                    for (var i in data) console.log(i+"="+data[i]);
                }
            }
        }
    \endqml
*/

Account::Account(QObject *parent)
    : QObject(parent), d(new AccountPrivate(this, 0))
{
}

Account::~Account()
{
}


// QQmlParserStatus
void Account::classBegin() { }
void Account::componentComplete()
{
    if (!d->account) {
        if (d->identifier == 0) {
            d->setStatus(Account::Invalid); // Set to invalid even though already set to error!  Since no account.
        } else {
            // loading an existing account
            Accounts::Account *existingAccount = d->manager->account(d->identifier);
            d->setAccount(existingAccount, true);
        }
    } else {
        // account was provided by AccountFactory.
        // do nothing.
    }
}

// helpers for AccountFactory only.
Account::Account(bool queryInfoOnCreation, Accounts::Account *account, QObject *parent)
    : QObject(parent), d(new AccountPrivate(this, account, queryInfoOnCreation)) { }
Accounts::Account *Account::account() { return d->account; }

/*!
    \qmlmethod void Account::sync()

    Writes any outstanding local modifications to the database.
    The operation may be either synchronous or asynchronous
    depending on whether the database is currently locked or
    open for writing.  The account will transition to the
    \c{SyncInProgress} status and remain with that status for
    the duration of the synchronisation operation.

    Calling this function will have no effect if the account is
    invalid or if a previous synchronisation operation is in
    progress.
*/
void Account::sync()
{
    if (d->status == Account::Initializing) {
        d->pendingSync = true;
    }

    if (d->status == Account::Invalid
            || d->status == Account::SyncInProgress
            || d->status == Account::Initializing) {
        return;
    }

    if (!d->account) { // initialization failed.
        d->error = Account::InitializationFailedError;
        emit errorChanged();
        d->setStatus(Account::Invalid);
        return;
    }

    if (d->pendingInitModifications) {
        // we have handled them by directly syncing.
        // after this sync, we will once again allow
        // change signals to cause modifications to the properties.
        d->pendingInitModifications = false;
        d->identifierPendingInit = false;
        d->enabledPendingInit = false;
        d->displayNamePendingInit = false;
        d->configurationValuesPendingInit = false;
    }

    // set the global configuration values.
    d->account->selectService(Accounts::Service());
    QStringList allKeys = d->account->allKeys();
    QStringList setKeys = d->configurationValues.keys();
    QStringList doneKeys;
    foreach (const QString &key, allKeys) {
        // overwrite existing keys
        if (setKeys.contains(key)) {
            doneKeys.append(key);
            const QVariant &currValue = d->configurationValues.value(key);
            if (currValue.isValid()) {
                d->account->setValue(key, currValue);
            } else {
                d->account->remove(key);
            }
        } else {
            // remove removed keys
            d->account->remove(key);
        }
    }
    foreach (const QString &key, setKeys) {
        // add new keys
        if (!doneKeys.contains(key)) {
            const QVariant &currValue = d->configurationValues.value(key);
            d->account->setValue(key, currValue);
        }
    }

    // and the service-specific configuration values and service-enabledness status
    foreach (const QString &srvn, d->supportedServiceNames) {
        Accounts::Service srv = d->manager->service(srvn);
        if (srv.isValid()) {
            d->account->selectService(srv);

            // first, configuration values:
            QVariantMap setSrvValues = d->serviceConfigurationValues.value(srvn);
            QStringList setSrvKeys = setSrvValues.keys();
            QStringList srvKeys = d->account->allKeys();
            QStringList doneSrvKeys;
            foreach (const QString &key, srvKeys) {
                // overwrite existing keys
                if (setSrvKeys.contains(key)) {
                    doneSrvKeys.append(key);
                    const QVariant &currValue = setSrvValues.value(key);
                    if (currValue.isValid()) {
                        d->account->setValue(key, currValue);
                    } else {
                        d->account->remove(key);
                    }
                } else {
                    // remove removed keys
                    d->account->remove(key);
                }
            }
            foreach (const QString &key, setSrvKeys) {
                // add new keys
                if (!doneSrvKeys.contains(key)) {
                    const QVariant &currValue = setSrvValues.value(key);
                    d->account->setValue(key, currValue);
                }
            }
        }
    }

    // enable or disable the global service
    d->account->selectService(Accounts::Service());
    d->account->setEnabled(d->enabled);

    // set the display name
    d->account->setDisplayName(d->displayName);

    // and write to database.
    d->setStatus(Account::SyncInProgress);
    d->account->sync();
}

/*!
    \qmlmethod void Account::remove()

    Removes the account.  A removed account becomes invalid.
    Any credentials associated with the account are also removed.
*/
void Account::remove()
{
    if (!d->account) {
        return;
    }

    // remove associated credentials.
    d->account->selectService(Accounts::Service());
    QStringList configurationKeys = d->account->allKeys();
    foreach (const QString &key, configurationKeys) {
        if (key.contains(CREDENTIALS_GROUP)) {
            int identityId = d->account->valueAsInt(key, 0);
            if (identityId) {
                SignOn::Identity *doomedIdentity = SignOn::Identity::existingIdentity(identityId);
                if (doomedIdentity) {
                    doomedIdentity->signOut();
                    doomedIdentity->remove();
                }
            }
        }
    }

    d->setStatus(Account::SyncInProgress);
    d->account->remove();
    d->account->sync();
}


/*!
    \qmlmethod QVariantMap Account::configurationValues(const QString &serviceName)

    Returns the configuration settings for the account which apply
    specifically to the service with the specified \a serviceName.
    Note that it won't include global configuration settings which
    may also be applied (as fallback settings) when the account is
    used with the service.

    Some default settings are usually specified in the \c{.service}
    file installed by the account provider plugin.  Other settings
    may be specified directly on an account for the service.

    If the specified \a serviceName is empty, the account's global
    configuration settings will be returned instead.
    Note: this does not return all configuration settings of all
    services; it only returns configuration settings which are
    globally applicable to all services for the account.
*/
QVariantMap Account::configurationValues(const QString &serviceName) const
{
    if (d->status == Account::Invalid) {
        return QVariantMap();
    }

    if (serviceName.isEmpty()) {
        return d->configurationValues;
    }

    return d->serviceConfigurationValues.value(serviceName);
}


/*!
    \qmlmethod void Account::setConfigurationValues(const QString &serviceName, const QVariantMap &values)

    Sets the configuration settings for the account which apply
    specifically to the service with the specified \a serviceName.

    The \a serviceName must identify a service supported by the
    account, or be empty, else calling this function will have no effect.
    If the \a serviceName is empty, the global account configuration
    settings will updated instead.
*/
void Account::setConfigurationValues(const QString &serviceName, const QVariantMap &values)
{
    if (d->status == Account::Invalid || d->status == Account::SyncInProgress) {
        return;
    }

    if (!serviceName.isEmpty() && !supportedServiceNames().contains(serviceName)) {
        return;
    }

    QVariantMap validValues;
    QStringList vkeys = values.keys();
    foreach (const QString &key, vkeys) {
        QVariant currValue = values.value(key);
        if (currValue.type() == QVariant::Bool
                || currValue.type() == QVariant::Int
                || currValue.type() == QVariant::LongLong
                || currValue.type() == QVariant::ULongLong
                || currValue.type() == QVariant::String
                || currValue.type() == QVariant::StringList) {
            validValues.insert(key, currValue);
        } else if (currValue.type() == QVariant::List) {
            validValues.insert(key, currValue.toStringList());
        }
    }

    // NOTE: we deliberately don't do change detection, because
    // we don't connect to valuesChanged() and update our internal
    // maps when someone else changes an account setting.

    bool globalService = serviceName.isEmpty();
    if (globalService) {
        d->configurationValues = validValues;
        if (d->status == Account::Initializing) {
            d->configurationValuesPendingInit = true;
        } else {
            d->setStatus(Account::Modified);
        }
    } else {
        d->serviceConfigurationValues.insert(serviceName, validValues);
        if (d->status == Account::Initializing) {
            d->configurationValuesPendingInit = true;
        } else {
            d->setStatus(Account::Modified);
        }
    }
}

/*!
    \qmlmethod void Account::setConfigurationValue(const QString &serviceName, const QString &key, const QVariant &value)

    Sets the configuration setting named \a key for the account which applies
    specifically to the service with the specified \a serviceName to the
    given \a value.

    The \a serviceName must identify a service supported by the
    account, or be empty, else calling this function will have no effect.
    If the \a serviceName is empty, the global account configuration
    settings will updated instead.
*/
void Account::setConfigurationValue(const QString &serviceName, const QString &key, const QVariant &value)
{
    QVariantMap configValues = configurationValues(serviceName);
    configValues.insert(key, value);
    setConfigurationValues(serviceName, configValues);
}

/*!
    \qmlmethod void Account::removeConfigurationValue(const QString &serviceName, const QString &key)

    Removes the configuration setting named \a key for the account which applies
    specifically to the service with the specified \a serviceName.

    The \a serviceName must identify a service supported by the
    account, or be empty, else calling this function will have no effect.
    If the \a serviceName is empty, the global account configuration
    settings will updated instead.
*/
void Account::removeConfigurationValue(const QString &serviceName, const QString &key)
{
    QVariantMap configValues = configurationValues(serviceName);
    configValues.remove(key);
    setConfigurationValues(serviceName, configValues);
}

/*!
    \qmlmethod bool Account::isEnabledWithService(const QString &serviceName)

    Returns true if the account is enabled with the specified service.
    Returns false if the account is not enabled with the specified service,
    or if the specified service does not exist, or is not supported by the
    account.
*/
bool Account::isEnabledWithService(const QString &serviceName)
{
    bool retn = false;

    if (d->supportedServiceNames.contains(serviceName)) {
        Accounts::Service srv = d->manager->service(serviceName);
        if (srv.isValid()) {
            d->account->selectService(srv);
            retn = d->account->enabled();
            d->account->selectService(Accounts::Service());
        }
    }

    return retn;
}

/*!
    \qmlmethod void Account::enableWithService(const QString &serviceName)

    Enables the account with the service identified by the given \a serviceName.

    If the service does not exist, or this account does not support the service,
    or the status of the account is either Invalid or SyncInProgress, the operation
    will silently fail.

    Note: after enabling the account with a service, you must call \c sync() in
    order to write the change to the database.
*/
void Account::enableWithService(const QString &serviceName)
{
    if (d->status == Account::Invalid || d->status == Account::SyncInProgress) {
        return;
    }

    if (d->supportedServiceNames.contains(serviceName)) {
        Accounts::Service srv = d->manager->service(serviceName);
        if (srv.isValid()) {
            d->account->selectService(srv);
            if (!d->account->enabled()) {
                d->account->setEnabled(true);
                d->account->selectService(Accounts::Service());
                if (d->status == Account::Initializing) {
                    d->enabledPendingInit = true;
                } else {
                    d->setStatus(Account::Modified);
                }
            } else {
                d->account->selectService(Accounts::Service());
            }
        }
    }
}


/*!
    \qmlmethod void Account::disableWithService(const QString &serviceName)

    Disables the account with the service identified by the given \a serviceName.

    If the service does not exist, or this account does not support the service,
    or the status of the account is either Invalid or SyncInProgress, the operation
    will silently fail.

    Note: this method will have no effect until sync() is called!
*/
void Account::disableWithService(const QString &serviceName)
{
    if (d->status == Account::Invalid || d->status == Account::SyncInProgress)
        return;

    if (d->supportedServiceNames.contains(serviceName)) {
        Accounts::Service srv = d->manager->service(serviceName);
        if (srv.isValid()) {
            d->account->selectService(srv);
            if (d->account->enabled()) {
                d->account->setEnabled(false);
                d->account->selectService(Accounts::Service());
                if (d->status == Account::Initializing) {
                    d->enabledPendingInit = true;
                } else {
                    d->setStatus(Account::Modified);
                }
            } else {
                d->account->selectService(Accounts::Service());
            }
        }
    }
}


/*!
    \qmlmethod SignInParameters *Account::signInParameters(const QString &serviceName, const QString &username = QString(), const QString &password = QString())

    Returns the \l SignInParameters which can be used to sign into the service
    identified by the given \a serviceName.  This includes the sign-in
    method, mechanism and session parameters which are applicable to the
    service.  The parameters can be augmented with more parameters if the
    client wishes (however the client must know which parameters are valid).

    If the \a serviceName does not identify a valid service, the sign-in
    parameters object will be empty.  The \a username and \a password
    arguments should only be specified if the application intends to create
    new credentials, and only if the service uses password-based
    authentication (not OAuth-based).

    The returned object is owned by this account and will be cleaned up
    when the account wrapper object is destroyed.
*/
SignInParameters *Account::signInParameters(const QString &serviceName, const QString &username, const QString &password)
{
    // XXX TODO: patch accounts&sso so that Service provides accessors
    // for method/mechanism/parameters from <template>
    QString method;
    QString mechanism;
    QVariantMap parameters;

    // Note: we don't use service-segregation, but instead we use per-application segregation.
    // So, we use the ServiceAccount's AuthData only to get the method/mechanism/params.
    Accounts::Service srv = d->manager->service(serviceName);
    if (srv.isValid()) {
        Accounts::AccountService as(d->account, srv);
        Accounts::AuthData authData(as.authData());
        method = authData.method();
        mechanism = authData.mechanism();
        parameters = authData.parameters();
    } else {
        qWarning() << Q_FUNC_INFO << "No such service:" << serviceName;
    }

    return new SignInParameters(method, mechanism, parameters, username, password, this);
}

/*!
    \qmlmethod Account::hasSignInCredentials(const QString &applicationName, const QString &credentialsName) const

    Returns true if the application named \a applicationName has created
    sign-in credentials with this account named \a credentialsName.  If
    \a credentialsName is empty, the function returns true if the "default"
    named credentials have been created by the application.

    Returns false if no credentials with the given \a credentialsName exist
    for the application.  This function will also return false if the account
    is not in either the \c Initialized or \c Synced state.
*/
bool Account::hasSignInCredentials(const QString &applicationName,
                                    const QString &credentialsName) const
{
    if (d->status != Account::Initialized && d->status != Account::Synced) {
        return false;
    }

    QString credName = credentialsName.isEmpty() ? QLatin1String("default") : credentialsName;
    QString configurationValueKey = BUILD_CREDENTIALS_CONFIGURATION_KEY(applicationName, credName);
    return d->configurationValues.value(configurationValueKey, QVariant::fromValue<int>(0)).toInt() != 0;
}

/*!
    \qmlmethod Account::createSignInCredentials(const QString &applicationName, const QString &credentialsName, SignInParameters *parameters, const QString &symmetricKey = QString())

    Creates sign-in credentials with this account for the application with
    the given \a applicationName named \a credentialsName (or named "default"
    if the \a credentialsName parameter is empty).

    The account will transition to the \c SigningIn state when creation of
    the credentials commences.

    If the \a parameters specify the OAuth method of authentication, sign-in
    will occur as part of the creation of credentials (and the user will be
    prompted for authorization via a web-view).  The result of this process,
    if successful, will include an AccessToken.  The \a symmetricKey
    parameter will be ignored for OAuth authentication.

    If the \a parameters specify a password-based method of authentication,
    sign-in will not occur but instead the username and password specified
    in the \a parameters will be encrypted with the given \a symmetricKey
    and stored in the credentials.  If the \a symmetricKey parameter is
    empty, the credentials will not be encrypted - which means that any
    application will be able to use those credentials with the service.

    Once creation of credentials completes successfully the
    \c signInCredentialsCreated() signal will be emitted.  If creation of the
    credentials encounters an error then the \c signInError() signal will
    be emitted.

    Finally, the account will then transition to the \c Synced state.

    Calling this function will have no effect if the account is not
    initially in either the \c Initialized or \c Synced state.
*/
void Account::createSignInCredentials(const QString &applicationName,
                                      const QString &credentialsName,
                                      SignInParameters *parameters,
                                      const QString &symmetricKey)
{
    if (d->status != Account::Initialized && d->status != Account::Synced) {
        //: Error emitted if function called while account is in invalid state
        //% "Account status is not Initialized or Synced"
        emit signInError(qtTrId("sailfish_accounts-account-unpw_invalid_status"));
        return;
    }

    if (parameters == NULL) {
        //: Error emitted if function called with invalid parameters
        //% "Invalid sign-in parameters specified"
        emit signInError(qtTrId("sailfish_accounts-account-unpw_invalid_params"));
        return;
    }

    if (applicationName.isEmpty()) {
        //: Error emitted if function called with invalid application name
        //% "Invalid application name specified"
        emit signInError(qtTrId("sailfish_accounts-account-unpw_invalid_appname"));
        return;
    }

    // For non-oauth2 signon:
    // step one: check if the credentials already exist
    // step two: create identity
    // step three: append application name to username/password
    // step four: encrypt username/password
    // step five: base64 encode the result
    // step six: store back into identity.
    // step seven: save identity id into account config settings for the application.
    // step eight: emit success including plain text credentials.

    // For oauth2 signon:
    // step one: check if the credentials already exist
    // step two: create identity
    // step three: perform signon
    // step four: emit success including plain text tokens returned from signond

    if (hasSignInCredentials(applicationName, credentialsName)) {
        //: Error emitted if signon credentials already exist
        //% "Named credentials already exist for this application"
        emit signInError(qtTrId("sailfish_accounts-account-unpw_already_exist"));
        return;
    }

    if (parameters->method().toLower().startsWith(QLatin1String("oauth"))) {
        // oauth-based authentication.  trigger sign-on process.
        QMap<QString, QStringList> methodMechanisms;
        methodMechanisms.insert(parameters->method(), QStringList(parameters->mechanism()));
        d->signInCredentials.identityInfo = SignOn::IdentityInfo(applicationName, QString(), methodMechanisms);
        d->signInCredentials.identity = SignOn::Identity::newIdentity(d->signInCredentials.identityInfo);
        if (d->signInCredentials.identity == NULL) {
            //: Error emitted if identity creation fails
            //% "Failed to create credentials"
            emit signInError(qtTrId("sailfish_accounts-account-oauth_identity_failed"));
            return;
        }

        d->signInCredentials.applicationName = applicationName;
        d->signInCredentials.symmetricKey = symmetricKey;
        d->signInCredentials.method = parameters->method();
        d->signInCredentials.mechanism = parameters->mechanism();
        d->signInCredentials.sessionData = parameters->parameters();
        d->signInCredentials.credentialsName = credentialsName;
        d->signInCredentials.username = QString();
        d->signInCredentials.password = QString();
        d->signInCredentials.creatingSignInCredentials = true;
        d->signInCredentials.storingEncryptedTokens = false; // we never attempt to store encrypted tokens for OAuth2, signond handles that.

        connect(d->signInCredentials.identity, SIGNAL(error(SignOn::Error)), d, SLOT(handleCredentialsFailed(SignOn::Error)));
        connect(d->signInCredentials.identity, SIGNAL(credentialsStored(quint32)), d, SLOT(handleCredentialsStored(quint32)));

        d->setStatus(Account::SigningIn);
        d->signInCredentials.identity->storeCredentials(d->signInCredentials.identityInfo);
    } else {
        // password-based authentication.  encrypt and store the credentials directly.
        QString usernameWithAppName = parameters->username() + applicationName;
        QString encryptedUserName = b64_encrypted_string(usernameWithAppName, symmetricKey);
        if (encryptedUserName.isNull()) {
            //: Error emitted if encrypting username fails
            //% "Error occurred while encrypting username"
            emit signInError(qtTrId("sailfish_accounts-account-uname_encryption_failed"));
            return;
        }

        QString secretWithAppName = parameters->password() + applicationName;
        QString encryptedSecret = b64_encrypted_string(secretWithAppName, symmetricKey);
        if (encryptedSecret.isNull()) {
            //: Error emitted if encrypting password fails
            //% "Error occurred while encrypting password"
            emit signInError(qtTrId("sailfish_accounts-account-pword_encryption_failed"));
            return;
        }

        QMap<QString, QStringList> methodMechanisms;
        methodMechanisms.insert(parameters->method(), QStringList(parameters->mechanism()));
        d->signInCredentials.identityInfo = SignOn::IdentityInfo(applicationName, encryptedUserName, methodMechanisms);
        d->signInCredentials.identityInfo.setSecret(encryptedSecret);
        d->signInCredentials.identity = SignOn::Identity::newIdentity(d->signInCredentials.identityInfo);
        if (d->signInCredentials.identity == NULL) {
            //: Error emitted if identity creation fails
            //% "Failed to create credentials"
            emit signInError(qtTrId("sailfish_accounts-account-unpw_identity_failed"));
            return;
        }

        d->signInCredentials.applicationName = applicationName;
        d->signInCredentials.symmetricKey = symmetricKey;
        d->signInCredentials.method = parameters->method();
        d->signInCredentials.mechanism = parameters->mechanism();
        d->signInCredentials.sessionData = parameters->parameters();
        d->signInCredentials.credentialsName = credentialsName;
        d->signInCredentials.username = parameters->username();
        d->signInCredentials.password = parameters->password();
        d->signInCredentials.creatingSignInCredentials = true;
        d->signInCredentials.storingEncryptedTokens = true; // for password method, we store encrypted username/password immediately

        connect(d->signInCredentials.identity, SIGNAL(error(SignOn::Error)), d, SLOT(handleCredentialsFailed(SignOn::Error)));
        connect(d->signInCredentials.identity, SIGNAL(credentialsStored(quint32)), d, SLOT(handleCredentialsStored(quint32)));

        d->setStatus(Account::SigningIn);
        d->signInCredentials.identity->storeCredentials(d->signInCredentials.identityInfo);
    }
}

/*!
    \qmlmethod Account::removeSignInCredentials(const QString &applicationName, const QString &credentialsName)

    Removes the sign-in credentials for the application with the given
    \a applicationName from the account, where the credentials are
    named the given \a credentialsName (or named "default" if the
    \a credentialsName parameter is empty).

    Calling this function will have no effect if the account is not
    in either the \c Initialized or \c Synced state, if the application
    name specified is invalid, or if the account is currently busy
    creating or using sign in credentials.
*/
void Account::removeSignInCredentials(const QString &applicationName,
                                      const QString &credentialsName)
{
    if (d->status != Account::Initialized && d->status != Account::Synced)
        return;

    if (applicationName.isEmpty())
        return;

    if (!hasSignInCredentials(applicationName, credentialsName))
        return;

    if (d->signInCredentials.creatingSignInCredentials
            || d->signInCredentials.signingInWithCredentials) {
        return;
    }

    QString credName = credentialsName.isEmpty() ? QLatin1String("default") : credentialsName;
    QString configurationValueKey = BUILD_CREDENTIALS_CONFIGURATION_KEY(applicationName, credName);
    int identityId = d->configurationValues.value(configurationValueKey, QVariant::fromValue<int>(0)).toInt();

    SignOn::Identity *removeIdentity = identityId == 0 ? NULL : SignOn::Identity::existingIdentity(identityId);
    if (removeIdentity != NULL) {
        removeIdentity->signOut();
        removeIdentity->remove();
    }

    // remove the key from our local map.
    d->configurationValues.remove(configurationValueKey);

    // and from the account
    d->account->selectService(Accounts::Service());
    d->account->remove(configurationValueKey);
    d->setStatus(Account::SyncInProgress);
    d->account->sync();
}

/*!
    \qmlmethod Account::signIn(const QString &applicationName, const QString &credentialsName, SignInParameters *parameters, const QString &symmetricKey = QString())

    Signs the application with the given \a applicationName into the account
    using the per-application credentials identified by the given
    \a credentialsName.  The given \a parameters will be used during sign-in
    (although any username or password specified in those parameters will be
    ignored).

    If the sign-in process uses a password authentication system, the
    previously stored credentials (username and password) will be decrypted
    using the given \a symmetricKey, which means that if the key given is
    incorrect, sign-in will fail.

    If the sign-in process uses an OAuth-based authentication system, the
    \a symmetricKey argument will be ignored, and the tokens associated with
    the ClientId or ConsumerKey specified in the \a parameters will be
    read from the database.

    Emits \c signInResponseReceived() on success, or \c signInError() on
    failure.

    Calling this function will have no effect if the account is not
    in either the \c Initialized or \c Synced state.
*/
void Account::signIn(const QString &applicationName,
                     const QString &credentialsName,
                     SignInParameters *parameters,
                     const QString &symmetricKey)
{
    if (d->status != Account::Initialized && d->status != Account::Synced) {
        //: Error emitted if function called while account is in invalid state
        //% "Account status is not Initialized or Synced"
        emit signInError(qtTrId("sailfish_accounts-account-signin_invalid_status"));
        return;
    }

    if (parameters == NULL) {
        //: Error emitted if function called with invalid parameters
        //% "Invalid sign-in parameters specified"
        emit signInError(qtTrId("sailfish_accounts-account-unpw_invalid_params"));
        return;
    }

    if (applicationName.isEmpty()) {
        //: Error emitted if function called with invalid application name
        //% "Invalid application name specified"
        emit signInError(qtTrId("sailfish_accounts-account-signin_invalid_appname"));
        return;
    }

    if (!hasSignInCredentials(applicationName, credentialsName)) {
        //: Error emitted if signon credentials do not exist
        //% "Named credentials do not exist for this application"
        emit signInError(qtTrId("sailfish_accounts-account-signin_not_exist"));
        return;
    }

    QString credName = credentialsName.isEmpty() ? QLatin1String("default") : credentialsName;
    QString configurationValueKey = BUILD_CREDENTIALS_CONFIGURATION_KEY(applicationName, credName);
    int identityId = d->configurationValues.value(configurationValueKey, QVariant::fromValue<int>(0)).toInt();

    SignOn::Identity *signInIdentity = identityId == 0 ? NULL : SignOn::Identity::existingIdentity(identityId);
    if (signInIdentity == NULL) {
        //: Error emitted if signon credentials could not be loaded from the database
        //% "Credentials with id %1 could not be loaded"
        emit signInError(qtTrId("sailfish_accounts-account-load_credentials_error").arg(identityId));
        return;
    }

    d->signInCredentials.signingInWithCredentials = true;
    
    d->signInCredentials.method = parameters->method();
    d->signInCredentials.mechanism = parameters->mechanism();
    d->signInCredentials.sessionData = parameters->parameters();
    d->signInCredentials.applicationName = applicationName;
    d->signInCredentials.symmetricKey = symmetricKey;
    d->signInCredentials.credentialsName = credName;

    d->signInCredentials.identity = signInIdentity;
    d->signInCredentials.session = signInIdentity->createSession(parameters->method());
    if (d->signInCredentials.session == NULL) {
        d->signInCredentials.cleanup();
        //: Error emitted if an error occurred while creating a signon session
        //% "Unable to create signon session with the specified parameters"
        emit signInError(qtTrId("sailfish_accounts-account-session_create_failed"));
        return;
    }

    connect(d->signInCredentials.session, SIGNAL(response(SignOn::SessionData)), this, SLOT(handleResponse(SignOn::SessionData)));
    connect(d->signInCredentials.session, SIGNAL(error(SignOn::Error)), this, SLOT(handleSignOnError(SignOn::Error)));
    d->setStatus(Account::SigningIn);
    d->signInCredentials.session->process(SignOn::SessionData(parameters->parameters()), parameters->mechanism());
}

/*!
    \qmlmethod Account::signOut(const QString &applicationName, const QString &credentialsName)

    Signs the application out of the account where it had previously been
    signed in using the credentials named the given \a credentialsName
    (or named "default" if no \a credentialsName is given).

    Client code should not need to call this method, as the account can
    remain signed in safely.  Signing out will clear the cache of any
    tokens or credentials stored for the named credentials.

    Calling this function will have no effect if the account is not
    in either the \c Initialized or \c Synced state.
*/
void Account::signOut(const QString &applicationName,
                      const QString &credentialsName)
{
    if (d->status != Account::Initialized && d->status != Account::Synced)
        return;

    if (!hasSignInCredentials(applicationName, credentialsName))
        return;

    QString credName = credentialsName.isEmpty() ? QLatin1String("default") : credentialsName;
    QString configurationValueKey = BUILD_CREDENTIALS_CONFIGURATION_KEY(applicationName, credName);
    int identityId = d->configurationValues.value(configurationValueKey, QVariant::fromValue<int>(0)).toInt();

    SignOn::Identity *signInIdentity = identityId == 0 ? NULL : SignOn::Identity::existingIdentity(identityId);
    if (signInIdentity == NULL) {
        qWarning() << Q_FUNC_INFO << "credentials with id" << identityId << "could not be signed out";
        return;
    }

    signInIdentity->signOut();
    signInIdentity->deleteLater();
}



/*!
    \qmlproperty bool Account::enabled
    This property will be true if the account can be used, or false if it cannot.

    The account should be enabled if the details specified for it are valid.
    An account may need valid credentials associated with it before it can be
    enabled.
*/

bool Account::enabled() const
{
    if (d->status == Account::Invalid)
        return false;
    return d->enabled;
}

void Account::setEnabled(bool e)
{
    if (d->status == Account::Invalid || d->status == Account::SyncInProgress)
        return;

    d->enabled = e;
    if (d->status == Account::Initializing) {
        d->enabledPendingInit = true;
    } else {
        d->setStatus(Account::Modified);
    }
    emit enabledChanged();
}

/*!
    \qmlproperty int Account::identifier
    This property contains the identifier of the Account.

    The value of the property will be zero if the Account is a new, unsynced
    account.  If the Account has been saved in the system accounts database,
    it will be non-zero.

    When declaring an Account you must supply an identifier to cause
    the account to reference an account that already exists in the
    system accounts database - this identifier can be retrieved from the
    AccountManager.
*/

int Account::identifier() const
{
    if (d->status == Account::Invalid)
        return 0;
    return d->identifier;
}

void Account::setIdentifier(int id)
{
    if (d->status == Account::Initializing) {
        d->identifierPendingInit = true;
        d->identifier = id;
    } else if (id != d->identifier && d->status == Account::Invalid) {
        // the client is setting the account identifier after initialization.
        d->deleteLater();
        d = new AccountPrivate(this, 0);
        d->identifierPendingInit = true;
        d->identifier = id;
        emit statusChanged(); // manually emit - initializing.
        componentComplete();
    }
}

/*!
    \qmlproperty string Account::providerName

    This property contains the name of the service provider with which
    the account is valid.

    An account provider plugin will provide a \c{.provider} file in
    \c{/usr/share/accounts/providers} which specifies the name of the
    provider.
*/

QString Account::providerName() const
{
    if (d->status == Account::Invalid)
        return QString();
    return d->providerName;
}

/*!
    \qmlproperty string Account::displayName
    This property contains the display name of the account

    The display name is the name of the account which should be
    displayed to users in selection lists, edit dialogues, and
    other user-interface contexts.
*/

QString Account::displayName() const
{
    if (d->status == Account::Invalid)
        return QString();
    return d->displayName;
}

void Account::setDisplayName(const QString &dn)
{
    if (d->status == Account::Invalid || d->status == Account::SyncInProgress)
        return;

    d->displayName = dn;
    if (d->status == Account::Initializing)
        d->displayNamePendingInit = true;
    else
        d->setStatus(Account::Modified);
    emit displayNameChanged();
}

/*!
    \qmlproperty QStringList Account::supportedServiceNames
    This property contains the names of services supported by the account

    Every service provided by a provider has a service name which is
    specified in the \c{.service} file located at
    \c{/usr/share/accounts/services} which is installed by the account
    provider plugin.
*/

QStringList Account::supportedServiceNames() const
{
    if (d->status == Account::Invalid)
        return QStringList();
    return d->supportedServiceNames;
}


/*!
    \qmlproperty Account::Status Account::status
    This property contains the current database-sync status of the account

    An account may have any of five statuses:
    \table
        \header
            \li Status
            \li Description
        \row
            \li Initializing
            \li The account is loading from the database
        \row
            \li Initialized
            \li The account is initialized and ready to be used
        \row
            \li Synced
            \li No outstanding local modifications to the account have occurred since last sync().  Any previous sync() calls have completed.
        \row
            \li SyncInProgress
            \li Any outstanding local modifications are currently being written to the database due to a call to sync().  No local property modifications may occur while the account has this status.
        \row
            \li Modified
            \li Local modifications to the account have occurred since last sync().  In order to persist the changes to the database, sync() must be called.  Note that if another process modifies the canonical (database) version of the account, no signal will be emitted and thus the status of the local account representation will NOT automatically change to Modified.
        \row
            \li SigningIn
            \li The account is creating credentials or signing in with credentials
        \row
            \li Error
            \li An error occurred during account creation or synchronisation.
        \row
            \li Invalid
            \li The account has been removed from the database and is no longer valid.
    \endtable

    Connecting to the account's statusChanged() signal is the usual
    way to handle database synchronisation events.
*/

Account::Status Account::status() const
{
    return d->status;
}


/*!
    \qmlproperty Account::Error Account::error
    This property contains the most recent error which occurred during
    account creation or synchronisation.

    Note that the error will NOT automatically return to \c{NoError}
    if subsequent synchronisation operations succeed.
*/

Account::ErrorType Account::error() const
{
    return d->error;
}


/*!
    \qmlproperty string Account::errorMessage
    This property contains the error message associated with the most
    recent error which occurred during account creation or synchronisation.

    Note that the error message will NOT automatically return to
    being empty if subsequent synchronisation operations succeed.
*/

QString Account::errorMessage() const
{
    return d->errorMessage;
}
