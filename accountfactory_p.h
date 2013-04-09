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

#include <SignOn/Identity>
#include <SignOn/AuthSession>
#include <SignOn/SessionData>
#include <SignOn/Error>

class AccountFactory : public QObject
{
    Q_OBJECT

public:
    AccountFactory(QObject *parent = 0);
    ~AccountFactory();

    Q_INVOKABLE void createOAuthAccount(const QString &providerName, const QString &serviceName, const QVariantMap &params);
    Q_INVOKABLE void setAccountDisplayName(const QString &displayName);
    Q_INVOKABLE void signOut();
    Q_INVOKABLE void cancel();

Q_SIGNALS:
    void error(const QString &message);
    void success(int newAccountId, int newIdentityId, const QVariantMap &responseData);

private Q_SLOTS:
    void handleResponse(const SignOn::SessionData &data);
    void handleSynced();
    void handleSignOnError(const SignOn::Error &err);
    void handleAccountError();

private:
    enum ResetMode {
        ResetOnly = 0,
        CleanupArtifacts
    };
    void resetState(ResetMode mode);

private:
    bool m_busy;
    bool m_created;
    bool m_settingName;
    Accounts::Service m_srv;
    QVariantMap m_responseData;
    Accounts::Manager *m_am;
    Accounts::Account *m_newAccount;
    Accounts::AccountService *m_accountService;
    SignOn::Identity *m_ident;
    SignOn::AuthSession *m_session;
};

#endif // ACCOUNTFACTORY_P_H
