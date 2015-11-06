/*
 * Copyright (C) 2015 Jolla Ltd.
 * Contact: Chris Adams <chris.adams@jollamobile.com>
 *
 * License: Proprietary
 */

#include "cloudbackupsynctrigger.h"

#include <QFileInfo>
#include <QPair>
#include <QUrl>
#include <QUrlQuery>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QVariantMap>
#include <QVariantList>
#include <QCryptographicHash>
#include <QtDebug>

// ssu
#include <ssudeviceinfo.h>

// mlite5
#include <MGConfItem>

// Buteo
#include <ProfileEngineDefs.h>

// libaccounts-qt5
#include <Accounts/Manager>
#include <Accounts/Account>
#include <Accounts/Service>
#include <Accounts/AccountService>

// libsignon-qt
#include <SignOn/Identity>
#include <SignOn/AuthSession>
#include <SignOn/SessionData>

// libsailfishkeyprovider
#include <sailfishkeyprovider.h>

namespace {
    QString cloudServiceName(const QStringList &accountSyncProfileIds, QString *backupSyncProfile) {
        Q_FOREACH (const QString &profileId, accountSyncProfileIds) {
            if (profileId.contains("backup", Qt::CaseInsensitive)) {
                // have valid backup sync profile
                *backupSyncProfile = profileId;
                if (backupSyncProfile->contains("onedrive", Qt::CaseInsensitive)) {
                    return QStringLiteral("OneDrive");
                } else if (backupSyncProfile->contains("dropbox", Qt::CaseInsensitive)) {
                    return QStringLiteral("Dropbox");
                }
                qDebug() << "Have unknown backup sync profile:" << profileId << ", ignoring.";
            }
        }
        return QString();
    }

    QPair<QString,QString> clientIdAndSecret(const QString &cloudName)
    {
        QPair<QString,QString> retn;
        char *cClientId = NULL;
        char *cClientSecret = NULL;
        bool isOneDrive = cloudName == QStringLiteral("OneDrive");

        int cSuccess = SailfishKeyProvider_storedKey(isOneDrive ? "onedrive" : "dropbox",
                                                     isOneDrive ? "onedrive-sync" : "dropbox-sync",
                                                     "client_id",
                                                     &cClientId);
        if (cClientId == NULL) {
            return retn;
        } else if (cSuccess != 0) {
            free(cClientId);
            return retn;
        }

        retn.first = QLatin1String(cClientId);
        free(cClientId);

        cSuccess = SailfishKeyProvider_storedKey(isOneDrive ? "onedrive" : "dropbox",
                                                 isOneDrive ? "onedrive-sync" : "dropbox-sync",
                                                 "client_secret",
                                                 &cClientSecret);
        if (cClientSecret == NULL) {
            return retn;
        } else if (cSuccess != 0) {
            free(cClientSecret);
            return retn;
        }

        retn.second = QLatin1String(cClientSecret);
        free(cClientSecret);
        return retn;
    }

    QJsonObject parseJsonObjectReplyData(const QByteArray &replyData, bool *ok)
    {
        QJsonDocument jsonDocument = QJsonDocument::fromJson(replyData);
        *ok = !jsonDocument.isEmpty();
        if (*ok && jsonDocument.isObject()) {
            return jsonDocument.object();
        }
        *ok = false;
        return QJsonObject();
    }

    QJsonArray parseJsonArrayReplyData(const QByteArray &replyData, bool *ok)
    {
        QJsonDocument jsonDocument = QJsonDocument::fromJson(replyData);
        *ok = !jsonDocument.isEmpty();
        if (*ok && jsonDocument.isArray()) {
            return jsonDocument.array();
        }
        *ok = false;
        return QJsonArray();
    }
}

