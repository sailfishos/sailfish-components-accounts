/*
 * Copyright (C) 2013 Jolla Ltd.
 * Contact: Chris Adams <chris.adams@jollamobile.com>
 *
 * License: Proprietary
 */

#ifndef JOLLAACCOUNTPROVIDER_P_H
#define JOLLAACCOUNTPROVIDER_P_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtNetwork/QNetworkReply>

class QNetworkAccessManager;

class JollaAccountProvider : public QObject
{
    Q_OBJECT

public:
    JollaAccountProvider(QObject *parent = 0);
    ~JollaAccountProvider();

    // registers new application and new user
    Q_INVOKABLE void registerUserAccount(const QString &username,
                                         const QString &password,
                                         const QString &email,
                                         const QString &firstName,
                                         const QString &lastName,
                                         const QString &countryCode,
                                         const QString &city,
                                         const QString &street,
                                         const QString &postCode);

    // registers new application and logs in existing user
    Q_INVOKABLE void registerExistingAccount(const QString &username,
                                             const QString &password);

Q_SIGNALS:
    void registerUserAccountSucceeded(const QVariantMap &responseData);
    void registerUserAccountFailed(const QString &errorMessage);
    void registerExistingAccountSucceeded(const QVariantMap &responseData);
    void registerExistingAccountFailed(const QString &errorMessage);

private:
    enum KeyResult {
        KeySuccess = 0,
        KeyFailure = 1,
        KeyProviderError = 2
    };
    KeyResult checkApplicationKeys();

    void registerApplication(const QString &username, const QString &password);
    void loginUserAccount(const QString &username, const QString &password, const QVariantMap &tokens);

private Q_SLOTS:
    void registerUserFinished();
    void registerUserError(const QNetworkReply::NetworkError &err);
    void registerApplicationFinished();
    void registerApplicationError(const QNetworkReply::NetworkError &err);
    void loginUserAccountFinished();
    void loginUserAccountError(const QNetworkReply::NetworkError &err);

private:
    QNetworkAccessManager *m_qnam;
};

#endif // JOLLAACCOUNTPROVIDER_P_H
