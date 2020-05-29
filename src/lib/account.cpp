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
#include "globaltranslatorcache_p.h"

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

#include <QTimer>
#include <QDBusInterface>
#include <QDBusConnection>
#include <QJSValue>

#define CREDENTIALS_GROUP QLatin1String("segregated_credentials")
#define BUILD_CREDENTIALS_CONFIGURATION_KEY(appName, credName) QString(QLatin1String("%1/%2/%3")).arg(appName).arg(CREDENTIALS_GROUP).arg(credName)

// Used for SSL certificate authentication in some email accounts
#define ACCOUNTS_KEY_SSL_CERT_CREDENTIALS_ID QLatin1String("SslCertCredentialsId")

#define DEFAULT_CREDENTIALS_USERNAME_CONFIGURATION_KEY QLatin1String("default_credentials_username")

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

    QByteArray encryptedData = aes_encrypt_plaintext(ptBA, kBA);
    if (encryptedData.size() == 0) {
        qWarning() << Q_FUNC_INFO << "encryption failed";
        return QString();
    }

    QByteArray b64encryptedData = encryptedData.toBase64();
    return QString::fromLatin1(b64encryptedData);
}

// decodes the given ciphertext from base64 into encrypted data, and decrypts it.
static QString decrypted_string_b64(const QString &ciphertext, const QString &key)
{
    if (key.isEmpty()) {
        return ciphertext;
    }

    QByteArray b64encryptedData = ciphertext.toLatin1();
    QByteArray encryptedData = QByteArray::fromBase64(b64encryptedData);
    QByteArray kBA = key.toUtf8();

    QByteArray decryptedData = aes_decrypt_ciphertext(encryptedData, kBA);
    if (decryptedData.size() == 0) {
        qWarning() << Q_FUNC_INFO << "decryption failed";
        return QString();
    }

    QString decryptedString = QString::fromUtf8(decryptedData);
    return decryptedString;
}

Account::ErrorType signOnErrorToErrorType(int signOnError)
{
    Account::ErrorType errType = Account::SignInUnknownError;
    switch (signOnError) {
        case SignOn::Error::UserInteraction:
            // Credentials have expired / need updating, and the UiPolicy was NoUserInteractionPolicy
            errType = Account::SignInCredentialsExpiredError;
            break;
        case SignOn::Error::PermissionDenied:
            errType = Account::SignInPermissionDeniedError;
            break;
        case SignOn::Error::MissingData:
            errType = Account::SignInMissingDataError;
            break;
        case SignOn::Error::NoConnection:
        case SignOn::Error::Network:
        case SignOn::Error::Ssl:
            errType = Account::SignInNetworkError;
            break;
        default:
            break;
    }

    return errType;
}