CloudBackupSyncTrigger::CloudBackupSyncTrigger(QObject *parent)
    : QObject(parent)
    , m_accountSyncManager(new AccountSyncManager(this))
    , m_networkManager(new QNetworkAccessManager(this))
{
    QString deviceId = SsuDeviceInfo().deviceUid();
    QByteArray hashedDeviceId = QCryptographicHash::hash(deviceId.toUtf8(), QCryptographicHash::Sha256);
    QString encodedDeviceId = QString::fromUtf8(hashedDeviceId.toBase64()).mid(0,12);
    QString defaultRemotePath = QString::fromLatin1("Backups/%1").arg(encodedDeviceId);
    m_defaultRemoteBackupsDirectory = deviceId.isEmpty() ? QString() : defaultRemotePath;
    connect(m_accountSyncManager, SIGNAL(profileSyncStatusChanged(QString,int,QString)),
            this, SLOT(handleProfileSyncStatusChanged(QString,int,QString)));
}

QString CloudBackupSyncTrigger::defaultRemoteBackupsDirectory() const
{
    return m_defaultRemoteBackupsDirectory;
}

bool CloudBackupSyncTrigger::syncWithCloud(int accountId, const QString &localDir, const QString &remotePath, const QString &remoteFile, const QString &direction)
{
    if (!m_currentSyncProfileId.isEmpty()) {
        qWarning() << "Currently busy syncing:" << m_currentSyncProfileId;
        return false;
    }

    // find appropriate sync profile for account
    QString backupSyncProfile;
    QStringList profileIds = m_accountSyncManager->profileIds(accountId);
    QString cloudName = cloudServiceName(profileIds, &backupSyncProfile);
    if (backupSyncProfile.isEmpty() || cloudName.isEmpty()) {
        qWarning() << "No valid cloud storage profile for account:" << accountId << ":" << profileIds;
        return false;
    }

    // set config keys as appropriate
    {
        static const QString vaultConfKeyTemplate = QStringLiteral("/SailfishOS/vault/%1/%2");

        QString localPath = localDir.size() ? localDir : QStringLiteral("/home/nemo/.local/share/system/privileged/Backups");
        if (direction == Buteo::VALUE_TO_REMOTE) {
            if (!QFileInfo::exists(localPath) || !QFileInfo(localPath).isDir()) {
                qWarning() << "Local path:" << localPath << "does not exist or is not directory; cannot upsync to cloud!";
                return false;
            }
        }

        // set local dir key
        MGConfItem localPathConf(vaultConfKeyTemplate.arg(cloudName).arg(QStringLiteral("localPath")));
        localPathConf.set(localPath);

        // set remote path key
        MGConfItem remotePathConf(vaultConfKeyTemplate.arg(cloudName).arg(QStringLiteral("remotePath")));
        remotePathConf.set(remotePath); // if empty, will be set empty.  Sync plugin will figure out what it should be.

        // set remote path key
        MGConfItem remoteFileConf(vaultConfKeyTemplate.arg(cloudName).arg(QStringLiteral("remoteFile")));
        remoteFileConf.set(remoteFile); // if empty, will be set empty, which denotes "all files from the remote path"

        // set sync direction key
        MGConfItem directionConf(vaultConfKeyTemplate.arg(cloudName).arg(QStringLiteral("direction")));
        directionConf.set(direction);
    }

    // trigger sync
    m_currentSyncProfileId = backupSyncProfile;
    m_accountSyncManager->syncProfile(backupSyncProfile);
    return true;
}

bool CloudBackupSyncTrigger::downloadFromCloud(int accountId, const QString &localDir, const QString &remotePath, const QString &remoteFile)
{
    return syncWithCloud(accountId, localDir, remotePath, remoteFile, Buteo::VALUE_FROM_REMOTE);
}

bool CloudBackupSyncTrigger::uploadToCloud(int accountId, const QString &localDir, const QString &remotePath)
{
    return syncWithCloud(accountId, localDir, remotePath, QString(), Buteo::VALUE_TO_REMOTE);
}

void CloudBackupSyncTrigger::resetState()
{
    m_currentSyncProfileId.clear();
}

