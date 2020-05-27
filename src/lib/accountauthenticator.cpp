/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "accountauthenticator_p.h"
#include "globalaccountmanager_p.h"

#include <QtDebug>

#ifdef USE_SAILFISHKEYPROVIDER
#include <sailfishkeyprovider.h>
namespace {
    QString skp_storedKey(const QString &provider, const QString &service, const QString &key)
    {
        QString retn;
        char *value = NULL;
        int success = SailfishKeyProvider_storedKey(provider.toLatin1(), service.toLatin1(), key.toLatin1(), &value);
        if (value) {
            if (success == 0) {
                retn = QString::fromLatin1(value);
            }
            free(value);
        }
        return retn;
    }
}
#endif // USE_SAILFISHKEYPROVIDER


AccountAuthenticator::AccountAuthenticator(QObject *parent)
    : QObject(parent)
    , d(new AccountAuthenticatorPrivate(this))
{
}

void AccountAuthenticator::signIn(int accountId, const QString &serviceName)
{
    d->signIn(accountId, serviceName);
}

bool AccountAuthenticator::setCredentialsNeedUpdate(int accountId, const QString &serviceName)
{
    return d->setCredentialsNeedUpdate(accountId, serviceName);
}


AccountAuthenticatorPrivate::AccountAuthenticatorPrivate(AccountAuthenticator *parent)
    : QObject(parent)
    , q(parent)
{
}

AccountAuthenticatorPrivate::~AccountAuthenticatorPrivate()
{
    while (m_authData.size()) {
        AuthData authData = m_authData.takeFirst();
        authData.identity->destroySession(authData.authSession);
        authData.identity->deleteLater();
        authData.account->deleteLater();
    }
}

void AccountAuthenticatorPrivate::signIn(int accountId, const QString &serviceName)
{
    Accounts::Account *account = Accounts::Account::fromId(globalAccountManager(), accountId, this);
    if (!account) {
        emit q->signInError(accountId,
                            serviceName,
                            //% "Cannot load account for sign-in."
                            qtTrId("sailfishaccounts-la-account_load_error"));
        return;
    }

    // determine which service to sign in with.
    Accounts::Service srv;
    Accounts::ServiceList services = account->services();
    Q_FOREACH (const Accounts::Service &s, services) {
        if (s.name() == serviceName) {
            srv = s;
            break;
        }
    }

    if (!srv.isValid()) {
        account->deleteLater();

        //% "Cannot find '%1' service for this account."
        const QString errorString = qtTrId("sailfishaccounts-la-service_load_error").arg(serviceName);
        emit q->signInError(accountId, serviceName, errorString);
        return;
    }

    account->selectService(srv);
    if (!account->enabled()) {
        account->deleteLater();

        //% "Cannot authenticate, '%1' service is not enabled."
        const QString errorString = qtTrId("sailfishaccounts-la-service_not_enabled").arg(srv.name());
        emit q->signInError(accountId, serviceName, errorString);
        return;
    }

    const QStringList &serviceKeys = account->allKeys();
    QVariantMap serviceSettings;
    for (const QString &key: serviceKeys) {
        serviceSettings.insert(key, account->value(key));
    }

    const QString serverAddress = serviceSettings.value(QStringLiteral("server_address")).toString();
    if (serverAddress.isEmpty()) {
        account->deleteLater();

        //% "Server address %1 for '%2' service is invalid."
        const QString errorString = qtTrId("sailfishaccounts-la-invalid_url").arg(serverAddress).arg(serviceName);
        emit q->signInError(accountId, serviceName, errorString);
        return;
    }

    SignOn::Identity *ident = account->credentialsId() > 0 ? SignOn::Identity::existingIdentity(account->credentialsId()) : 0;
    if (!ident) {
        account->deleteLater();
        emit q->signInError(accountId,
                            serviceName,
                            //% "Cannot find valid sign-in credentials for this account."
                            qtTrId("sailfishaccounts-la-invalid_credentials"));
        return;
    }

    Accounts::AccountService accountService(account, srv);
    QString method = accountService.authData().method();
    QString mechanism = accountService.authData().mechanism();
    SignOn::AuthSession *session = ident->createSession(method);
    if (!session) {
        account->deleteLater();
        ident->deleteLater();
        emit q->signInError(accountId,
                            serviceName,
                            //% "Cannot create an authentication session for this account."
                            qtTrId("sailfishaccounts-la-auth_session_error"));
        return;
    }

    QString clientId;
    QString clientSecret;
    QString consumerKey;
    QString consumerSecret;
#ifdef USE_SAILFISHKEYPROVIDER
    QString providerName = account->providerName();
    clientId = skp_storedKey(providerName, QString(), QStringLiteral("client_id"));
    clientSecret = skp_storedKey(providerName, QString(), QStringLiteral("client_secret"));
    consumerKey = skp_storedKey(providerName, QString(), QStringLiteral("consumer_key"));
    consumerSecret = skp_storedKey(providerName, QString(), QStringLiteral("consumer_secret"));
#endif

    QVariantMap signonSessionData = accountService.authData().parameters();
    signonSessionData.insert("UiPolicy", SignOn::NoUserInteractionPolicy);
    if (!clientId.isEmpty()) signonSessionData.insert("ClientId", clientId);
    if (!clientSecret.isEmpty()) signonSessionData.insert("ClientSecret", clientSecret);
    if (!consumerKey.isEmpty()) signonSessionData.insert("ConsumerKey", consumerKey);
    if (!consumerSecret.isEmpty()) signonSessionData.insert("ConsumerSecret", consumerSecret);

    connect(session, &SignOn::AuthSession::response,
            this, &AccountAuthenticatorPrivate::signOnResponse,
            Qt::UniqueConnection);
    connect(session, &SignOn::AuthSession::error,
            this, &AccountAuthenticatorPrivate::signOnError,
            Qt::UniqueConnection);

    AuthData authData;
    authData.accountId = accountId;
    authData.serviceName = serviceName;
    authData.mechanism = mechanism;
    authData.signonSessionData = signonSessionData;
    authData.account = account;
    authData.identity = ident;
    authData.authSession = session;
    authData.credentials.serviceSettings = serviceSettings;
    m_authData.append(authData);

    session->setProperty("accountId", accountId);
    session->process(SignOn::SessionData(signonSessionData), mechanism);
}

