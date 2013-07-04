/*
 * Copyright (C) 2013 Jolla Ltd.
 * Contact: Chris Adams <chris.adams@jollamobile.com>
 *
 * License: Proprietary
 */

#include "jollaaccountprovider_p.h"

//libsailfishkeyprovider
#include <sailfishkeyprovider.h>

#include <QMetaObject>
#include <QMetaEnum>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslConfiguration>
#include <QSsl>
#include <QUrl>
#include <QUuid>

#include <QtDebug>
#include <QJsonDocument>

#define JOLLA_ACCOUNT_PROVIDER_NAME     "jolla"
#define JOLLA_ACCOUNT_SERVICE_NAME      "jolla-store"
#define JOLLA_ACCOUNT_ENCODING_SCHEME   "xor"
#define JOLLA_ACCOUNT_ENCODING_KEY      "xor_key" // not a secret key.
#define JOLLA_ACCOUNT_KEY_APP_NAME      "application_name"
#define JOLLA_ACCOUNT_KEY_APP_ID        "application_id"
#define JOLLA_ACCOUNT_KEY_CLIENT_ID     "client_id"
#define JOLLA_ACCOUNT_KEY_CLIENT_SECRET "client_secret"

QVariantMap fromJsonBlob(const QByteArray &replyData, bool *ok)
{
    QVariant parsed;

    QJsonDocument jsonDocument = QJsonDocument::fromJson(replyData);
    *ok = !jsonDocument.isEmpty();
    parsed = jsonDocument.toVariant();

    if (*ok && parsed.type() == QVariant::Map) {
        return parsed.toMap();
    }
    *ok = false;
    return QVariantMap();
}

QByteArray toJsonBlob(const QVariantMap &data, bool *ok)
{
    QByteArray blob;

    QJsonDocument jsonDocument = QJsonDocument::fromVariant(data);
    *ok = !jsonDocument.isNull();
    blob = jsonDocument.toJson();

    return blob;
}

QString networkErrorString(QNetworkReply::NetworkError err)
{
    QMetaObject meta = QNetworkReply::staticMetaObject;
    QMetaEnum networkErrorEnum = meta.enumerator(meta.indexOfEnumerator("NetworkError"));
    return QLatin1String("QNetworkReply::") + QLatin1String(networkErrorEnum.valueToKey(err));
}

bool storeEncodedValue(const QString &which, const QString &value)
{
    char *encodedValue = NULL;
    int success = SailfishKeyProvider_encodeKey(
                                value.toUtf8().constData(),
                                JOLLA_ACCOUNT_ENCODING_SCHEME,
                                JOLLA_ACCOUNT_ENCODING_KEY,
                                &encodedValue);
    if (success == -1 || encodedValue == NULL) {
        return false;
    }

    success = SailfishKeyProvider_storeKey(
                                JOLLA_ACCOUNT_PROVIDER_NAME,
                                JOLLA_ACCOUNT_SERVICE_NAME,
                                which.toLatin1().constData(), // the key in the ini settings file
                                encodedValue,
                                JOLLA_ACCOUNT_ENCODING_SCHEME,
                                JOLLA_ACCOUNT_ENCODING_KEY);

    free(encodedValue);
    if (success == -1) {
        return false;
    }

    return true;
}

bool storeApplicationKeys(const QString &appName, const QString &clientId, const QString &clientSecret)
{
    if (!storeEncodedValue(JOLLA_ACCOUNT_KEY_APP_NAME, appName)) {
        return false;
    }

    if (!storeEncodedValue(JOLLA_ACCOUNT_KEY_CLIENT_ID, clientId)) {
        return false;
    }

    if (!storeEncodedValue(JOLLA_ACCOUNT_KEY_CLIENT_SECRET, clientSecret)) {
        return false;
    }

    return true;
}

QNetworkRequest createTlsRequest(const QUrl &url)
{
    QNetworkRequest request(url);
    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    config.setProtocol(QSsl::TlsV1SslV3);
    request.setSslConfiguration(config);
    return request;
}