void CloudBackupSyncTrigger::handleProfileSyncStatusChanged(const QString &profileId, int status, const QString &errorString)
{
    if (profileId == m_currentSyncProfileId) {
        // this is a status change to a profile we triggered.
        switch (status) {
        case AccountSyncManager::SyncStarted:
            qDebug() << "Started cloud sync with profile:" << profileId;
            break;
        case AccountSyncManager::SyncFinished:
            qDebug() << "Successfully finished cloud sync with profile:" << profileId;
            m_currentSyncProfileId = QString(); // finished syncing this profile.
            break;
        case AccountSyncManager::SyncError:
            qDebug() << "Error during cloud sync with profile:" << profileId;
            m_currentSyncProfileId = QString(); // finished syncing this profile.
            break;
        case AccountSyncManager::SyncAborted:
            qDebug() << "Cloud sync with profile:" << profileId << "aborted!";
            m_currentSyncProfileId = QString(); // finished syncing this profile.
            break;
        default:
            qWarning() << "Cloud sync with profile:" << profileId << "has unknown status, ignoring signal...";
            return;
        }

        int accountId = 0;
        int accountIdIndex = profileId.lastIndexOf('-');
        if (accountIdIndex >= 0) {
            accountId = profileId.mid(accountIdIndex + 1).toInt();
            if (accountId == 0) {
                qWarning() << "Unable to extract account id from profileId" << profileId;
            }
        }

        // re-emit with queued, to ensure that it happens after we return from the download/upload functions in error case.
        QMetaObject::invokeMethod(this,
                                  "cloudSyncProgress",
                                  Qt::QueuedConnection,
                                  Q_ARG(int, accountId),
                                  Q_ARG(int, status),
                                  Q_ARG(QString, errorString));
    }
}

// --------------------------------------------- remote directory listing

bool CloudBackupSyncTrigger::requestListing(int accountId, const QString &remotePath)
{
    if (!m_currentSyncProfileId.isEmpty()) {
        qWarning() << "Currently busy syncing:" << m_currentSyncProfileId;
        return false;
    }

    // find appropriate sync profile for account
    QString backupSyncProfile;
    QStringList profileIds = m_accountSyncManager->profileIds(accountId);
    QString cloudName = cloudServiceName(profileIds, &backupSyncProfile);
    if (backupSyncProfile.isEmpty() || cloudName.isEmpty()) {
        qWarning() << "No valid cloud storage profile for account:" << accountId << ":" << profileIds;
        return false;
    }

    // Fetch consumer key and secret from keyprovider
    QPair<QString,QString> appKeys = clientIdAndSecret(cloudName);
    if (appKeys.first.isEmpty() || (cloudName == QStringLiteral("OneDrive") && appKeys.second.isEmpty())) {
        qDebug() << "Could not retrieve application keys for cloud provider:" << cloudName;
        return false;
    }

    // grab out a valid identity for the sync service.
    QString serviceName = cloudName == QStringLiteral("OneDrive") ? QStringLiteral("onedrive-backup") : QStringLiteral("dropbox-backup");
    Accounts::Service srv(m_accountSyncManager->accountManager()->service(serviceName));
    if (!srv.isValid()) {
        qDebug() << "Invalid account service specified:" << serviceName << "for cloud provider:" << cloudName;
        return false;
    }
    Accounts::Account *account = m_accountSyncManager->accountManager()->account(accountId);
    account->selectService(srv);
    SignOn::Identity *identity = account->credentialsId() > 0 ? SignOn::Identity::existingIdentity(account->credentialsId()) : 0;
    if (!identity) {
        qDebug() << "account" << accountId << "has no valid credentials; cannot sign in";
        return false;
    }

    Accounts::AccountService accSrv(account, srv);
    QString method = accSrv.authData().method();
    QString mechanism = accSrv.authData().mechanism();
    SignOn::AuthSession *session = identity->createSession(method);
    if (!session) {
        qDebug() << "could not create signon session for account" << accountId;
        identity->deleteLater();
        return false;
    }

    QVariantMap signonSessionData = accSrv.authData().parameters();
    signonSessionData.insert("ClientId", appKeys.first);
    if (!appKeys.second.isEmpty()) {
        signonSessionData.insert("ClientSecret", appKeys.second);
    }
    signonSessionData.insert("UiPolicy", SignOn::NoUserInteractionPolicy);

    connect(session, SIGNAL(response(SignOn::SessionData)),
            this, SLOT(signOnResponse(SignOn::SessionData)),
            Qt::UniqueConnection);
    connect(session, SIGNAL(error(SignOn::Error)),
            this, SLOT(signOnError(SignOn::Error)),
            Qt::UniqueConnection);

    session->setProperty("account", QVariant::fromValue<Accounts::Account*>(account));
    session->setProperty("identity", QVariant::fromValue<SignOn::Identity*>(identity));
    session->setProperty("cloudName", cloudName);
    session->setProperty("remotePath", remotePath);

    // set our status to busy, and perform signon to get AccessToken
    m_currentSyncProfileId = backupSyncProfile;
    session->process(SignOn::SessionData(signonSessionData), mechanism);
    return true;
}

