#include "accountfactory_p.h"

#include "globalaccountmanager_p.h"
#include "account.h"
#include "signinparameters.h"

#include <QtDebug>

AccountFactory::AccountFactory(QObject *parent)
    : QObject(parent)
    , m_busy(false)
    , m_created(false)
    , m_resettingState(false)
    , m_am(globalAccountManager())
    , m_accountService(0)
    , m_sailfishAccount(0)
{
}

AccountFactory::~AccountFactory()
{
    resetState(AccountFactory::ResetOnly);
}

/*
    Creates an OAuth account (and associated credentials) with the provider
    identified by the given \a providerName.  It will use the signon parameters
    specified for the service identified by the given \a serviceName by
    default, overridden or extended by the parameters specified by \a params.

    If an error occurs, the \l error() signal will be emitted.  If the account
    is created successfully, the \l success() signal will be emitted.

    The account will be created with all services disabled.
*/
void AccountFactory::createOAuthAccount(const QString &providerName, const QString &serviceName, const QVariantMap &params,
                                        const QString &applicationName, const QString &credentialsName)
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

    m_method = m_accountService->authData().method();
    m_mechanism = m_accountService->authData().mechanism();
    m_applicationName = applicationName;
    m_credentialsName = credentialsName;

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

    If an error occurs, the \l error() signal will be emitted.  If the account
    is created successfully, the \l success() signal will be emitted.

    The account will be created with all services disabled.
*/
void AccountFactory::createAccount(const QString &providerName,
                                   const QString &serviceName,
                                   const QString &username,
                                   const QString &password,
                                   const QString &displayName,
                                   const QVariantMap &configuration,
                                   const QString &applicationName,
                                   const QString &symmetricKey,
                                   const QString &credentialsName)
{
    initializeAccountCreation(providerName, serviceName);

    foreach (const QString &serviceName, configuration.keys()) {
        QVariant v = configuration[serviceName];
        if (v.type() != QVariant::Map) {
            qWarning() << Q_FUNC_INFO << "Configuration for service" << serviceName << "is not a QVariantMap!";
            continue;
        }
        setConfigurationValues(v.toMap(), serviceName);
    }

    if (!displayName.isEmpty() && m_sailfishAccount && m_sailfishAccount->account())
        m_sailfishAccount->account()->setDisplayName(displayName);

    m_method = m_accountService->authData().method();
    m_mechanism = m_accountService->authData().mechanism();
    m_applicationName = applicationName;
    m_symmetricKey = symmetricKey;
    m_credentialsName = credentialsName;
    m_username = username;
    m_password = password;

    startAccountCreation();
}

void AccountFactory::setConfigurationValues(const QVariantMap &configurationValues, const QString &configurationServiceName)
{
    if (configurationValues.isEmpty() || !m_sailfishAccount || !m_sailfishAccount->account())
        return;

    Accounts::Service service;
    if (!configurationServiceName.isEmpty()) {
        service = m_am->service(configurationServiceName);
        if (service.isValid()) {
            m_sailfishAccount->account()->selectService(service);
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
            m_sailfishAccount->account()->setValue(key, currValue);
        } else if (currValue.type() == QVariant::List) {
            m_sailfishAccount->account()->setValue(key, currValue.toStringList());
        } else {
            qWarning() << Q_FUNC_INFO << "Unsupported configuration value type!  Must be int, quint64, bool, string or string list.";
        }
    }
    if (service.isValid()) {
        m_sailfishAccount->account()->selectService(Accounts::Service());
    }
}