//------------------------------------------

JollaAccountProvider::JollaAccountProvider(QObject *parent)
    : QObject(parent)
    , m_qnam(new QNetworkAccessManager)
{
}

JollaAccountProvider::~JollaAccountProvider()
{
    delete m_qnam;
}

JollaAccountProvider::KeyResult JollaAccountProvider::checkApplicationKeys()
{
    // libsailfishkeyprovider.
    // we expect: application name, application id, client_id, client_secret.
    // If the keys exist, return success.  If they don't return failure.
    // If the keyprovider aborts / returns zero, return error.

    char *cAppName = NULL;
    char *cClientId = NULL;
    char *cClientSecret = NULL;

    int canSuccess = SailfishKeyProvider_storedKey(
                                JOLLA_ACCOUNT_PROVIDER_NAME,
                                JOLLA_ACCOUNT_SERVICE_NAME,
                                JOLLA_ACCOUNT_KEY_APP_NAME,
                                &cAppName);

    int cciSuccess = SailfishKeyProvider_storedKey(
                                JOLLA_ACCOUNT_PROVIDER_NAME,
                                JOLLA_ACCOUNT_SERVICE_NAME,
                                JOLLA_ACCOUNT_KEY_CLIENT_ID,
                                &cClientId);

    int ccsSuccess = SailfishKeyProvider_storedKey(
                                JOLLA_ACCOUNT_PROVIDER_NAME,
                                JOLLA_ACCOUNT_SERVICE_NAME,
                                JOLLA_ACCOUNT_KEY_CLIENT_SECRET,
                                &cClientSecret);

    free(cAppName);
    free(cClientId);
    free(cClientSecret);

    if (canSuccess == -1 || cciSuccess == -1 || ccsSuccess == -1) {
        return JollaAccountProvider::KeyProviderError;
    } else if (canSuccess == 1 || cciSuccess == 1 || ccsSuccess == 1) {
        return JollaAccountProvider::KeyFailure;
    }

    return JollaAccountProvider::KeySuccess;
}

void JollaAccountProvider::registerUserAccount(
                                const QString &username,
                                const QString &password,
                                const QString &email,
                                const QString &firstName,
                                const QString &lastName,
                                const QString &countryCode,
                                const QString &city,
                                const QString &street,
                                const QString &postCode)
{
    // We register a new application each time we use
    // register_with_profile to register a new user.
    // TODO: use IMEI as generator?
    // TODO: if an appname already exists, use that?
    bool newAppName = true;
    QString appName = QUuid::createUuid().toString();
    appName.chop(1);
    appName = appName.mid(1);

    QUrl reqUrl;
    reqUrl.setScheme("https");
    reqUrl.setHost("account-b45a9d3f.jollamobile.com");
    reqUrl.setPath("/api/registration/account/register_with_profile/");
    QNetworkRequest req(createTlsRequest(reqUrl));
    req.setRawHeader("Content-Type", "application/json");

    bool ok = false;
    QVariantMap registrationDetails;
    registrationDetails.insert("username", username);
    registrationDetails.insert("password1", password);
    registrationDetails.insert("password2", password);
    registrationDetails.insert("email", email);
    registrationDetails.insert("first_name", firstName);
    registrationDetails.insert("last_name", lastName);
    registrationDetails.insert("country", countryCode);
    registrationDetails.insert("city", city);
    registrationDetails.insert("street", street);
    registrationDetails.insert("zipcode", postCode);
    registrationDetails.insert("oauth_application", appName);
    QByteArray putData = toJsonBlob(registrationDetails, &ok);

    QNetworkReply *reply = ok ? m_qnam->put(req, putData) : NULL;
    if (!reply) {
        //: Error displayed when the request to the Jolla server to register a user fails
        //% "Failed to request Jolla user registration"
        emit registerUserAccountFailed(qtTrId("jolla_account_provider-register_user_request_failed"));
        return;
    }

    if (newAppName) {
        // if it succeeds, need to store to key file.
        reply->setProperty("appName", QVariant(appName));
    }

    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(registerUserError(QNetworkReply::NetworkError)));
    connect(reply, SIGNAL(finished()), this, SLOT(registerUserFinished()));
}