void CloudBackupSyncTrigger::signOnError(const SignOn::Error &error)
{
    SignOn::AuthSession *session = qobject_cast<SignOn::AuthSession*>(sender());
    Accounts::Account *account = session->property("account").value<Accounts::Account*>();
    SignOn::Identity *identity = session->property("identity").value<SignOn::Identity*>();
    int accountId = account->id();
    qDebug() << "credentials for account with id" << accountId <<
                "couldn't be retrieved:" << error.type() << error.message();

    session->disconnect(this);
    identity->destroySession(session);
    identity->deleteLater();
    account->deleteLater();

    // if the error is because credentials have expired, we
    // set the CredentialsNeedUpdate key.
    if (error.type() == SignOn::Error::UserInteraction) {
        emit requestListingFailed(accountId, QStringLiteral("Credentials are invalid for account %1").arg(accountId));
    } else {
        emit requestListingFailed(accountId, QStringLiteral("Could not retrieve token for account %1: %2").arg(accountId).arg(error.message()));
    }
    m_currentSyncProfileId.clear();
}

void CloudBackupSyncTrigger::signOnResponse(const SignOn::SessionData &responseData)
{
    QVariantMap data;
    foreach (const QString &key, responseData.propertyNames()) {
        data.insert(key, responseData.getProperty(key));
    }

    QString accessToken;
    SignOn::AuthSession *session = qobject_cast<SignOn::AuthSession*>(sender());
    Accounts::Account *account = session->property("account").value<Accounts::Account*>();
    SignOn::Identity *identity = session->property("identity").value<SignOn::Identity*>();
    QString cloudName = session->property("cloudName").toString();
    QString remotePath = session->property("remotePath").toString();
    int accountId = account->id();
    if (data.contains(QLatin1String("AccessToken"))) {
        accessToken = data.value(QLatin1String("AccessToken")).toString();
    } else {
        qDebug() << "signon response for account with id" << accountId << "contained no access token";
    }

    session->disconnect(this);
    identity->destroySession(session);
    identity->deleteLater();
    account->deleteLater();

    if (!accessToken.isEmpty()) {
        if (m_currentSyncProfileId.isEmpty()) {
            // the listing request was cancelled via a call to resetState()
            qDebug() << "resetState() was called, will not perform listing request:" << cloudName << remotePath;
        } else {
            // finally perform the actual remote directory listing request
            qDebug() << "performing remote directory listing request:" << cloudName << remotePath;
            performListingRequest(accountId, accessToken, cloudName, remotePath);
        }
    } else {
        emit requestListingFailed(accountId, QStringLiteral("No access token response in credentials for account %1").arg(accountId));
        m_currentSyncProfileId.clear();
    }
}

