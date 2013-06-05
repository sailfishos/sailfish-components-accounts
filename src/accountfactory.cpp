#include "accountfactory_p.h"

#include <QtDebug>

AccountFactory::AccountFactory(QObject *parent)
    : QObject(parent)
    , m_busy(false)
    , m_created(false)
    , m_resettingState(false)
    , m_am(new Accounts::Manager(this))
    , m_newAccount(0)
    , m_accountService(0)
    , m_ident(0)
    , m_session(0)
{
}

AccountFactory::~AccountFactory()
{
    resetState(AccountFactory::ResetOnly);
    m_am->deleteLater();
}

/*
    Creates an OAuth account (and associated credentials) with the provider
    identified by the given \a providerName.  It will use the signon parameters
    specified for the service identified by the given \a serviceName by
    default, overridden or extended by the parameters specified by \a params.

    If an error occurs, the \l error() signal will be emitted.

    If signon is successful, the account will be created and the identity used
    during signon will be set as the credentials for all services offered by
    the provider in that account.  The auth session will remain signed in until
    \l signOut() method is invoked.

    The account will be created with all services disabled.
*/
void AccountFactory::createOAuthAccount(const QString &providerName, const QString &serviceName, const QVariantMap &params)
{
    initializeAccountCreation(providerName, serviceName);

    QVariantMap adp = m_accountService->authData().parameters();
    QStringList paramKeys = params.keys();
    foreach (const QString &key, paramKeys) {
        QVariant pv = params.value(key);
        if (pv.type() == QVariant::List) {
            adp.insert(key, pv.toStringList());
        } else {
            adp.insert(key, pv);
        }
    }
    m_signonSessionParams = adp;

    startAccountCreation();
}

/*
    Creates an account (and associated credentials) with the provider
    identified by the given \a providerName.  It will use the signon parameters
    specified for the service identified by the given \a serviceName by
    default, overridden or extended by the parameters specified by \a params.
    It will also set \a displayName as the account display name, if specified.

    If \a configuration is specified, it should be a set of key-value parameters
    where each key is an account service name and each value is a QVariantMap of
    the configuration values to be set for that service. If a service name is
    empty, then its values will be set as the account's global configuration
    values.

    If an error occurs, the \l error() signal will be emitted.

    If signon is successful, the account will be created and the identity used
    during signon will be set as the credentials for all services offered by
    the provider in that account.  The auth session will remain signed in until
    \l signOut() method is invoked.

    The account will be created with all services disabled.
*/
void AccountFactory::createAccount(const QString &providerName,
                                   const QString &serviceName,
                                   const QString &username,
                                   const QString &password,
                                   const QString displayName,
                                   const QVariantMap &configuration)
{
    initializeAccountCreation(providerName, serviceName);

    m_identInfo.setUserName(username);
    m_identInfo.setSecret(password);

    foreach (const QString &serviceName, configuration.keys()) {
        QVariant v = configuration[serviceName];
        if (v.type() != QVariant::Map) {
            qWarning() << Q_FUNC_INFO << "Configuration for service" << serviceName << "is not a QVariantMap!";
            continue;
        }
        setConfigurationValues(v.toMap(), serviceName);
    }

    if (!displayName.isEmpty() && m_newAccount)
        m_newAccount->setDisplayName(displayName);

    startAccountCreation();
}

void AccountFactory::setConfigurationValues(const QVariantMap &configurationValues, const QString &configurationServiceName)
{
    if (configurationValues.isEmpty())
        return;

    Accounts::Service service;
    if (!configurationServiceName.isEmpty()) {
        service = m_am->service(configurationServiceName);
        if (service.isValid()) {
            m_newAccount->selectService(service);
        } else {
            qWarning() << Q_FUNC_INFO << "Unable to find service" << configurationServiceName << ", not setting account configuration values";
            return;
        }
    }
    foreach (const QString &key, configurationValues.keys()) {
        QVariant currValue = configurationValues.value(key);
        if (currValue.type() == QVariant::Bool
                || currValue.type() == QVariant::Int
                || currValue.type() == QVariant::LongLong
                || currValue.type() == QVariant::ULongLong
                || currValue.type() == QVariant::String
                || currValue.type() == QVariant::StringList) {
            m_newAccount->setValue(key, currValue);
        } else if (currValue.type() == QVariant::List) {
            m_newAccount->setValue(key, currValue.toStringList());
        } else {
            qWarning() << Q_FUNC_INFO << "Unsupported configuration value type!  Must be int, quint64, bool, string or string list.";
        }
    }
    if (service.isValid())
        m_newAccount->selectService(Accounts::Service());
}