void SignInCredentials::cleanup(bool removeIdentity)
{
    if (identity != NULL) {
        if (session != NULL) {
            session->disconnect();
            identity->destroySession(session);
            session = NULL;
        }

        if (removeIdentity == true) {
            identity->signOut();
            identity->remove();
        }

        identity->disconnect();
        identity->deleteLater();
        identity = NULL;
    }

    creatingSignInCredentials = false;
    updatingSignInCredentials = false;
    signingInWithCredentials = false;
    storingEncryptedTokens = false;
    forcingCredentialsRefresh = false;
    haveForcedCredentialsExpiry = false;

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
    , account(nullptr)
    , pendingSync(false)
    , pendingInitModifications(false)
    , identifier(0)
    , enabled(false)
    , identifierPendingInit(false)
    , enabledPendingInit(false)
    , displayNamePendingInit(false)
    , configurationValuesPendingInit(false)
    , constructedWithAccountPtr(false)
    , provisioned(false)
    , readonly(false)
    , limited(false)
    , status(Account::Initializing)
    , error(Account::NoError)
    , sessionState(SignOn::AuthSession::SessionNotStarted)
{
    // initialize the signInCredentials struct
    signInCredentials.creatingSignInCredentials = false;
    signInCredentials.updatingSignInCredentials = false;
    signInCredentials.signingInWithCredentials = false;
    signInCredentials.storingEncryptedTokens = false;
    signInCredentials.forcingCredentialsRefresh = false;
    signInCredentials.haveForcedCredentialsExpiry = false;
    signInCredentials.canceling = false;
    signInCredentials.identity = nullptr;
    signInCredentials.session = nullptr;

    // set up the account
    if (acc) {
        constructedWithAccountPtr = true;
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
    connect(account, SIGNAL(enabledChanged(QString,bool)),
            this, SLOT(enabledHandler(QString,bool)), Qt::UniqueConnection);
    connect(account, SIGNAL(displayNameChanged(QString)),
            this, SLOT(displayNameChangedHandler()), Qt::UniqueConnection);
    connect(account, SIGNAL(synced()),
            this, SLOT(handleSynced()), Qt::UniqueConnection);
    connect(account, SIGNAL(removed()),
            this, SLOT(handleRemoved()), Qt::UniqueConnection);
    connect(account, SIGNAL(destroyed()),
            this, SLOT(invalidate()), Qt::UniqueConnection);

    // grab the provider name: this is necessary for provenance
    if (providerName != account->providerName()) {
        providerName = account->providerName();
        if (!constructedWithAccountPtr) {
            // we need to emit the change, as it happened after construction
            emit q->providerNameChanged();
        }
    }

    // grab the supported service list: this is necessary for enablement etc.
    supportedServiceNames.clear();
    Accounts::ServiceList supportedServices = account->services();
    for (int i = 0; i < supportedServices.size(); ++i) {
        const Accounts::Service &currService(supportedServices.at(i));
        supportedServiceNames.append(currService.name());
    }

    bool validAccountId = (account->id() != 0);

    // grab the default service enabled status unless the user has set it or it's a new account
    if (!enabledPendingInit && validAccountId) {
        if (enabled != account->enabled()) {
            enabled = account->enabled();
            emit q->enabledChanged();
        }
    }

    // similarly for the display name
    if (!displayNamePendingInit && validAccountId) {
        if (displayName != account->displayName()) {
            displayName = account->displayName();
            emit q->displayNameChanged();
        }
    }

    bool newProvisioned = account->value(AccountProvisionedKey, false).toBool();
    if (provisioned != newProvisioned) {
        provisioned = newProvisioned;
        emit q->provisionedChanged();
    }

    bool newReadonly = account->value(AccountReadOnlyKey, false).toBool();
    if (readonly != newReadonly) {
        readonly = newReadonly;
        emit q->readonlyChanged();
    }

    bool newLimited = account->value(AccountLimitedKey, false).toBool();
    if (limited != newLimited) {
        limited = newLimited;
        emit q->limitedChanged();
    }

    // clear the list of service enabled state changes
    serviceEnabledChanges.clear();

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

    bool newProvisioned = account->value(AccountProvisionedKey, false).toBool();
    if (provisioned != newProvisioned) {
        provisioned = newProvisioned;
        emit q->provisionedChanged();
    }

    bool newReadonly = account->value(AccountReadOnlyKey, false).toBool();
    if (readonly != newReadonly) {
        readonly = newReadonly;
        emit q->readonlyChanged();
    }

    bool newLimited = account->value(AccountLimitedKey, false).toBool();
    if (limited != newLimited) {
        limited = newLimited;
        emit q->limitedChanged();
    }

    // supported service names
    supportedServiceNames.clear();
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

    if (configurationValues.contains(DEFAULT_CREDENTIALS_USERNAME_CONFIGURATION_KEY)) {
        setUserName(configurationValues.value(DEFAULT_CREDENTIALS_USERNAME_CONFIGURATION_KEY).toString());
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
    } else {
        emit q->enabledWithServiceChanged(serviceName);
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

void AccountPrivate::handleRemoved()
{
    if (account && account->providerName() == "jolla") {
        updateStoreRepositories(false);
    }
    invalidate();
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

void AccountPrivate::updateStoreRepositories(bool enable)
{
    QDBusInterface ssuInterface("org.nemo.ssu",
                                "/org/nemo/ssu",
                                "org.nemo.ssu",
                                QDBusConnection::systemBus());
    ssuInterface.call("modifyRepo", (enable ? 1 : 0), "store");
    ssuInterface.call("updateRepos");
}

bool AccountPrivate::prepareSync()
{
    if (status == Account::Initializing) {
        pendingSync = true;
    }

    if (status == Account::Invalid
            || status == Account::SyncInProgress
            || status == Account::Initializing) {
        return false;
    }

    if (!account) { // initialization failed.
        error = Account::InitializationFailedError;
        emit q->errorChanged();
        setStatus(Account::Invalid);
        return false;
    }

    if (pendingInitModifications) {
        // we have handled them by directly syncing.
        // after this sync, we will once again allow
        // change signals to cause modifications to the properties.
        pendingInitModifications = false;
        identifierPendingInit = false;
        enabledPendingInit = false;
        displayNamePendingInit = false;
        configurationValuesPendingInit = false;
    }

    // set the user name
    if (!defaultCredentialsUserName.isEmpty() && configurationValues.value(DEFAULT_CREDENTIALS_USERNAME_CONFIGURATION_KEY).toString().isEmpty()) {
        account->setValue(DEFAULT_CREDENTIALS_USERNAME_CONFIGURATION_KEY, defaultCredentialsUserName);
    }

    // set the global configuration values.
    account->selectService(Accounts::Service());
    QStringList allKeys = account->allKeys();
    QStringList setKeys = configurationValues.keys();
    QStringList doneKeys;
    foreach (const QString &key, allKeys) {
        // overwrite existing keys
        if (setKeys.contains(key)) {
            doneKeys.append(key);
            const QVariant &currValue = configurationValues.value(key);
            if (currValue.isValid()) {
                account->setValue(key, currValue);
            } else {
                account->remove(key);
            }
        } else {
            // remove removed keys
            // The CredentialsId key may have been added by Accounts::Account internally due to a
            // call to Accounts::Account::setCredentialsId(), so make sure we don't remove this.
            if (key != ACCOUNTS_KEY_CREDENTIALS_ID && key != DEFAULT_CREDENTIALS_USERNAME_CONFIGURATION_KEY) {
                account->remove(key);
            }
        }
    }
    foreach (const QString &key, setKeys) {
        // add new keys
        if (!doneKeys.contains(key)) {
            const QVariant &currValue = configurationValues.value(key);
            account->setValue(key, currValue);
        }
    }

    // and the service-specific configuration values and service-enabledness status
    foreach (const QString &srvn, supportedServiceNames) {
        Accounts::Service srv = manager->service(srvn);
        if (srv.isValid()) {
            account->selectService(srv);

            // first, configuration values:
            QVariantMap setSrvValues = serviceConfigurationValues.value(srvn);
            QStringList srvKeys = account->allKeys();
            QStringList doneSrvKeys;
            foreach (const QString &key, srvKeys) {
                // overwrite existing keys
                if (setSrvValues.contains(key)) {
                    doneSrvKeys.append(key);
                    const QVariant &currValue = setSrvValues.value(key);
                    if (currValue.isValid()) {
                        account->setValue(key, currValue);
                    } else {
                        account->remove(key);
                    }
                } else {
                    // remove removed keys
                    // The CredentialsId key may have been added by Accounts::Account internally due to a
                    // call to Accounts::Account::setCredentialsId(), so make sure we don't remove this.
                    if (key != ACCOUNTS_KEY_CREDENTIALS_ID) {
                        account->remove(key);
                    }
                }
            }
            foreach (const QString &key, setSrvValues.keys()) {
                // add new keys
                if (!doneSrvKeys.contains(key)) {
                    const QVariant &currValue = setSrvValues.value(key);
                    account->setValue(key, currValue);
                }
            }

            // If the saved enabled state was stored in the config values and thus updated above
            // then we need to overwrite it here with the updated enabled state.
            if (serviceEnabledChanges.contains(srvn)) {
                account->setEnabled(serviceEnabledChanges[srvn]);
            }
        }
    }

    // enable or disable the global service
    account->selectService(Accounts::Service());
    account->setEnabled(enabled);

    // set the display name
    account->setDisplayName(displayName);

    // and write to database.
    setStatus(Account::SyncInProgress);

    return true;
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

        bool newProvisioned = account->value(AccountProvisionedKey, false).toBool();
        if (provisioned != newProvisioned) {
            provisioned = newProvisioned;
            emit q->provisionedChanged();
        }

        bool newReadonly = account->value(AccountReadOnlyKey, false).toBool();
        if (readonly != newReadonly) {
            readonly = newReadonly;
            emit q->readonlyChanged();
        }

        bool newLimited = account->value(AccountLimitedKey, false).toBool();
        if (limited != newLimited) {
            limited = newLimited;
            emit q->limitedChanged();
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

        if (configurationValues.contains(DEFAULT_CREDENTIALS_USERNAME_CONFIGURATION_KEY)) {
            setUserName(configurationValues.value(DEFAULT_CREDENTIALS_USERNAME_CONFIGURATION_KEY).toString());
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
                    setStatus(Account::Synced);
                    //: Error emitted if unable to decrypt the stored encrypted credentials
                    //% "Unable to decrypt stored credentials - aborting credentials creation"
                    emit q->signInError(qtTrId("sailfish_accounts-account-decryption_failed"), Account::SignInPermissionDeniedError);
                }
            } else {
                // "oauth2" method - we just emit the cached response data.
                QVariantMap responseData = signInCredentials.responseData;
                signInCredentials.cleanup();
                setStatus(Account::Synced);
                if (account->providerName() == "jolla") {
                    updateStoreRepositories(true);
                }
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
        setStatus(Account::Synced);
        //: Error emitted if an error occurred while storing credentials
        //% "Unable to store credentials - invalid id"
        emit q->signInError(qtTrId("sailfish_accounts-account-identity_id_failed"), Account::SignInInvalidCredentialsError);
        return;
    }

    signInCredentials.session = signInCredentials.identity->createSession(signInCredentials.method);
    if (!signInCredentials.session) {
        signInCredentials.cleanup(true);
        setStatus(Account::Synced);
        //: Error emitted if an error occurred while creating a signon session after storing the credentials
        //% "Unable to create signon session after storing credentials"
        emit q->signInError(qtTrId("sailfish_accounts-account-session_create_after_store_credentials_failed"), Account::SignInUnknownError);
        return;
    }

    connect(signInCredentials.session, SIGNAL(response(SignOn::SessionData)),
            this, SLOT(handleResponse(SignOn::SessionData)), Qt::UniqueConnection);
    connect(signInCredentials.session, SIGNAL(error(SignOn::Error)),
            this, SLOT(handleSignOnError(SignOn::Error)), Qt::UniqueConnection);
    connect(signInCredentials.session, SIGNAL(stateChanged(SignOn::AuthSession::AuthSessionState,QString)),
            this, SLOT(handleStateChanged(SignOn::AuthSession::AuthSessionState,QString)), Qt::UniqueConnection);

    signInCredentials.session->process(SignOn::SessionData(signInCredentials.sessionData), signInCredentials.mechanism);
}

void AccountPrivate::handleCredentialsInfo(const SignOn::IdentityInfo &info)
{
    if (!signInCredentials.updatingSignInCredentials) {
        return;
    }

    // update with the new username and password, and store them.
    SignOn::IdentityInfo updatedInfo(info);
    if (!signInCredentials.username.isEmpty())
        updatedInfo.setUserName(signInCredentials.username);
    if (!signInCredentials.password.isEmpty())
        updatedInfo.setSecret(signInCredentials.password, true);
    signInCredentials.identityInfo = updatedInfo;

    connect(signInCredentials.identity, SIGNAL(credentialsStored(quint32)),
            this, SLOT(handleCredentialsStored(quint32)), Qt::UniqueConnection);

    signInCredentials.identity->storeCredentials(signInCredentials.identityInfo);
}

static void maybeSetCredentialsIdForProvider(Accounts::Account *account,
                                             int identityId,
                                             const QString &method,
                                             const QString &serviceName,
                                             const QString &symmetricKey,
                                             const QString &username,
                                             QVariantMap *configurationValues)
{
    // if the method is oauth2, and if no credentialsId has been set
    // for the services from that provider, we can set the given credentialsId
    // as the credentials for all services from that provider (since
    // oauth2 requires a ClientId / ConsumerKey so it's already per-application).

    // if the method is password, and if no symmetricKey was specified,
    // we can do the same (as the application is saying "no need to
    // keep these credentials application-specific").

    if (method.toLower() == QLatin1String("oauth2")) {
        // set for each service from the provider (that the account supports)
        // NOTE: this assumes that each service uses the oauth method.  TODO: fixme?  Requires Accounts::Service API to be improved.
        Accounts::ServiceList supportedServices = account->services();
        for (int i = 0; i < supportedServices.size(); ++i) {
            const Accounts::Service &currService(supportedServices.at(i));
            account->selectService(currService);
            if (account->credentialsId() == 0) {
                account->setCredentialsId(identityId);
            }
        }
        // set the global account credentials to that id, if not already set.
        account->selectService(Accounts::Service());
        if (account->credentialsId() == 0) {
            account->setCredentialsId(identityId);
        }
    } else if (method.toLower() == QLatin1String("password") && symmetricKey.isEmpty()) {
        // set for each service from the provider (that the account supports)
        Accounts::ServiceList supportedServices = account->services();
        for (int i = 0; i < supportedServices.size(); ++i) {
            const Accounts::Service &currService(supportedServices.at(i));
            account->selectService(currService);
            if (account->credentialsId() == 0 || currService.name() == serviceName) {
                account->setCredentialsId(identityId);
            }
        }

        account->selectService(Accounts::Service());
        // set the global account credentials to that id, if not already set.
        if (account->credentialsId() == 0) {
            account->setCredentialsId(identityId);
        }
    }

    if (!username.isEmpty()) {
        account->setValue(DEFAULT_CREDENTIALS_USERNAME_CONFIGURATION_KEY, username);
        configurationValues->insert(DEFAULT_CREDENTIALS_USERNAME_CONFIGURATION_KEY, username);
        if (account->displayName().isEmpty()) {
            // display name defaults to username
            account->setDisplayName(username);
        }
    }
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

        // second, re-set all of the configuration values, as they will
        // have been clobbered by the store sign in credentials call.
        account->selectService(Accounts::Service());
        foreach (const QString &cachedKey, configurationValues.keys()) {
            account->setValue(cachedKey, configurationValues.value(cachedKey));
        }
        foreach (const QString &srvName, serviceConfigurationValues.keys()) {
            Accounts::Service srv = manager->service(srvName);
            if (srv.isValid()) {
                account->selectService(srv);
                QVariantMap scvs = serviceConfigurationValues.value(srvName);
                foreach (const QString &cachedKey, scvs.keys()) {
                    account->setValue(cachedKey, scvs.value(cachedKey));
                }
            }
        }

        // then update the account with the credentials information.
        QString credName = signInCredentials.credentialsName.isEmpty() ? QLatin1String("default") : signInCredentials.credentialsName;
        QString configurationValueKey = BUILD_CREDENTIALS_CONFIGURATION_KEY(signInCredentials.applicationName, credName);
        account->selectService(Accounts::Service());
        account->setValue(configurationValueKey, signInCredentials.identity->id());
        configurationValues.insert(configurationValueKey, signInCredentials.identity->id());

        // if the provider is Jolla, we should write the username to an account setting.
        if (account->providerName() == QString::fromLatin1("jolla")) {
            account->selectService(manager->service(QString::fromLatin1("jolla-store")));
            account->setValue(QString::fromLatin1("username"), signInCredentials.username);
            account->selectService(Accounts::Service());
            QVariantMap storeValues = serviceConfigurationValues.value(QString::fromLatin1("jolla-store"));
            storeValues.insert(QString::fromLatin1("username"), signInCredentials.username);
            serviceConfigurationValues.insert(QString::fromLatin1("jolla-store"), storeValues);
        }

        maybeSetCredentialsIdForProvider(account,
                                         signInCredentials.identity->id(),
                                         signInCredentials.method,
                                         signInCredentials.serviceName,
                                         signInCredentials.symmetricKey,
                                         signInCredentials.username,
                                         &configurationValues);

        // and write the changes to the accounts database
        connect(account, SIGNAL(error(Accounts::Error)), this, SLOT(handleAccountError()), Qt::UniqueConnection);
        /* have already connected account->synced() to handleSynced() */
        account->sync();
    } else if (signInCredentials.updatingSignInCredentials || signInCredentials.signingInWithCredentials) {
        // if it is "password" method, then the username/password are encrypted, and we need to decrypt.
        // if it is "oauth2" (oauth1.0a / oauth2) then we just emit the tokens immediately,
        // as the security is provided by signond (and the fact that the client needs to know the clientid),
        // unless we're forcing a refresh.
        signInCredentials.responseData.clear();
        QStringList keys = data.propertyNames();
        foreach (const QString &key, keys) {
            signInCredentials.responseData.insert(key, data.getProperty(key));
        }

        bool emitUpdated = signInCredentials.updatingSignInCredentials;
        if (signInCredentials.method.toLower() == QLatin1String("oauth2")) {
            QVariantMap responseData = signInCredentials.responseData;
            if (signInCredentials.forcingCredentialsRefresh) {
                if (!signInCredentials.haveForcedCredentialsExpiry) {
                    // we need to use the ProvidedTokens hook to overwrite the expiry.
                    signInCredentials.haveForcedCredentialsExpiry = true;
                    QVariantMap providedTokens;
                    Q_FOREACH (const QString &key, responseData.keys()) {
                        if (key == QStringLiteral("ExpiresIn")) {
                            providedTokens.insert(key, QVariant::fromValue<int>(1));
                        } else {
                            providedTokens.insert(key, responseData.value(key));
                        }
                    }
                    QVariantMap sipParams = signInCredentials.sessionData;
                    sipParams.insert(QStringLiteral("ProvidedTokens"), providedTokens);
                    signInCredentials.session->process(SignOn::SessionData(sipParams), signInCredentials.mechanism);
                } else {
                    // we need to wait for the expiry to finish before forcing refresh.
                    signInCredentials.forcingCredentialsRefresh = false;
                    QTimer::singleShot(3000, this, SLOT(handleExpiryTimeout()));
                }
            } else {
                signInCredentials.cleanup();
                setStatus(Account::Synced);
                if (emitUpdated) {
                    emit q->signInCredentialsUpdated(responseData);
                } else {
                    emit q->signInResponse(responseData);
                }
            }
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
                setStatus(Account::Synced);
                if (emitUpdated) {
                    emit q->signInCredentialsUpdated(responseData);
                } else {
                    emit q->signInResponse(responseData);
                }
            } else {
                setStatus(Account::Synced);
                //: Error emitted if unable to decrypt the stored encrypted credentials
                //% "Unable to decrypt stored credentials - aborting credentials creation"
                emit q->signInError(qtTrId("sailfish_accounts-account-decryption_failed"), Account::SignInPermissionDeniedError);
            }
        }
    }
}

void AccountPrivate::handleExpiryTimeout()
{
    // we've expired the tokens successfully. Now we perform "normal" sign-in.
    signInCredentials.session->process(SignOn::SessionData(signInCredentials.sessionData), signInCredentials.mechanism);
}

void AccountPrivate::handleCredentialsFailed(const SignOn::Error &err)
{
    if (signInCredentials.creatingSignInCredentials || signInCredentials.updatingSignInCredentials) {
        Account::ErrorType errType = signOnErrorToErrorType(err.type());
        QString providerName = account->providerName();
        signInCredentials.cleanup(signInCredentials.creatingSignInCredentials); // delete identity if creating.
        setStatus(Account::Synced);
        //: Error emitted if account credentials save failed at the database level
        //% "Unable to save credentials for %1 account in database: %2"
        emit q->signInError(qtTrId("sailfish_accounts-account-credentials_database_failed").arg(providerName).arg(err.message()), errType);
    }
}

void AccountPrivate::handleSignOnError(const SignOn::Error &err)
{
    //: Error emitted if signon failed due to network connection failure
    //% "Network connection failure"
    QString networkConnectionFailure = qtTrId("sailfish_accounts-account-network_failed");

    //: Error emitted if signon failed due to having no cached credentials, and NoUserInteractionPolicy specified
    //% "No cached credentials exist"
    QString noCachedCredentials = qtTrId("sailfish_accounts-account-no_cached_creds");

    bool sessionWasRunning = (signInCredentials.session != NULL);
    if (signInCredentials.creatingSignInCredentials || signInCredentials.updatingSignInCredentials) {
        signInCredentials.cleanup(signInCredentials.creatingSignInCredentials); // delete identity if creating.
    } else {
        signInCredentials.signingInWithCredentials = false; // error occured.
    }

    Account::ErrorType errType = signOnErrorToErrorType(err.type());
    QString errMess = err.message();
    setStatus(Account::Synced);
    if (errMess == QLatin1String("userActionFinished error: 5")) {
        emit q->signInError(networkConnectionFailure, Account::SignInNetworkError);
    } else if (errMess == QLatin1String("userActionFinished error: 10")) {
        emit q->signInError(noCachedCredentials, Account::SignInCredentialsExpiredError);
    } else if (err.type() == SignOn::Error::SessionCanceled && (signInCredentials.canceling || !sessionWasRunning)) {
        // ignore, cancelSignInOperation() was called and it would have already emitted signInError()
    } else {
        emit q->signInError(errMess, errType);
    }
}

void AccountPrivate::handleStateChanged(SignOn::AuthSession::AuthSessionState state, const QString &message)
{
    Q_UNUSED(message)
    sessionState = state;
}

void AccountPrivate::handleAccountError()
{
    if (signInCredentials.creatingSignInCredentials) {
        QString providerName = account->providerName();
        signInCredentials.cleanup(true);
        setStatus(Account::Synced);
        //: Error emitted if account creation failed at the database level
        //% "Unable to save %1 account in database"
        emit q->signInError(qtTrId("sailfish_accounts-account-account_database_failed").arg(providerName), Account::DatabaseError);
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
                if (symmetricKey.isEmpty()) {
                    retn.insert(key, decryptedPassword);
                } else if (decryptedPassword.endsWith(applicationName)) {
                    decryptedPassword.chop(applicationName.length());
                    retn.insert(key, decryptedPassword);
                } else {
                    // else, they supplied the wrong decryption key.
                    *succeeded = false;
                }
            } else if (key.toLower() == QLatin1String("username")) {
                QString decryptedUsername = decrypted_string_b64(encryptedResponseData.value(key).toString(), symmetricKey);
                if (symmetricKey.isEmpty()) {
                    retn.insert(key, decryptedUsername);
                } else if (decryptedUsername.endsWith(applicationName)) {
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

void AccountPrivate::cancelCredentialsOperation(bool removeIdentity)
{
    if (signInCredentials.session
            && sessionState != SignOn::AuthSession::SessionNotStarted
            && sessionState != SignOn::AuthSession::ProcessCanceling
            && sessionState != SignOn::AuthSession::ProcessDone) {
        signInCredentials.canceling = true;
        signInCredentials.session->cancel();
        signInCredentials.canceling = false;
    }
    signInCredentials.cleanup(removeIdentity);
    setStatus(Account::Synced);

    //: Error emitted if the sign-in operation was canceled
    //% "The sign-in operation was canceled"
    emit q->signInError(qtTrId("sailfish_accounts-account-credentials-operation-canceled"), Account::SignInOperationCanceledError);
}

void AccountPrivate::setUserName(const QString &user)
{
    if (!user.isEmpty() && user != defaultCredentialsUserName) {
        defaultCredentialsUserName = user;
        emit q->defaultCredentialsUserNameChanged();
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
    SailfishAccounts::initLibTranslator();
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
            Accounts::Account *existingAccount = Accounts::Account::fromId(d->manager, d->identifier, this);
            d->setAccount(existingAccount, true);
        }
    } else {
        // account was provided by AccountFactory.
        // do nothing.
    }
}

// helpers for AccountFactory only.
Accounts::Account *Account::account() { return d->account; }
Account::Account(bool queryInfoOnCreation, Accounts::Account *account, QObject *parent, const QVariantMap &serviceConfigValues)
    : QObject(parent), d(new AccountPrivate(this, account, queryInfoOnCreation))
{
    if (!serviceConfigValues.isEmpty()) {
        foreach (const QString &serviceName, serviceConfigValues.keys()) {
            // sanitize the configuration values
            if (serviceConfigValues[serviceName].type() != QVariant::Map) {
                qWarning() << Q_FUNC_INFO << "Configuration for service" << serviceName << "is not a QVariantMap!";
                continue;
            }

            QVariantMap sanitizedConfig;
            QVariantMap vm = serviceConfigValues[serviceName].toMap();
            foreach (const QString &key, vm.keys()) {
                QVariant currValue = vm[key];
                if (currValue.type() == QVariant::Bool
                        || currValue.type() == QVariant::Int
                        || currValue.type() == QVariant::UInt
                        || currValue.type() == QVariant::LongLong
                        || currValue.type() == QVariant::ULongLong
                        || currValue.type() == QVariant::String
                        || currValue.type() == QVariant::StringList) {
                    sanitizedConfig.insert(key, currValue);
                } else if (currValue.type() == QVariant::List) {
                    sanitizedConfig.insert(key, currValue.toStringList());
                } else {
                    qWarning() << Q_FUNC_INFO << "Unsupported configuration value" << currValue << "for key" << key;
                }
            }

            if (serviceName.isEmpty()) {
                foreach (const QString &key, sanitizedConfig.keys()) {
                    d->account->setValue(key, sanitizedConfig.value(key));
                }
                d->configurationValues = sanitizedConfig;
            } else {
                Accounts::Service srv = d->manager->service(serviceName);
                if (!srv.isValid()) {
                    qWarning() << Q_FUNC_INFO << "Unsupported service" << serviceName;
                } else {
                    d->account->selectService(srv);
                    foreach (const QString &key, sanitizedConfig.keys()) {
                        d->account->setValue(key, sanitizedConfig.value(key));
                    }
                    d->account->selectService(Accounts::Service());
                    d->serviceConfigurationValues.insert(serviceName, sanitizedConfig);
                }
            }
        }
    }
}

/*!
    \qmlmethod void Account::blockingSync()

    This is the same as sync(), but the operation is guaranteed
    to be synchronous.
*/
void Account::blockingSync()
{
    if (d->prepareSync()) {
        d->account->syncAndBlock();
        d->handleSynced();
    }
}

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
    if (d->prepareSync()) {
        d->account->sync();
    }
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
        if (key.contains(CREDENTIALS_GROUP)
                || key.contains(ACCOUNTS_KEY_SSL_CERT_CREDENTIALS_ID)) {
            int identityId = d->account->valueAsInt(key, 0);
            if (identityId) {
                SignOn::Identity *doomedIdentity = SignOn::Identity::existingIdentity(identityId, this);
                if (doomedIdentity) {
                    doomedIdentity->signOut();
                    doomedIdentity->remove();
                }
            }
        }
    }

    if (d->account->id()) {
        // only remove if we've synced / have an id.
        d->setStatus(Account::SyncInProgress);
        d->account->remove();
        d->account->sync();
    } else {
        d->invalidate();
    }
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

    The configuration values are retrieved asynchronously after
    account construction.  Clients should wait until the account
    is in the \c Initialized or \c Synced state before they attempt
    to access (or modify) the configuration values.
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

    The follow variant types are supported:

    \list
    \li QVariant::Int
    \li QVariant::UInt
    \li QVariant::LongLong
    \li QVariant::ULongLong
    \li QVariant::String
    \li QVariant::StringList
    \li QVariant::List (in this case, the value will be converted to a QStringList)
    \endlist

    If a variant in the given \a values uses a type outside of those listed above, it will not
    be added to the configuration settings.
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
                || currValue.type() == QVariant::UInt
                || currValue.type() == QVariant::LongLong
                || currValue.type() == QVariant::ULongLong
                || currValue.type() == QVariant::String
                || currValue.type() == QVariant::StringList) {
            validValues.insert(key, currValue);
        } else if (currValue.type() == QVariant::List) {
            validValues.insert(key, currValue.toStringList());
        } else if (currValue.userType() == QMetaType::type("QJSValue")) {
            QVariant convertedValue = currValue.value<QJSValue>().toVariant();
            if (convertedValue.isValid()) {
                validValues.insert(key, currValue.toStringList());
            } else {
                qWarning() << "Account::setConfigurationValues(): variant type QJSValue for key '" + key + "' cannot be converted, value will not be added";
            }
        } else {
            qWarning() << "Account::setConfigurationValues(): variant type " << currValue.typeName()
                       << "for key '" + key + "' not supported, value will not be added";
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
    given \a value. The \a value must be of a supported type; see setConfigurationValues() for
    the list of types that are supported.

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
    // Return the (non-saved) enabled state if it was changed but we haven't synced yet.
    if (d->serviceEnabledChanges.contains(serviceName)) {
        return d->serviceEnabledChanges[serviceName];
    }

    bool retn = false;

    // Return the saved enabled state.
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

    Note: After calling this, isEnabledWithService() will report the service as enabled, but
    this change will not take effect in the database until sync() is called.
    The \l enabledWithServiceChanged() signal will be emitted when the change
    is committed to the database.
*/
void Account::enableWithService(const QString &serviceName)
{
    if (d->status == Account::Invalid || d->status == Account::SyncInProgress) {
        return;
    }

    if (d->serviceEnabledChanges.value(serviceName)) {
        return;
    }

    if (d->supportedServiceNames.contains(serviceName)) {
        Accounts::Service srv = d->manager->service(serviceName);
        if (srv.isValid()) {
            d->serviceEnabledChanges[serviceName] = true;
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

    Note: After calling this, isEnabledWithService() will report the service as disabled, but
    this change will not take effect in the database until sync() is called.
    The \l enabledWithServiceChanged() signal will be emitted when the change
    is committed to the database.
*/
void Account::disableWithService(const QString &serviceName)
{
    if (d->status == Account::Invalid || d->status == Account::SyncInProgress)
        return;

    if (d->serviceEnabledChanges.contains(serviceName) && !d->serviceEnabledChanges[serviceName]) {
        return;
    }

    if (d->supportedServiceNames.contains(serviceName)) {
        Accounts::Service srv = d->manager->service(serviceName);
        if (srv.isValid()) {
            d->serviceEnabledChanges[serviceName] = false;
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
    QString validServiceName;
    QString method;
    QString mechanism;
    QVariantMap parameters;

    // Note: we don't use service-segregation, but instead we use per-application segregation.
    // So, we use the ServiceAccount's AuthData only to get the method/mechanism/params.
    Accounts::Service srv = d->manager->service(serviceName);
    if (srv.isValid() && d->account) {
        Accounts::AccountService as(d->account, srv);
        Accounts::AuthData authData(as.authData());
        validServiceName = serviceName;
        method = authData.method();
        mechanism = authData.mechanism();
        parameters = authData.parameters();
    } else {
        qWarning() << Q_FUNC_INFO << "No such service:" << serviceName;
    }

    return new SignInParameters(validServiceName, method, mechanism, parameters, username, password, this);
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
    application will be able to use those credentials with the service,
    and in fact the credentials will be set as the default credentials for
    the account with that service.

    Once creation of credentials completes successfully the
    \c signInCredentialsCreated() signal will be emitted.  If creation of the
    credentials encounters an error then the \c signInError() signal will
    be emitted.  The account will transition to the \c Synced state just prior
    to emission of the success or error signals.

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
        emit signInError(qtTrId("sailfish_accounts-account-csic_invalid_status"), Account::SignInInvalidStatusError);
        return;
    }

    if (parameters == NULL) {
        //: Error emitted if function called with invalid parameters
        //% "Invalid sign-in parameters specified"
        emit signInError(qtTrId("sailfish_accounts-account-csic_invalid_params"), Account::SignInMissingDataError);
        return;
    }

    if (applicationName.isEmpty()) {
        //: Error emitted if function called with invalid application name
        //% "Invalid application name specified"
        emit signInError(qtTrId("sailfish_accounts-account-csic_invalid_appname"), Account::SignInMissingDataError);
        return;
    }

    Accounts::Service service(d->manager->service(parameters->serviceName()));
    if (!service.isValid()) {
        //: Error emitted if function called with invalid service name
        //% "Invalid service name specified via SignInParameters"
        emit signInError(qtTrId("sailfish_accounts-account-csic_same"), Account::SignInInvalidCredentialsError);
        return;
    }

    // For non-oauth2 signon:
    // step one: check if credentials already exist, and if not:
    // step two: create identity
    // step three: check if symmetric key given, if not jump to step eight.
    // step four: append application name to username/password
    // step five: encrypt username/password
    // step six: base64 encode the result
    // step seven: store back into identity.
    // step eight: save identity id into account config settings for the application.
    // step nine: if no symmetric key given, set credentials as default for the service.
    // step ten: emit success including plain text credentials.

    // For oauth2 signon:
    // step one: check if the credentials already exist, if not
    // step two: create identity
    // step three: perform signon
    // step four: emit success including plain text tokens returned from signond

    if (hasSignInCredentials(applicationName, credentialsName)) {
        //: Error emitted if signon credentials already exist
        //% "Named credentials already exist for this application"
        emit signInError(qtTrId("sailfish_accounts-account-csic_already_exist"), Account::SignInMissingDataError);
        return;
    }

    if (parameters->method().toLower() == QLatin1String("oauth2")) {
        // oauth-based authentication.  trigger sign-on process.
        // Because application segregation is done in signond (via ClientId/ConsumerKey token separation)
        // we can re-use existing default credentials if they exist.
        bool needToCreateIdentity = true;
        d->account->selectService(service);
        quint32 defaultIdentityId = d->account->credentialsId();
        if (defaultIdentityId != 0) {
            SignOn::Identity *existingIdent = SignOn::Identity::existingIdentity(defaultIdentityId);
            if (existingIdent) {
                // we can re-use this identity.
                needToCreateIdentity = false;
                d->signInCredentials.identity = existingIdent;
            } else {
                // the default identity does not actually exist.
                // reset it for the particular service
                d->account->setCredentialsId(0);

                // reset it for the global service
                d->account->selectService(Accounts::Service());
                if (d->account->credentialsId() == defaultIdentityId) {
                    d->account->setCredentialsId(0);
                }

                // note: we don't sync the account yet - that happens later.
            }
        }

        d->signInCredentials.applicationName = applicationName;
        d->signInCredentials.symmetricKey = symmetricKey;
        d->signInCredentials.serviceName = parameters->serviceName();
        d->signInCredentials.method = parameters->method();
        d->signInCredentials.mechanism = parameters->mechanism();
        d->signInCredentials.sessionData = parameters->parameters();
        d->signInCredentials.credentialsName = credentialsName;
        d->signInCredentials.username = parameters->username();
        d->signInCredentials.password = QString();
        d->signInCredentials.creatingSignInCredentials = true;
        d->signInCredentials.updatingSignInCredentials = false;
        d->signInCredentials.storingEncryptedTokens = false; // we never attempt to store encrypted tokens for OAuth2, signond handles that.
        d->signInCredentials.forcingCredentialsRefresh = false; // creating, don't need to force refresh
        d->signInCredentials.haveForcedCredentialsExpiry = false;

        if (needToCreateIdentity) {
            // we need to create the credentials.
            QMap<QString, QStringList> methodMechanisms;
            methodMechanisms.insert(parameters->method(), QStringList(parameters->mechanism()));
            d->signInCredentials.identityInfo = SignOn::IdentityInfo(applicationName, parameters->username(), methodMechanisms);
            d->signInCredentials.identity = SignOn::Identity::newIdentity(d->signInCredentials.identityInfo);
            if (d->signInCredentials.identity == NULL) {
                //: Error emitted if identity creation fails
                //% "Failed to create credentials"
                emit signInError(qtTrId("sailfish_accounts-account-oauth_identity_failed"), Account::SignInInvalidCredentialsError);
                return;
            }

            connect(d->signInCredentials.identity, SIGNAL(error(SignOn::Error)),
                    d, SLOT(handleCredentialsFailed(SignOn::Error)), Qt::UniqueConnection);
            connect(d->signInCredentials.identity, SIGNAL(credentialsStored(quint32)),
                    d, SLOT(handleCredentialsStored(quint32)), Qt::UniqueConnection);

            d->setStatus(Account::SigningIn);
            d->signInCredentials.identity->storeCredentials(d->signInCredentials.identityInfo);
        } else {
            connect(d->signInCredentials.identity, SIGNAL(error(SignOn::Error)),
                    d, SLOT(handleCredentialsFailed(SignOn::Error)), Qt::UniqueConnection);
            d->setStatus(Account::SigningIn);
            d->handleCredentialsStored(d->signInCredentials.identity->id()); // reusing previously stored identity.
        }
    } else {
        // password-based authentication.  encrypt and store the credentials directly.
        // note: we _always_ create new identity for this.  We don't try to re-use
        // the default if it exists, just because someone can (out of band) set an encrypted
        // identity as the account default credentials, causing problems.
        QString identityUsername = parameters->username();
        if (!symmetricKey.isEmpty()) {
            // append the application name if we're encrypting.
            identityUsername += applicationName;
            identityUsername = b64_encrypted_string(identityUsername, symmetricKey);
        }
        if (identityUsername.isNull()) {
            //: Error emitted if encrypting username fails
            //% "Error occurred while encrypting username"
            emit signInError(qtTrId("sailfish_accounts-account-uname_encryption_failed"), Account::SignInUnknownError);
            return;
        }

        QString identitySecret = parameters->password();
        if (!symmetricKey.isEmpty()) {
            // only append the application name if we're encrypting.
            identitySecret += applicationName;
            identitySecret = b64_encrypted_string(identitySecret, symmetricKey);
        }
        if (identitySecret.isNull()) {
            //: Error emitted if encrypting password fails
            //% "Error occurred while encrypting password"
            emit signInError(qtTrId("sailfish_accounts-account-pword_encryption_failed"), Account::SignInUnknownError);
            return;
        }

        QMap<QString, QStringList> methodMechanisms;
        methodMechanisms.insert(parameters->method(), QStringList(parameters->mechanism()));
        d->signInCredentials.identityInfo = SignOn::IdentityInfo(applicationName, identityUsername, methodMechanisms);
        d->signInCredentials.identityInfo.setSecret(identitySecret);
        d->signInCredentials.identity = SignOn::Identity::newIdentity(d->signInCredentials.identityInfo);
        if (d->signInCredentials.identity == NULL) {
            //: Error emitted if identity creation fails
            //% "Failed to create credentials"
            emit signInError(qtTrId("sailfish_accounts-account-unpw_identity_failed"), Account::SignInUnknownError);
            return;
        }

        d->signInCredentials.applicationName = applicationName;
        d->signInCredentials.symmetricKey = symmetricKey;
        d->signInCredentials.serviceName = parameters->serviceName();
        d->signInCredentials.method = parameters->method();
        d->signInCredentials.mechanism = parameters->mechanism();
        d->signInCredentials.sessionData = parameters->parameters();
        d->signInCredentials.credentialsName = credentialsName;
        d->signInCredentials.username = parameters->username();
        d->signInCredentials.password = parameters->password();
        d->signInCredentials.creatingSignInCredentials = true;
        d->signInCredentials.updatingSignInCredentials = false;
        d->signInCredentials.storingEncryptedTokens = true; // for password method, we store encrypted username/password immediately
        d->signInCredentials.forcingCredentialsRefresh = false;
        d->signInCredentials.haveForcedCredentialsExpiry = false;

        connect(d->signInCredentials.identity, SIGNAL(error(SignOn::Error)),
                d, SLOT(handleCredentialsFailed(SignOn::Error)), Qt::UniqueConnection);
        connect(d->signInCredentials.identity, SIGNAL(credentialsStored(quint32)),
                d, SLOT(handleCredentialsStored(quint32)), Qt::UniqueConnection);

        d->setStatus(Account::SigningIn);
        d->signInCredentials.identity->storeCredentials(d->signInCredentials.identityInfo);
    }
}

/*!
    \qmlmethod Account::updateSignInCredentials(const QString &applicationName, const QString &credentialsName, SignInParameters *parameters, const QString &symmetricKey)

    Updates the existing credentials with the given \a credentialsName
    for the application with the given \a applicationName.

    If the \a applicationName is invalid or the named credentials do
    not exist, or if the account is not in either the \c Initialized
    or \c Synced state, calling this function will immediately fail.

    If the credentials are OAuth credentials, the tokens will be cleared
    and the user will be asked to sign in again via webview in order to
    generate new, valid tokens.

    If the credentials are non-OAuth credentials, the username and
    password stored in the existing credentials will be cleared and
    replaced with those specified in the \a parameters.  If the
    \a symmetricKey is non-empty, it will be used to encrypt the
    username and password before being stored.

    On success the \l signInCredentialsUpdated() signal will be emitted.
    On failure the \l signInError() signal will be emitted.  The account
    will transition to the \c Synced state just prior to the success or
    error signals being emitted.
*/
void Account::updateSignInCredentials(const QString &applicationName,
                                      const QString &credentialsName,
                                      SignInParameters *parameters,
                                      const QString &symmetricKey)
{
    // step one: find out if credentials exist; if not fail.
    // step two: look at credentials type (oauth vs password)
    //  -> if oauth, just sign in via RequestPasswordPolicy (clears tokens and gets new ones)
    //  -> if password, check symmetric key; update username/password in identity; sync.

    if (d->status != Account::Initialized && d->status != Account::Synced) {
        //: Error emitted if function called while account is in invalid state
        //% "Account status is not Initialized or Synced"
        emit signInError(qtTrId("sailfish_accounts-account-usic_invalid_status"), Account::SignInInvalidStatusError);
        return;
    }

    if (parameters == NULL) {
        //: Error emitted if function called with invalid parameters
        //% "Invalid sign-in parameters specified"
        emit signInError(qtTrId("sailfish_accounts-account-usic_invalid_params"), Account::SignInMissingDataError);
        return;
    }

    if (applicationName.isEmpty()) {
        //: Error emitted if function called with invalid application name
        //% "Invalid application name specified"
        emit signInError(qtTrId("sailfish_accounts-account-usic_invalid_appname"), Account::SignInMissingDataError);
        return;
    }

    if (!hasSignInCredentials(applicationName, credentialsName)) {
        //: Error emitted if no such credentials exist
        //% "Cannot update nonexistent credentials"
        emit signInError(qtTrId("sailfish_accounts-account-usic_nonexistent_credentials"), Account::SignInInvalidCredentialsError);
        return;
    }

    if (d->signInCredentials.creatingSignInCredentials
            || d->signInCredentials.updatingSignInCredentials
            || d->signInCredentials.signingInWithCredentials) {
        //: Error emitted if function called while account is already signing in
        //% "Account is currently creating, updating, removing or signing in with credentials"
        emit signInError(qtTrId("sailfish_accounts-account-usic_signin_busy"), Account::SignInInvalidStatusError);
        return;
    }

    // retrieve the identity (credentials) id specified
    QString credName = credentialsName.isEmpty() ? QLatin1String("default") : credentialsName;
    QString configurationValueKey = BUILD_CREDENTIALS_CONFIGURATION_KEY(applicationName, credName);
    quint32 identityId = d->configurationValues.value(configurationValueKey, QVariant::fromValue<int>(0)).toInt();
    SignOn::Identity *updateIdentity = identityId == 0 ? NULL : SignOn::Identity::existingIdentity(identityId, this);

    if (updateIdentity == NULL) {
        //: Error emitted if identity could not be loaded in order to update it
        //% "Failed to load credentials to update"
        emit signInError(qtTrId("sailfish_accounts-account-update_load_failed"), Account::SignInInvalidCredentialsError);
        return;
    }

    // XXX TODO: It would be nice if we could do some "programmer protection" here:
    // check to see if the method/mechanism in the parameters matches that of the identity.
    // But to do so, we need to query the info struct from the identity, which is async
    // and causes a read on the FS / signon DB, which is not performant.
    // Instead, we assume that the SignInParameters are "correct".

    if (parameters->method().toLower() == QLatin1String("oauth2")) {

        QVariantMap modifiedParameters = parameters->parameters();
        modifiedParameters.insert("UiPolicy", SignInParameters::RequestPasswordPolicy);

        d->signInCredentials.identity = updateIdentity;
        d->signInCredentials.applicationName = applicationName;
        d->signInCredentials.symmetricKey = symmetricKey;
        d->signInCredentials.serviceName = parameters->serviceName();
        d->signInCredentials.method = parameters->method();
        d->signInCredentials.mechanism = parameters->mechanism();
        d->signInCredentials.sessionData = modifiedParameters;
        d->signInCredentials.credentialsName = credentialsName;
        d->signInCredentials.username = parameters->username();
        d->signInCredentials.password = QString();
        d->signInCredentials.creatingSignInCredentials = false;
        d->signInCredentials.updatingSignInCredentials = true;
        d->signInCredentials.storingEncryptedTokens = false;
        d->signInCredentials.forcingCredentialsRefresh = false;
        d->signInCredentials.haveForcedCredentialsExpiry = false;

        connect(d->signInCredentials.identity, SIGNAL(error(SignOn::Error)),
                d, SLOT(handleCredentialsFailed(SignOn::Error)), Qt::UniqueConnection);

        d->setStatus(Account::SigningIn);
        d->handleCredentialsStored(identityId);
    } else {
        // password based authentication.
        QString identityUsername = parameters->username();
        if (!symmetricKey.isEmpty()) {
            // append the application name if we're encrypting.
            identityUsername += applicationName;
            identityUsername = b64_encrypted_string(identityUsername, symmetricKey);
        }
        if (identityUsername.isNull()) {
            //: Error emitted if encrypting username fails
            //% "Error occurred while encrypting username"
            emit signInError(qtTrId("sailfish_accounts-account-uname_encryption_failed"), Account::SignInUnknownError);
            return;
        }

        QString identitySecret = parameters->password();
        if (!symmetricKey.isEmpty()) {
            // only append the application name if we're encrypting.
            identitySecret += applicationName;
            identitySecret = b64_encrypted_string(identitySecret, symmetricKey);
        }
        if (identitySecret.isNull()) {
            //: Error emitted if encrypting password fails
            //% "Error occurred while encrypting password"
            emit signInError(qtTrId("sailfish_accounts-account-pword_encryption_failed"), Account::SignInUnknownError);
            return;
        }

        // we want to modify the username and secret.  To do so, we need to load the
        // identity info associated with the identity.  This is an asynchronous op.
        // First, set all of our parameters, then load the identity info.

        d->signInCredentials.identity = updateIdentity;
        d->signInCredentials.applicationName = applicationName;
        d->signInCredentials.symmetricKey = symmetricKey;
        d->signInCredentials.serviceName = parameters->serviceName();
        d->signInCredentials.method = parameters->method();
        d->signInCredentials.mechanism = parameters->mechanism();
        d->signInCredentials.sessionData = parameters->parameters();
        d->signInCredentials.credentialsName = credentialsName;
        d->signInCredentials.username = identityUsername;
        d->signInCredentials.password = identitySecret;
        d->signInCredentials.creatingSignInCredentials = false;
        d->signInCredentials.updatingSignInCredentials = true;
        d->signInCredentials.storingEncryptedTokens = true; // for password method, we store encrypted username/password immediately
        d->signInCredentials.forcingCredentialsRefresh = false;
        d->signInCredentials.haveForcedCredentialsExpiry = false;

        connect(d->signInCredentials.identity, SIGNAL(error(SignOn::Error)),
                d, SLOT(handleCredentialsFailed(SignOn::Error)), Qt::UniqueConnection);
        connect(d->signInCredentials.identity, SIGNAL(info(SignOn::IdentityInfo)),
                d, SLOT(handleCredentialsInfo(SignOn::IdentityInfo)), Qt::UniqueConnection);

        d->setStatus(Account::SigningIn);
        d->signInCredentials.identity->queryInfo();
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
    if (d->status != Account::Initialized && d->status != Account::Synced) {
        return;
    }

    if (applicationName.isEmpty()) {
        return;
    }

    if (!hasSignInCredentials(applicationName, credentialsName)) {
        return;
    }

    if (d->signInCredentials.creatingSignInCredentials
            || d->signInCredentials.updatingSignInCredentials
            || d->signInCredentials.signingInWithCredentials) {
        return;
    }

    // retrieve the identity (credentials) id specified
    QString credName = credentialsName.isEmpty() ? QLatin1String("default") : credentialsName;
    QString configurationValueKey = BUILD_CREDENTIALS_CONFIGURATION_KEY(applicationName, credName);
    quint32 identityId = d->configurationValues.value(configurationValueKey, QVariant::fromValue<int>(0)).toInt();

    // remove the key from our local map.
    d->configurationValues.remove(configurationValueKey);

    // remove the key from the account
    d->account->selectService(Accounts::Service());
    d->account->remove(configurationValueKey);

    // now check to see if any other applications are using the credentials
    // this is most likely for OAuth2 credentials, as they are segregated
    // internally in signond via the ClientId parameter.
    bool stillInUse = false;
    QStringList configKeys = d->configurationValues.keys();
    foreach (const QString &key, configKeys) {
        if (key.contains(CREDENTIALS_GROUP)) {
            quint32 keyval = d->configurationValues.value(key).toInt();
            if (identityId != 0 && keyval == identityId) {
                stillInUse = true;
                break;
            }
        }
    }

    if (!stillInUse) {
        // remove the identity from the database
        SignOn::Identity *removeIdentity = identityId == 0 ? NULL : SignOn::Identity::existingIdentity(identityId, this);
        if (removeIdentity != NULL) {
            removeIdentity->signOut();
            removeIdentity->remove();
        }

        // reset global service account default credentials if necessary
        if (identityId != 0 && d->account->credentialsId() == identityId) {
            d->account->setCredentialsId(0);
        }

        // reset specific service account default credentials if necessary
        Accounts::ServiceList supportedServices = d->account->services();
        for (int i = 0; i < supportedServices.size(); ++i) {
            const Accounts::Service &currService(supportedServices.at(i));
            d->account->selectService(currService);
            if (identityId != 0 && d->account->credentialsId() == identityId) {
                d->account->setCredentialsId(0);
            }
        }
        d->account->selectService(Accounts::Service());
    }

    // update the account in the db.
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
        emit signInError(qtTrId("sailfish_accounts-account-signin_invalid_status"), Account::SignInInvalidStatusError);
        return;
    }

    if (parameters == NULL) {
        //: Error emitted if function called with invalid parameters
        //% "Invalid sign-in parameters specified"
        emit signInError(qtTrId("sailfish_accounts-account-unpw_invalid_params"), Account::SignInMissingDataError);
        return;
    }

    if (applicationName.isEmpty()) {
        //: Error emitted if function called with invalid application name
        //% "Invalid application name specified"
        emit signInError(qtTrId("sailfish_accounts-account-signin_invalid_appname"), Account::SignInMissingDataError);
        return;
    }

    if (!hasSignInCredentials(applicationName, credentialsName)) {
        //: Error emitted if signon credentials do not exist
        //% "Named credentials do not exist for this application"
        emit signInError(qtTrId("sailfish_accounts-account-signin_not_exist"), Account::SignInInvalidCredentialsError);
        return;
    }

    QString credName = credentialsName.isEmpty() ? QLatin1String("default") : credentialsName;
    QString configurationValueKey = BUILD_CREDENTIALS_CONFIGURATION_KEY(applicationName, credName);
    int identityId = d->configurationValues.value(configurationValueKey, QVariant::fromValue<int>(0)).toInt();

    SignOn::Identity *signInIdentity = identityId == 0 ? NULL : SignOn::Identity::existingIdentity(identityId);
    if (signInIdentity == NULL) {
        //: Error emitted if signon credentials could not be loaded from the database
        //% "Credentials with id %1 could not be loaded"
        emit signInError(qtTrId("sailfish_accounts-account-load_credentials_error").arg(identityId), Account::SignInInvalidCredentialsError);
        return;
    }

    QVariantMap sipParams = parameters->parameters();
    int credPolicy = sipParams.value(QStringLiteral("CredentialsPolicy"),
                                     QVariant::fromValue<int>(SignInParameters::UseCachedCredentialsPolicy))
                                     .toInt();
    sipParams.remove(QStringLiteral("CredentialsPolicy"));

    d->signInCredentials.forcingCredentialsRefresh = credPolicy == SignInParameters::RefreshCredentialsPolicy;
    d->signInCredentials.haveForcedCredentialsExpiry = false;
    d->signInCredentials.signingInWithCredentials = true;

    d->signInCredentials.serviceName = parameters->serviceName();
    d->signInCredentials.method = parameters->method();
    d->signInCredentials.mechanism = parameters->mechanism();
    d->signInCredentials.sessionData = sipParams;
    d->signInCredentials.applicationName = applicationName;
    d->signInCredentials.symmetricKey = symmetricKey;
    d->signInCredentials.credentialsName = credName;

    d->signInCredentials.identity = signInIdentity;
    d->signInCredentials.session = signInIdentity->createSession(parameters->method());
    if (d->signInCredentials.session == NULL) {
        d->signInCredentials.cleanup();
        //: Error emitted if an error occurred while creating a signon session
        //% "Unable to create signon session with the specified parameters"
        emit signInError(qtTrId("sailfish_accounts-account-session_create_failed"), Account::SignInUnknownError);
        return;
    }

    connect(d->signInCredentials.session, SIGNAL(response(SignOn::SessionData)),
            d, SLOT(handleResponse(SignOn::SessionData)), Qt::UniqueConnection);
    connect(d->signInCredentials.session, SIGNAL(error(SignOn::Error)),
            d, SLOT(handleSignOnError(SignOn::Error)), Qt::UniqueConnection);
    connect(d->signInCredentials.session, SIGNAL(stateChanged(SignOn::AuthSession::AuthSessionState,QString)),
            d, SLOT(handleStateChanged(SignOn::AuthSession::AuthSessionState,QString)), Qt::UniqueConnection);

    d->setStatus(Account::SigningIn);
    d->signInCredentials.session->process(SignOn::SessionData(sipParams), parameters->mechanism());
}

/*!
    \qmlmethod Account::signOut(const QString &applicationName, const QString &credentialsName)

    Signs the account out of the service where it had previously been
    signed in using the credentials named the given \a credentialsName
    (or named "default" if no \a credentialsName is given).

    Client code should not call this method, as the account can
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

    SignOn::Identity *signInIdentity = identityId == 0 ? NULL : SignOn::Identity::existingIdentity(identityId, this);
    if (signInIdentity == NULL) {
        qWarning() << Q_FUNC_INFO << "credentials with id" << identityId << "could not be signed out";
        return;
    }

    signInIdentity->signOut();
}

/*!
    \qmlmethod Account::cancelSignInOperation()

    Cancels the sign-in operation started by a call to createSignInCredentials(),
    updateSignInCredentials() or signIn(). Upon cancellation, the signInError() is emitted with
    SignInOperationCanceledError.

    This function has no effect if none of these functions have been called, or if the sign-in
    operation started by the function has already finished (that is, if signInCredentialsCreated(),
    signInCredentialsUpdated(), signInResponse() or signInError() have already been emitted).
*/
void Account::cancelSignInOperation()
{
    if (d->status != Account::SigningIn) {
        qWarning("Account::cancelCredentialsCreation() called, but no sign-in operation in progress");
        return;
    }

    bool creatingAccount = d->signInCredentials.creatingSignInCredentials;
    d->signInCredentials.creatingSignInCredentials = false;
    d->signInCredentials.updatingSignInCredentials = false;
    d->signInCredentials.signingInWithCredentials = false;
    d->cancelCredentialsOperation(creatingAccount);
}

/*!
    \qmlproperty bool Account::enabled
    This property will be true if the account can be used, or false if it cannot.

    The account should be enabled if the details specified for it are valid.
    An account may need valid credentials associated with it before it can be
    enabled.

    Note: After changing this property, the changed will be reflected in the property value but
    will not actually take effect in the database until sync() is called.
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
    if (e == d->enabled)
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
    } else if (id != d->identifier
               && (d->status != Account::SigningIn && d->status != Account::SyncInProgress)) {
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

QString Account::defaultCredentialsUserName() const
{
    return d->defaultCredentialsUserName;

}

bool Account::provisioned() const
{
    return d && d->provisioned;
}

bool Account::readonly() const
{
    return d && d->readonly;
}

bool Account::limited() const
{
    return d && d->limited;
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