void JollaAccountProvider::registerUserError(const QNetworkReply::NetworkError &err)
{
    // 409 means "either a param is missing (user/pass/email) or username already taken"
    // we should emit an error here and abort the process, as it requires
    // user input (change the user name).

    int httpCode = -1;
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (reply) {
        reply->setProperty("isError", QVariant::fromValue<bool>(true));
        httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    }

    if (httpCode != 0) {
        if (httpCode == 409) {
            //: Error displayed when the request to the Jolla server to register a user is missing parameters
            //% "Missing parameter or username already taken"
            emit registerUserAccountFailed(qtTrId("jolla_account_provider-register_user_missing_params"));
        } else {
            //: Error displayed when the request to the Jolla server to register a user is rejected
            //% "Request rejected with HTTP code %1"
            emit registerUserAccountFailed(qtTrId("jolla_account_provider-register_user_rejected").arg(httpCode));
        }
    } else {
        //: Error displayed when the request to the Jolla server to register a user errors due to network error
        //% "Network error: %1"
        emit registerUserAccountFailed(qtTrId("jolla_account_provider-register_user_request_error").arg(networkErrorString(err)));
    }
}

void JollaAccountProvider::registerUserFinished()
{
    // 200 ok means succeeded.  Response should contain:
    // access_token, refresh_token.

    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }

    if (reply->property("isError").toBool() == true) {
        reply->deleteLater();
        return;
    }

    // if it succeeded, read out the token and emit success.
    QByteArray replyData = reply->readAll();
    bool ok = false;
    QVariantMap parsed = fromJsonBlob(replyData, &ok);
    if (parsed.contains("access_token")
            && !parsed.value("access_token").toString().isEmpty()) {
        // store the app details retrieved for the registration.
        // XXX TODO: need the app name....
        storeApplicationKeys(QString(), parsed.value("app_id").toString(), parsed.value("secret").toString());

        // Now fill out the responseData map as expected by the factory
        QVariantMap responseData;
        // first, the tokens
        responseData.insert(QLatin1String("AccessToken"), parsed.value("access_token").toString());
        responseData.insert(QLatin1String("RefreshToken"), parsed.value("refresh_token").toString());
        responseData.insert(QLatin1String("ExpiresIn"), parsed.value("expires").toInt());
        responseData.insert(QLatin1String("ClientId"), parsed.value("app_id").toString());
        responseData.insert(QLatin1String("ClientSecret"), parsed.value("secret").toString());
        emit registerUserAccountSucceeded(responseData);
    } else {
        //: Error displayed when the response from the Jolla server is missing data
        //% "Missing data from response to create user request"
        emit registerUserAccountFailed(qtTrId("jolla_account_provider-register_user_missing_response"));
    }

    reply->deleteLater();
    return;
}

void JollaAccountProvider::registerExistingAccount(const QString &username, const QString &password)
{
    registerApplication(username, password);
}