void AccountFactory::initializeAccountCreation(const QString &providerName, const QString &serviceName)
{
    // we still have to create the account first, then the service account, then the identity
    // simply because the accounts framework doesn't allow us to access parameters from the
    // service itself :-/

    // could be creating an account already
    if (m_busy) {
        //: Error emitted if the function is called while creation is in progress
        //% "Cannot create account - already busy!"
        emit error(qtTrId("jollacomponents_internal-accountfactory-already_busy"));
        return;
    }

    // could have created an account, but not signed out after success()
    if (m_ident || m_session) {
        //: Error emitted if the function is called while signed in to previously created account
        //% "Cannot create account - still signed in to previously created account!"
        emit error(qtTrId("jollacomponents_internal-accountfactory-still_signed_in"));
        return;
    }

    // create the account, create a new identity, sign in.
    m_busy = true;            // we're busy creating an account until it succeeds or fails.
    m_created = false;        // haven't yet successfully created the account.
    m_resettingState = false; // and we're not in the middle of resetting our state.
    m_srv = m_am->service(serviceName);
    if (!m_srv.isValid()) {
        //: Error emitted if the given serviceName isn't valid
        //% "Not a valid signon service: %1"
        emit error(qtTrId("jollacomponents_internal-accountfactory-invalid_service").arg(serviceName));
        return;
    }

    m_newAccount = m_am->createAccount(providerName);
    if (!m_newAccount) {
        //: Error emitted if an error occurred while creating an account for the given providerName
        //% "Could not create account with provider %1"
        emit error(qtTrId("jollacomponents_internal-accountfactory-account_create_failed").arg(providerName));
        return;
    }

    m_accountService = new Accounts::AccountService(m_newAccount, m_srv);
    if (!m_accountService) {
        resetState(AccountFactory::CleanupArtifacts);
        //: Error emitted if an error occurred while creating a service account
        //% "Could not create service account for service %1"
        emit error(qtTrId("jollacomponents_internal-accountfactory-service_account_create_failed").arg(serviceName));
        return;
    }

    // now create the identity and attempt to store credentials.  Once that's complete, we'll sign in.
    Accounts::Provider prv = m_am->provider(providerName);
    QMap<QString, QStringList> methodMechs;
    methodMechs.insert(m_accountService->authData().method(), QStringList() << m_accountService->authData().mechanism());
    m_identInfo = SignOn::IdentityInfo(prv.displayName(), prv.displayName(), methodMechs);
}

void AccountFactory::startAccountCreation()
{
    m_ident = SignOn::Identity::newIdentity(m_identInfo);
    if (!m_ident) {
        resetState(AccountFactory::CleanupArtifacts);
        //: Error emitted if an error occurred while creating an identity (signon credentials)
        //% "Unable to create credentials to sign in to service %1 from provider %2"
        emit error(qtTrId("jollacomponents_internal-accountfactory-credentials_create_failed").arg(m_srv.name()).arg(m_newAccount->providerName()));
        return;
    }

    connect(m_ident, SIGNAL(error(SignOn::Error)), this, SLOT(handleCredentialsFailed(SignOn::Error)));
    connect(m_ident, SIGNAL(credentialsStored(quint32)), this, SLOT(handleCredentialsStored(quint32)));

    m_ident->storeCredentials(m_identInfo);
}

void AccountFactory::signOut()
{
    if (!m_busy) {
        resetState(AccountFactory::ResetOnly); // reset state, but don't remove the account/identity.
    } else {
        //: Error emitted if the function is called while busy
        //% "Unable to sign out - still creating account or setting name"
        emit error(qtTrId("jollacomponents_internal-accountfactory-signout_busy"));
    }
}

void AccountFactory::cancel()
{
    resetState(AccountFactory::CleanupArtifacts);
}

