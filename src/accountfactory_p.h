#ifndef ACCOUNTFACTORY_P_H
#define ACCOUNTFACTORY_P_H

#include <QtCore/QObject>
#include <QtCore/QVariantMap>
#include <QtCore/QString>

#include <Accounts/Manager>
#include <Accounts/Account>
#include <Accounts/AccountService>
#include <Accounts/Service>
#include <Accounts/AuthData>

class Account;

class AccountFactory : public QObject
{
    Q_OBJECT

public:
    AccountFactory(QObject *parent = 0);
    ~AccountFactory();

    Q_INVOKABLE void createAccount(const QString &providerName, const QString &serviceName,
                                   const QString &username, const QString &password,
                                   const QString &displayName, const QVariantMap &configuration,
                                   const QString &applicationName, const QString &symmetricKey,
                                   const QString &credentialsName = QString());
    Q_INVOKABLE void createOAuthAccount(const QString &providerName, const QString &serviceName, const QVariantMap &params,
                                        const QString &applicationName, const QString &symmetricKey,
                                        const QString &credentialsName);

    Q_INVOKABLE void cancel();

Q_SIGNALS:
    void error(const QString &message);
    void success(int newAccountId, const QVariantMap &responseData);

private Q_SLOTS:
    void handleSignInCredentialsCreated(const QVariantMap &responseData);
    void handleSignInError(const QString &message);

private:
    enum ResetMode {
        ResetOnly = 0,
        CleanupArtifacts
    };
    void resetState(ResetMode mode);
    void initializeAccountCreation(const QString &providerName, const QString &serviceName);
    void startAccountCreation();
    void setConfigurationValues(const QVariantMap &configurationValues, const QString &configurationServiceName);

private:
    bool m_busy;
    bool m_created;
    bool m_resettingState;
    Accounts::Service m_srv;
    QVariantMap m_responseData;
    QVariantMap m_signonSessionParams;
    Accounts::Manager *m_am;
    Accounts::AccountService *m_accountService;
    Account *m_sailfishAccount;

    QString m_method;
    QString m_mechanism;
    QString m_applicationName;
    QString m_symmetricKey;
    QString m_credentialsName;
    QString m_username;
    QString m_password;
};

#endif // ACCOUNTFACTORY_P_H