void CloudBackupSyncTrigger::performListingRequest(int accountId, const QString &accessToken, const QString &cloudName, const QString &remotePath)
{
    // perform the appropriate REST call to get the directory listing
    if (m_defaultRemoteBackupsDirectory.isEmpty()) {
        emit requestListingFailed(accountId, QStringLiteral("Unable to determine device ssu id, cannot determine default backup directory!"));
        m_currentSyncProfileId.clear();
        return;
    }

    QUrl url;
    if (cloudName == QStringLiteral("OneDrive")) {
        url = QUrl(QStringLiteral("https://api.onedrive.com/v1.0/drive/special/approot:/%1:/").arg(remotePath.isEmpty() ? m_defaultRemoteBackupsDirectory : remotePath));
        QUrlQuery query(url);
        QList<QPair<QString, QString> > queryItems;
        queryItems.append(QPair<QString, QString>(QStringLiteral("expand"), QStringLiteral("children")));
        query.setQueryItems(queryItems);
        url.setQuery(query);
    } else { // cloudName == QStringLiteral("Dropbox")
        url = QUrl(QStringLiteral("https://api.dropboxapi.com/1/metadata/auto/%1").arg(remotePath.isEmpty() ? m_defaultRemoteBackupsDirectory : remotePath));
    }

    QNetworkRequest req(url);
    req.setRawHeader(QString(QLatin1String("Authorization")).toUtf8(),
                     QString(QLatin1String("Bearer ")).toUtf8() + accessToken.toUtf8());

    QNetworkReply *reply = m_networkManager->get(req);
    reply->setProperty("accountId", accountId);
    reply->setProperty("accessToken", accessToken);
    reply->setProperty("cloudName", cloudName);
    reply->setProperty("remotePath", remotePath.isEmpty() ? m_defaultRemoteBackupsDirectory : remotePath);
    connect(reply, SIGNAL(finished()), this, SLOT(handleListingResponse()));
}

void CloudBackupSyncTrigger::handleListingResponse()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    QByteArray data = reply->readAll();
    int accountId = reply->property("accountId").toInt();
    QString cloudName = reply->property("cloudName").toString();
    QString remotePath = reply->property("remotePath").toString();
    reply->deleteLater();

    bool ok = false;
    QString responseData = QString::fromUtf8(data);
    QJsonObject parsed = parseJsonObjectReplyData(data, &ok);
    if (!ok || (cloudName == QStringLiteral("Dropbox")  && !parsed.contains("contents"))
            || (cloudName == QStringLiteral("OneDrive") && !parsed.contains("children"))) {
        qDebug() << "unable to parse directory listing from response for:" << cloudName << remotePath;
        Q_FOREACH (const QString &line, responseData.split('\n')) {
            qDebug() << line;
        }
        emit requestListingFailed(accountId, QStringLiteral("Unable to parse directory listing from response for account %1").arg(accountId));
        m_currentSyncProfileId.clear();
        return;
    }

    QVariantList listing;
    if (cloudName == QStringLiteral("OneDrive")) {
        QJsonArray children = parsed.value("children").toArray();
        Q_FOREACH (const QJsonValue &child, children) {
            QVariantMap entry;
            const QString childName = child.toObject().value("name").toString();
            entry.insert("name", childName);
            entry.insert("parent", remotePath);
            if (child.toObject().keys().contains("folder")) {
                entry.insert("is_dir", true);
            } else {
                entry.insert("is_dir", false);
            }
            listing.append(entry);
        }
    } else { // cloudName == QStringLiteral("Dropbox")
        QJsonArray contents = parsed.value("contents").toArray();
        Q_FOREACH (const QJsonValue &child, contents) {
            QVariantMap entry;
            const QString childPath = child.toObject().value("path").toString();
            entry.insert("name", childPath.split('/').last());
            entry.insert("parent", remotePath);
            if (child.toObject().value("is_dir").toBool() == true) {
                entry.insert("is_dir", true);
            } else {
                entry.insert("is_dir", false);
            }
            listing.append(entry);
        }
    }

    emit requestedListing(accountId, listing);
    m_currentSyncProfileId.clear();
}