void JollaAccountProvider::registerApplication(const QString &username, const QString &password)
{
    // step one: check if application already exists - abort if so.
    // step two: fetch IMEI
    // step three: generate "application name" from IMEI
    // step four: POST /oauth2/application/create/?name=applicationName
    // step five: decode result:
    //     name: the name of newly created application
    //     id: Local application's ID
    //     key: application's key
    //     secret: the secret
    // step six: store those to libsailfishkeyprovider
    //
    //
    // Note: if application creation fails (because application with that
    // name was previously registered - eg, before a factory reset) then
    // what do we do?  We can't remove it via REST api because we don't
    // know the old id...
    //
    // I guess if it fails, we could just append a random number to the
    // end, ie generate a new set of application keys and use those.

    if (checkApplicationKeys() == JollaAccountProvider::KeySuccess) {
        // already have registered a device-specific application.
        // go directly to logging the user in with that app.
        return;
    }

    // XXX TODO: use IMEI as generator.
    QString imei = QUuid::createUuid().toString();
    imei.chop(1);
    imei = imei.mid(1);

    QUrl reqUrl;
    reqUrl.setScheme("https");
    reqUrl.setHost("account-b45a9d3f.jollamobile.com");
    reqUrl.setPath("/oauth2/application/create/");

    QByteArray postData;
    QNetworkRequest req(createTlsRequest(reqUrl));
    QString multipartBoundary = QLatin1String("-------Sska2129ifcalksmqq3");
    postData.append("--"+multipartBoundary+"\r\n");
    postData.append("Content-Disposition: form-data; name=\"name\"\r\n\r\n");
    postData.append(imei);
    postData.append("\r\n");
    postData.append("--"+multipartBoundary+"\r\n");
    req.setRawHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
    req.setRawHeader("Accept-Language", "en-us,en;q=0.5");
    req.setRawHeader("Accept-Encoding", "gzip,deflate");
    req.setRawHeader("Accept-Charset", "ISO-8859-1,utf-8;q=0.7,*;q=0.7");
    req.setRawHeader("Keep-Alive", "300");
    req.setRawHeader("Connection", "keep-alive");
    req.setRawHeader("Content-Type",QString("multipart/form-data; boundary="+multipartBoundary).toLatin1());
    req.setHeader(QNetworkRequest::ContentLengthHeader, postData.size());

    QNetworkReply *reply = m_qnam->post(req, postData);
    if (!reply) {
        //: Error displayed when the request to the Jolla server to create an application fails
        //% "Failed to request Jolla application creation"
        emit registerExistingAccountFailed(qtTrId("jolla_account_provider-create_app_request_failed"));
        return;
    }

    reply->setProperty("accountUsername", username);
    reply->setProperty("accountPassword", password);
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(registerApplicationError(QNetworkReply::NetworkError)));
    connect(reply, SIGNAL(finished()), this, SLOT(registerApplicationFinished()));
}

void JollaAccountProvider::registerApplicationError(const QNetworkReply::NetworkError &err)
{
    // extract the error code: 409 means "already exists".
    // if so, we should add random chars to the app name and try again?

    int httpCode = -1;
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (reply) {
        reply->setProperty("isError", QVariant::fromValue<bool>(true));
        httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    }

    if (httpCode != 0) {
        if (httpCode == 409) {
            // XXX TODO: retry with different appname.
            emit registerExistingAccountFailed("application name already exists.  TODO: retry.");
        } else {
            // XXX TODO: handle this properly
            emit registerExistingAccountFailed(QString(QLatin1String("application request failed with %1")).arg(httpCode));
        }
    } else {
        //: Error displayed when the request to the Jolla server to create an application fails due to network error
        //% "Network error: %2"
        emit registerExistingAccountFailed(qtTrId("jolla_account_provider-create_app_request_error").arg(networkErrorString(err)));
    }
}

void JollaAccountProvider::registerApplicationFinished()
{
    // 200 ok means succeeded.  Response should contain:
    // name, id, key, secret.  JSON.

    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }

    if (reply->property("isError").toBool()) {
        reply->deleteLater();
        return;
    }

    QByteArray replyData = reply->readAll();
    reply->deleteLater();
    bool ok = false;
    QVariantMap parsed = fromJsonBlob(replyData, &ok);
    if (!ok) {
        qWarning() << Q_FUNC_INFO << "unknown data:" << replyData;
        //: Error displayed when the response to the create application request is invalid
        //% "Unknown response from Jolla servers"
        emit registerExistingAccountFailed(qtTrId("jolla_account_provider-reply_error"));
        return;
    }

    QString appName = parsed.value(QLatin1String("name")).toString();
    QString appId = parsed.value(QLatin1String("id")).toString();
    QString clientId = parsed.value(QLatin1String("key")).toString();
    QString clientSecret = parsed.value(QLatin1String("secret")).toString();

    // Now, preferably, the log in user api (account/login) would take a client_id / client_secret
    // parameter, and return the AccessToken / RefreshToken.  But, as it does not, we need to
    // retrieve them manually, prior to logging the user in.
    if (!storeApplicationKeys(appName, clientId, clientSecret)) {
        //: Error displayed when unable to store encoded application keys
        //% "Error occurred while storing application keys"
        emit registerExistingAccountFailed(qtTrId("jolla_account_provider-store_appkeys_error"));
        return;
    }

    // This will actually be in one of the other functions, I guess, once we have the access token.
    // But for now, for testing purposes:
    QVariantMap tokens;
    tokens.insert("AccessToken", "123456");
    tokens.insert("RefreshToken", "abcdef");
    loginUserAccount(reply->property("accountUsername").toString(),
                     reply->property("accountPassword").toString(),
                     tokens);
}