void AccountAuthenticatorPrivate::signOnResponse(const SignOn::SessionData &response)
{
    QString username, password, accessToken;
    Q_FOREACH (const QString &key, response.propertyNames()) {
        if (key.toLower() == QStringLiteral("username")) {
            username = response.getProperty(key).toString();
        } else if (key.toLower() == QStringLiteral("secret")) {
            password = response.getProperty(key).toString();
        } else if (key.toLower() == QStringLiteral("password")) {
            password = response.getProperty(key).toString();
        } else if (key.toLower() == QStringLiteral("accesstoken")) {
            accessToken = response.getProperty(key).toString();
        }
    }

    const int accountId = sender()->property("accountId").toInt();
    const int authDataSize = m_authData.size();
    for (int i = 0; i < authDataSize; ++i) {
        if (m_authData[i].accountId == accountId) {
            AuthData authData = m_authData.takeAt(i);
            authData.identity->destroySession(authData.authSession);
            authData.identity->deleteLater();
            authData.account->deleteLater();

            // we need both username+password, OR accessToken.
            if (!accessToken.isEmpty()) {
                authData.credentials.accessToken = accessToken;
                emit q->signInCompleted(accountId, authData.serviceName, authData.credentials);
            } else if (!username.isEmpty() && !password.isEmpty()) {
                authData.credentials.username = username;
                authData.credentials.password = password;
                emit q->signInCompleted(accountId, authData.serviceName, authData.credentials);
            } else {
                emit q->signInError(accountId,
                                    authData.serviceName,
                                    //% "Cannot find valid sign-in credentials for the new account."
                                    qtTrId("sailfishaccounts-la-invalid_credentials_after_auth"));
            }
            return;
        }
    }

    emit q->signInError(accountId,
                        QString(),
                        //% "Cannot find authentication details for the new account."
                        qtTrId("sailfishaccounts-la-invalid_auth_session_on_sign_in"));
}

void AccountAuthenticatorPrivate::signOnError(const SignOn::Error &error)
{
    const int accountId = sender()->property("accountId").toInt();
    const int authDataSize = m_authData.size();
    for (int i = 0; i < authDataSize; ++i) {
        if (m_authData[i].accountId == accountId) {
            AuthData authData = m_authData.takeAt(i);

            // Signon failed due to credentials not existing in the database.
            // Set the CredentialsNeedUpdate key to force user to update credentials.
            if (error.type() == SignOn::Error::UserInteraction) {
                setCredentialsNeedUpdate(accountId, authData.serviceName);
            }

            authData.identity->destroySession(authData.authSession);
            authData.identity->deleteLater();
            authData.account->deleteLater();

            //% "Authentication error: %1"
            const QString errorString = qtTrId("sailfishaccounts-la-auth_error").arg(error.message());
            emit q->signInError(accountId, authData.serviceName, errorString);
            return;
        }
    }

    //% "Authentication error (for unknown service): %1"
    const QString errorString = qtTrId("sailfishaccounts-la-auth_error_no_service").arg(error.message());
    emit q->signInError(accountId, QString(), errorString);
}

bool AccountAuthenticatorPrivate::setCredentialsNeedUpdate(int accountId, const QString &serviceName)
{
    Accounts::Account *account = globalAccountManager()->account(accountId);
    if (account) {
        Accounts::Service service(globalAccountManager()->service(serviceName));
        if (service.isValid()) {
            account->selectService(service);
            account->setValue(QStringLiteral("CredentialsNeedUpdate"), QVariant::fromValue<bool>(true));
            account->setValue(QStringLiteral("CredentialsNeedUpdateFrom"), QVariant::fromValue<QString>(serviceName));
            account->selectService(Accounts::Service());
            account->syncAndBlock();
            return true;
        }
    }
    return false;
}
