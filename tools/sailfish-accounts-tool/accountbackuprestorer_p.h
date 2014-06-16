/*
** Copyright (C) 2014 Jolla Ltd.
*/

#ifndef ACCOUNTBACKUPRESTORER_P_H
#define ACCOUNTBACKUPRESTORER_P_H

#include <QObject>
#include <QVariantMap>
#include <QString>
#include <QMap>
#include <QList>
#include <QPair>
#include <QSettings>
#include <QMutex>

#include <Accounts/Account>
#include <Accounts/AccountService>
#include <Accounts/Service>
#include <Accounts/Manager>
#include <Accounts/AuthData>
#include <SignOn/Identity>
#include <SignOn/SessionData>
#include <SignOn/AuthSession>

class AccountSyncManager;

class AccountBackupRestorer : public QObject
{
    Q_OBJECT

public:
    AccountBackupRestorer(AccountSyncManager *syncManager, Accounts::Manager *accountManager, QObject *parent = 0);
    ~AccountBackupRestorer();

    bool backupAccount(Accounts::Account *account, const QString &backupFile);
    bool restoreAccounts(const QString &backupFile);

private:
    struct SignOnCredentials {
        quint32 id;
        QString method;
        QString mechanism;
        QVariantMap sessionData;
    };

    void backupAccountServiceSettings(QSettings &backupIni,
                                      const Accounts::ServiceList &accountServices,
                                      const Accounts::Service &srv,
                                      Accounts::Account *account,
                                      QList<SignOnCredentials> &requiredCredentials,
                                      const QString &clientId,
                                      const QString &clientSecret,
                                      const QString &consumerKey,
                                      const QString &consumerSecret);

    void restoreAccountServiceSettings(QSettings &backupIni,
                                       const Accounts::Service &srv,
                                       Accounts::Account *account,
                                       const QString &oldAccountId,
                                       const QMap<quint32, quint32> &oldToNewCredentialsIds,
                                       QMap<QString, bool> *servicesEnabled);

    QMap<quint32, quint32> createCredentials(const QVariantMap &allCredentialsSettings);

    AccountSyncManager *m_syncManager;
    Accounts::Manager *m_accountManager;
};

class CredentialKeysQuery : public QObject
{
    Q_OBJECT

public:
    CredentialKeysQuery(int credentialsId, const QString &method, const QString &mechanism, const QVariantMap &sessionData, QObject *parent = 0);
    ~CredentialKeysQuery();

    bool finished() const;
    bool error() const;
    QVariantMap infoValues() const;
    QVariantMap methodMechanismSecrets() const;

public Q_SLOTS:
    void queryCredentials();

private Q_SLOTS:
    void credentialsInfo(const SignOn::IdentityInfo &info);
    void signOnResponse(const SignOn::SessionData &responseData);
    void signOnError(const SignOn::Error &error);

private:
    bool m_finished;
    bool m_error;
    quint32 m_credentialsId;
    QString m_method;
    QString m_mechanism;
    QVariantMap m_sessionData;
    QVariantMap m_infoValues;
    QVariantMap m_methodMechanismSecrets;
    mutable QMutex m_mutex;
    SignOn::Identity *m_identity;
};

class CredentialCreationRequest : public QObject
{
    Q_OBJECT

public:
    CredentialCreationRequest(const QVariantMap &infoValues, const QVariantMap &methodMechSecrets, QObject *parent = 0);
    ~CredentialCreationRequest();

    bool finished() const;
    bool error() const;
    quint32 newCredentialsId() const;

public Q_SLOTS:
    void createCredentials();

private Q_SLOTS:
    void credentialsStored(quint32 id);
    void credentialsError(const SignOn::Error &error);
    void signOnResponse(const SignOn::SessionData &responseData);
    void signOnError(const SignOn::Error &error);

private:
    void storeProvidedTokens();
    bool m_finished;
    bool m_error;
    quint32 m_credentialsId;
    QList<QPair<QVariantMap, QVariantMap> > m_providedTokensQueue; // sessionData,providedTokens
    QVariantMap m_infoValues;
    QVariantMap m_methodMechSecrets;
    mutable QMutex m_mutex;
    SignOn::Identity *m_identity;
};

#endif