void JollaAccountProvider::loginUserAccount(const QString &username, const QString &password, const QVariantMap &tokens)
{
    QUrl reqUrl;
    reqUrl.setScheme("https");
    reqUrl.setHost("account-b45a9d3f.jollamobile.com");
    reqUrl.setPath("/api/registration/account/login/");

    QByteArray postData;
    QNetworkRequest req(createTlsRequest(reqUrl));
    QString multipartBoundary = QLatin1String("-------Sska2129ifcalksmqq3");
    postData.append("--"+multipartBoundary+"\r\n");
    postData.append("Content-Disposition: form-data; name=\"username\"\r\n\r\n");
    postData.append(username);
    postData.append("\r\n");
    postData.append("--"+multipartBoundary+"\r\n");
    postData.append("Content-Disposition: form-data; name=\"password\"\r\n\r\n");
    postData.append(password);
    postData.append("\r\n");
    postData.append("--"+multipartBoundary+"\r\n");
    req.setRawHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
    req.setRawHeader("Accept-Language", "en-us,en;q=0.5");
    req.setRawHeader("Accept-Encoding", "gzip,deflate");
    req.setRawHeader("Accept-Charset", "ISO-8859-1,utf-8;q=0.7,*;q=0.7");
    req.setRawHeader("Keep-Alive", "300");
    req.setRawHeader("Connection", "keep-alive");
    req.setRawHeader("Content-Type",QString("multipart/form-data; boundary="+multipartBoundary).toLatin1());
    req.setHeader(QNetworkRequest::ContentLengthHeader, postData.size());

    QNetworkReply *reply = m_qnam->post(req, postData);
    if (!reply) {
        //: Error displayed when the request to the Jolla server to login the user fails
        //% "Failed to request Jolla user login"
        emit registerExistingAccountFailed(qtTrId("jolla_account_provider-login_user_request_failed"));
        return;
    }

    reply->setProperty("accountTokens", tokens);
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(loginUserAccountError(QNetworkReply::NetworkError)));
    connect(reply, SIGNAL(finished()), this, SLOT(loginUserAccountFinished()));
}

void JollaAccountProvider::loginUserAccountError(const QNetworkReply::NetworkError &err)
{
    int httpCode = -1;
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (reply) {
        reply->setProperty("isError", QVariant::fromValue<bool>(true));
        httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    }

    //: Error displayed when the request to the Jolla server to login a user errors due to network error
    //% "Error occurred during login: %1 HTTP: %2"
    emit registerExistingAccountFailed(qtTrId("jolla_account_provider-login_user_request_error").arg(networkErrorString(err)).arg(httpCode));
}

void JollaAccountProvider::loginUserAccountFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }

    if (reply->property("isError").toBool() == true) {
        reply->deleteLater();
        return;
    }

    QByteArray replyData = reply->readAll();
    qWarning() << Q_FUNC_INFO << QString(QLatin1String("login succeeded: %1")).arg(QString::fromUtf8(replyData));
    QVariantMap tokens = reply->property("accountTokens").toMap();
    reply->deleteLater();
    emit registerExistingAccountSucceeded(tokens);
}