void AccountFactory::initializeAccountCreation(const QString &providerName, const QString &serviceName)
{
    // Create the account and service account so that we can retrieve the sign on parameters.
    // Then we construct a Sailfish Account (segregated credentials).

    // could be creating an account already
    if (m_busy) {
        //: Error emitted if the function is called while creation is in progress
        //% "Cannot create account - already busy!"
        emit error(qtTrId("jollacomponents_internal-accountfactory-already_busy"));
        return;
    }

    // create the account, trigger credentials creation
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

    Accounts::Account *newAccount = m_am->createAccount(providerName);
    if (!newAccount) {
        //: Error emitted if an error occurred while creating an account for the given providerName
        //% "Could not create account with provider %1"
        emit error(qtTrId("jollacomponents_internal-accountfactory-account_create_failed").arg(providerName));
        return;
    }

    m_accountService = new Accounts::AccountService(newAccount, m_srv);
    if (!m_accountService) {
        resetState(AccountFactory::CleanupArtifacts);
        //: Error emitted if an error occurred while creating a service account
        //% "Could not create service account for service %1"
        emit error(qtTrId("jollacomponents_internal-accountfactory-service_account_create_failed").arg(serviceName));
        return;
    }

    // now attempt to create credentials.
    Accounts::Provider prv = m_am->provider(providerName);
    QString method = m_accountService->authData().method();
    QString mechanism = m_accountService->authData().mechanism();
    QVariantMap sessionData = m_accountService->authData().parameters();

    m_sailfishAccount = new Account(false, newAccount, this); // false = don't query info.
    if (!m_sailfishAccount) {
        resetState(AccountFactory::CleanupArtifacts);
        //: Error emitted if an error occurred while creating a Sailfish (segregated credentials) account
        //% "Could not create account credentials for service %1"
        emit error(qtTrId("jollacomponents_internal-accountfactory-sailfish_account_create_failed").arg(serviceName));
        return;
    }
}

void AccountFactory::startAccountCreation()
{
    if (!m_sailfishAccount) {
        qWarning() << Q_FUNC_INFO << "no sailfish account created - aborting credentials creation";
        return; // error - should have already emitted.
    }

    connect(m_sailfishAccount, SIGNAL(signInCredentialsCreated(QVariantMap)),
            this, SLOT(handleSignInCredentialsCreated(QVariantMap)));
    connect(m_sailfishAccount, SIGNAL(signInError(QString)),
            this, SLOT(handleSignInError(QString)));

    SignInParameters *params = new SignInParameters(m_method, m_mechanism, m_signonSessionParams, m_username, m_password, m_sailfishAccount);
    if (m_method.toLower().startsWith(QLatin1String("oauth"))) {
        m_sailfishAccount->createSignInCredentials(
                                m_applicationName,
                                m_credentialsName,
                                params);
    } else {
        m_sailfishAccount->createSignInCredentials(
                                m_applicationName,
                                m_credentialsName,
                                params,
                                m_symmetricKey);
    }
}

void AccountFactory::cancel()
{
    resetState(AccountFactory::CleanupArtifacts);
}

void AccountFactory::handleSignInError(const QString &message)
{
    QString providerName = m_sailfishAccount->account()->providerName();
    resetState(AccountFactory::CleanupArtifacts);
    //: Error emitted if account creation failed at the database level
    //% "Unable to save %1 account in database: %2"
    emit error(qtTrId("jollacomponents_internal-accountfactory-account_database").arg(providerName).arg(message));
}

void AccountFactory::handleSignInCredentialsCreated(const QVariantMap &responseData)
{
    int newAccountId = m_sailfishAccount->account()->id();

    // now enable the account.
    Accounts::ServiceList supportedServices = m_sailfishAccount->account()->services();
    for (int i = 0; i < supportedServices.size(); ++i) {
        m_sailfishAccount->account()->selectService(supportedServices.at(i));
        m_sailfishAccount->account()->setEnabled(true);
    }
    m_sailfishAccount->account()->selectService(Accounts::Service());
    m_sailfishAccount->account()->setEnabled(true);

    resetState(AccountFactory::ResetOnly);
    emit success(newAccountId, responseData);
}

void AccountFactory::resetState(AccountFactory::ResetMode mode)
{
    if (!m_resettingState) {
        m_resettingState = true;
        if (m_sailfishAccount) {
            if (mode == AccountFactory::CleanupArtifacts) {
                m_sailfishAccount->remove();
            }
            m_sailfishAccount->deleteLater();
        }
        if (m_accountService) {
            m_accountService->deleteLater();
        }
        m_sailfishAccount = 0;
        m_accountService = 0;
        m_responseData = QVariantMap();
        m_signonSessionParams = QVariantMap();
        m_srv = Accounts::Service();
        m_method = QString();
        m_mechanism = QString();
        m_applicationName = QString();
        m_symmetricKey = QString();
        m_credentialsName = QString();
        m_username = QString();
        m_password = QString();
        m_busy = false;
        m_created = false;
    }
}