void AccountFactory::handleCredentialsStored(quint32 id)
{
    if (m_ident->id() != id) {
        resetState(AccountFactory::CleanupArtifacts);
        //: Error emitted if an error occurred while storing credentials
        //% "Unable to store credentials - invalid id"
        emit error(qtTrId("jollacomponents_internal-accountfactory-identity_id_failed"));
        return;
    }

    m_session = m_ident->createSession(m_accountService->authData().method());
    if (!m_session) {
        resetState(AccountFactory::CleanupArtifacts);
        //: Error emitted if an error occurred while creating a signon session
        //% "Unable to create signon session"
        emit error(qtTrId("jollacomponents_internal-accountfactory-session_create_failed"));
        return;
    }

    connect(m_session, SIGNAL(response(SignOn::SessionData)), this, SLOT(handleResponse(SignOn::SessionData)));
    connect(m_session, SIGNAL(error(SignOn::Error)), this, SLOT(handleSignOnError(SignOn::Error)));

    m_session->process(SignOn::SessionData(m_signonSessionParams), m_accountService->authData().mechanism());
    emit startedSignon();
}

void AccountFactory::handleResponse(const SignOn::SessionData &data)
{
    emit finishedSignon();

    if (m_busy && !m_created) {
        // first, cache the response data.
        m_responseData.clear();
        QStringList keys = data.propertyNames();
        foreach (const QString &key, keys) {
            m_responseData.insert(key, data.getProperty(key));
        }

        // then sync the account.  First, set the credentials for all services from the provider.
        Accounts::ServiceList srvList = m_am->serviceList();
        foreach (const Accounts::Service &srv, srvList) {
            if (srv.provider() == m_newAccount->providerName()) {
                m_newAccount->selectService(srv);
                m_newAccount->setCredentialsId(m_ident->id());
                m_newAccount->setEnabled(false);
                m_newAccount->selectService(Accounts::Service());
            }
        }
        m_newAccount->setCredentialsId(m_ident->id()); // set credentials for global service

        connect(m_newAccount, SIGNAL(synced()), this, SLOT(handleSynced()), Qt::UniqueConnection);
        connect(m_newAccount, SIGNAL(error(Accounts::Error)), this, SLOT(handleAccountError()), Qt::UniqueConnection);

        m_newAccount->sync();
    }
}

void AccountFactory::handleSynced()
{
    if (m_busy && !m_created) {
        // successfully created a new account and stored the credentials.
        m_busy = false;
        m_created = true;
        emit success(m_newAccount->id(), m_ident->id(), m_responseData);
    }
}

void AccountFactory::handleCredentialsFailed(const SignOn::Error &err)
{
    QString providerName = m_newAccount->providerName();
    resetState(AccountFactory::CleanupArtifacts);
    //: Error emitted if account credentials creation failed at the database level
    //% "Unable to save credentials for %1 account in database: %2"
    emit error(qtTrId("jollacomponents_internal-accountfactory-credentials_database").arg(providerName).arg(err.message()));
}

void AccountFactory::handleSignOnError(const SignOn::Error &err)
{
    resetState(AccountFactory::CleanupArtifacts);
    QString errMess = err.message();
    if (errMess == QLatin1String("userActionFinished error: 5")) {
        //: Error emitted if signon failed due to network connection failure
        //% "Network connection failure"
        emit error(qtTrId("jollacomponents_internal-accountfactory-network_failure"));
    } else {
        emit error(errMess);
    }
}

void AccountFactory::handleAccountError()
{
    QString providerName = m_newAccount->providerName();
    resetState(AccountFactory::CleanupArtifacts);
    //: Error emitted if account creation failed at the database level
    //% "Unable to save %1 account in database"
    emit error(qtTrId("jollacomponents_internal-accountfactory-account_database").arg(providerName));
}

void AccountFactory::resetState(AccountFactory::ResetMode mode)
{
    if (!m_resettingState) {
        m_resettingState = true;
        if (m_ident) {
            if (m_session) {
                m_ident->destroySession(m_session);
            }
            if (mode == AccountFactory::CleanupArtifacts) {
                m_ident->signOut();
                m_ident->remove();
            }
            m_ident->deleteLater();
        }
        if (m_newAccount) {
            if (mode == AccountFactory::CleanupArtifacts) {
                m_newAccount->remove();
            }
            m_newAccount->deleteLater();
        }
        if (m_accountService) {
            m_accountService->deleteLater();
        }
        m_newAccount = 0;
        m_ident = 0;
        m_accountService = 0;
        m_session = 0;
        m_responseData = QVariantMap();
        m_signonSessionParams = QVariantMap();
        m_srv = Accounts::Service();
        m_busy = false;
        m_created = false;
    }
}
