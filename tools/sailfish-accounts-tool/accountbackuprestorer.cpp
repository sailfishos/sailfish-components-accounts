/*
** Copyright (C) 2014 Jolla Ltd.
*/

#include "accountbackuprestorer_p.h"

#include <accountsyncmanager.h>

// libsailfishkeyprovider
#include <sailfishkeyprovider.h>

// buteo
#include <ProfileEngineDefs.h>

#include <QFile>
#include <QTimer>
#include <QDateTime>
#include <QThread>
#include <QMutexLocker>
#include <QCoreApplication>
#include <QtDebug>
#include <QRegExp>

#define BACKUP_RESTORER_VERSION 1

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

    QStringList legacyCalDavServices() {
        QStringList serviceNames;
        serviceNames << QStringLiteral("onlinesync-caldav_yahoo");
        serviceNames << QStringLiteral("onlinesync-caldav_memotoo");
        serviceNames << QStringLiteral("onlinesync-caldav_generic");
        serviceNames << QStringLiteral("onlinesync-caldav_fruux");
        return serviceNames;
    }

template <typename To, typename From>
QVariant convert(From const &src)
{
    return QVariant::fromValue<To>(src);
}

template <typename T>
T value(QSettings const &settings, QString const &name, T const &defVal)
{
    return settings.value(name, convert<T>(defVal)).value<T>();
}

template <typename T>
T value(QSettings const &settings, QString const &name)
{
    return settings.value(name).value<T>();
}

inline bool hasType(QVariant const &v, QMetaType::Type t)
{
    return static_cast<QMetaType::Type>(v.type()) == t;
}

QRegExp re(QString const &cs)
{
    return QRegExp(cs, Qt::CaseSensitive, QRegExp::RegExp2);
};

const std::vector<std::pair<QRegExp, QVariant::Type> > re_types
= {{re("[+-][0-9]+"), QVariant::Int}
   , {re("[0-9]{11,}"), QVariant::String}
   , {re("[0-9]+"), QVariant::UInt}
   // , {re("[+-]?([0-9]+\\.[0-9]*|[0-9]*\\.[0-9]+)"), QVariant::Double}
   , {re("true|false"), QVariant::Bool}
};

QVariant deduce(QVariant const& v)
{
    if (!hasType(v, QMetaType::QString))
        return v;

    QVariant res = v;
    QString s = v.toString();
    for (auto const& re_type : re_types) {
        if (re_type.first.exactMatch(s)) {
            res.convert(re_type.second);
            break;
        }
    }
    return res;
}

QVariant deduce(QSettings const &v, QString const &key)
{
    auto res = deduce(v.value(key));
    return res;
}

}

AccountBackupRestorer::AccountBackupRestorer(AccountSyncManager *syncManager,
                                             Accounts::Manager *accountManager,
                                             QObject *parent)
    : QObject(parent)
    , m_syncManager(syncManager)
    , m_accountManager(accountManager)
{
}

AccountBackupRestorer::~AccountBackupRestorer()
{
}

QList<uint> AccountBackupRestorer::oldAccountIds() const
{
    return m_oldAccountIds;
}

QList<uint> AccountBackupRestorer::newAccountIds() const
{
    return m_newAccountIds;
}

// CalDAV accounts prior to Sailfish 1.1.0.x all used the same onlinesync.provider with services
// like onlinesync-caldav_yahoo, onlinesync-caldav_generic, etc. These accounts need to be ported
// to use separate providers like yahoo.provider with yahoo-caldav.service, or for the generic
// CalDAV case, continue to use onlinesync.provider but use the onlinesync-caldav.service instead.
void AccountBackupRestorer::getCalDavMigrationParameters(const QString &providerName,
                                                         QSettings *backupIni,
                                                         CalDavMigrationData *migrationData)
{
    if (providerName != QStringLiteral("onlinesync")) {
        migrationData->migrationRequired = false;
        return;
    }

    // find the legacy CalDAV service for this account
    backupIni->beginGroup(QStringLiteral("serviceSettings"));
    migrationData->oldCalDavServiceName.clear();
    int newServiceNameStartIndex = -1;
    Q_FOREACH (const QString &serviceName, backupIni->childGroups()) {
        if (serviceName.startsWith(QStringLiteral("onlinesync-caldav_"))) {
            backupIni->beginGroup(serviceName);
            QVariant templateProfiles = backupIni->value(QStringLiteral("sync_profile_templates"));
            bool enabled = value(*backupIni, QStringLiteral("enabled"), false);
            backupIni->endGroup();
            if (enabled && templateProfiles.isValid() && !templateProfiles.toStringList().isEmpty()) {
                migrationData->oldCalDavServiceName = serviceName;
                newServiceNameStartIndex = serviceName.indexOf('_') + 1;
                break;
            }
        }
    }
    backupIni->endGroup();
    if (migrationData->oldCalDavServiceName.isEmpty()) {
        migrationData->migrationRequired = false;
        return;
    }

    if (migrationData->oldCalDavServiceName == QStringLiteral("onlinesync-caldav_generic")) {
        migrationData->providerName = providerName;
    } else {
        migrationData->providerName = migrationData->oldCalDavServiceName.mid(newServiceNameStartIndex);
    }
    migrationData->newCalDavServiceName = QString("%1-caldav").arg(migrationData->providerName);
    migrationData->migrationRequired = true;
}

Accounts::Service AccountBackupRestorer::findLegacyCalDavService(Accounts::Account *account) const
{
    Accounts::Service prevService = account->selectedService();
    Q_FOREACH (const QString &serviceName, legacyCalDavServices()) {
        Accounts::Service srv = m_accountManager->service(serviceName);
        if (!srv.isValid()) {
            continue;
        }
        account->selectService(srv);
        QVariant enabled = account->value(QStringLiteral("enabled"));
        QVariant templateProfiles = account->value(QStringLiteral("sync_profile_templates"));
        if (enabled.isValid() && enabled.toBool()
                && templateProfiles.isValid() && !templateProfiles.toStringList().isEmpty()) {
            account->selectService(prevService);
            return srv;
        }
    }
    account->selectService(prevService);
    return Accounts::Service();
}

bool AccountBackupRestorer::backupAccount(Accounts::Account *account, const QString &backupFile)
{
    // we write the account data to the backup file in .ini format
    QSettings backupIni(backupFile, QSettings::IniFormat);
    if (!backupIni.isWritable()) {
        qWarning() << Q_FUNC_INFO << "backup file not writable:" << backupFile;
        return false;
    }

    if (!backupIni.childGroups().contains(QStringLiteral("metadata"))) {
        // if the file has not yet been tagged with metadata, do so.
        backupIni.beginGroup(QStringLiteral("metadata"));
        backupIni.setValue(QStringLiteral("version"), BACKUP_RESTORER_VERSION);
        backupIni.setValue(QStringLiteral("datetime"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
        backupIni.endGroup();
    }

    account->selectService(Accounts::Service());
    Accounts::ServiceList accountServices(account->services());
    QString providerName = account->providerName();
    QString skpSrvName = providerName == QStringLiteral("jolla") ? QStringLiteral("jolla-store") : QString();
    QString clientId = skp_storedKey(providerName, skpSrvName, QStringLiteral("client_id"));
    QString clientSecret = skp_storedKey(providerName, skpSrvName, QStringLiteral("client_secret"));
    QString consumerKey = skp_storedKey(providerName, skpSrvName, QStringLiteral("consumer_key"));
    QString consumerSecret = skp_storedKey(providerName, skpSrvName, QStringLiteral("consumer_secret"));
    QList<SignOnCredentials> requiredCredentials;

    // If the account has a legacy CalDAV service, back it up also, so that it can be created with
    // the new CalDAV service type when the account is restored.
    Accounts::Service legacyCalDavService = findLegacyCalDavService(account);
    if (legacyCalDavService.isValid()) {
        accountServices.append(legacyCalDavService);
    }

    // New group for this account.
    backupIni.beginGroup(QString::number(account->id()));
    {
        // Backup the global settings.
        // note that they may be duplicated later when fetching the default service key/values.
        backupIni.beginGroup(QStringLiteral("globalSettings"));
        {
            backupIni.setValue(QStringLiteral("providerName"), QVariant::fromValue<QString>(providerName));
            backupIni.setValue(QStringLiteral("enabled"), QVariant::fromValue<bool>(account->enabled()));
            backupIni.setValue(QStringLiteral("displayName"), QVariant::fromValue<QString>(account->displayName()));
        }
        backupIni.endGroup();

        // Backup all service settings.
        backupIni.beginGroup(QStringLiteral("serviceSettings"));
        {
            // Backup the default service settings
            backupAccountServiceSettings(backupIni, accountServices, Accounts::Service(), account, requiredCredentials,
                                         clientId, clientSecret, consumerKey, consumerSecret);

            // Backup the real service settings
            Q_FOREACH (const Accounts::Service &srv, accountServices) {
                backupAccountServiceSettings(backupIni, accountServices, srv, account, requiredCredentials,
                                             clientId, clientSecret, consumerKey, consumerSecret);
            }
        }
        backupIni.endGroup();

        // backup credentials settings
        backupIni.beginGroup(QStringLiteral("credentialsSettings"));
        {
            Q_FOREACH(const SignOnCredentials &rsoc, requiredCredentials) {
                // some ugliness to query the IdentityInfo data "synchronously"
                QThread thread;
                CredentialKeysQuery *query = new CredentialKeysQuery(rsoc.id, rsoc.method, rsoc.mechanism, rsoc.sessionData);
                connect(&thread, SIGNAL(finished()), query, SLOT(deleteLater()));
                query->moveToThread(&thread);
                thread.start();
                QTimer::singleShot(0, query, SLOT(queryCredentials()));
                while (!query->finished()) {
                    QCoreApplication::processEvents();
                    QThread::msleep(100);
                }
                QVariantMap infoValues = query->infoValues();
                QVariantMap methodMechanismSecrets = query->methodMechanismSecrets();
                thread.quit();
                thread.wait();

                // we now have the IdentityInfo data and can back it up.
                if (!infoValues.isEmpty()) {
                    backupIni.beginGroup(QString::number(rsoc.id));

                    backupIni.beginGroup(QStringLiteral("infoValues"));
                    Q_FOREACH (const QString &key, infoValues.keys()) {
                        backupIni.setValue(key, infoValues.value(key));
                    }
                    backupIni.endGroup();

                    backupIni.beginGroup(QStringLiteral("methodMechanismSecrets"));
                    Q_FOREACH (const QString &method, methodMechanismSecrets.keys()) {
                        backupIni.beginGroup(method);
                        QVariantMap mechanismSecrets = methodMechanismSecrets.value(method).toMap();
                        Q_FOREACH (const QString &mechanism, mechanismSecrets.keys()) {
                            backupIni.beginGroup(mechanism);
                            QVariantMap secrets = mechanismSecrets.value(mechanism).toMap();
                            Q_FOREACH (const QString &key, secrets.keys()) {
                                backupIni.setValue(key, secrets.value(key));
                            }
                            backupIni.endGroup();
                        }
                        backupIni.endGroup();
                    }
                    backupIni.endGroup();

                    backupIni.endGroup();
                }
            }
        }
        backupIni.endGroup();


        // backup the sync profile settings
        backupIni.beginGroup(QStringLiteral("syncSettings"));
        {
            Q_FOREACH (const Accounts::Service &srv, accountServices) {
                account->selectService(srv);
                Q_FOREACH (const QString &templateProfile, m_syncManager->defaultTemplateProfiles(account, srv)) {
                    backupIni.beginGroup(templateProfile);
                    QMap<QString, QString> settingsMap = syncProfileSettings(account, srv, templateProfile);
                    Q_FOREACH (const QString &key, settingsMap.keys()) {
                        backupIni.setValue(key, settingsMap.value(key));
                    }
                    backupIni.endGroup();
                }
            }
            account->selectService(Accounts::Service());
        }
        backupIni.endGroup();

        // backup the sync schedule
        backupIni.beginGroup(QStringLiteral("syncSchedule"));
        {
            Q_FOREACH (const Accounts::Service &srv, accountServices) {
                account->selectService(srv);
                Q_FOREACH (const QString &templateProfile, m_syncManager->defaultTemplateProfiles(account, srv)) {
                    backupIni.beginGroup(templateProfile);
                    backupIni.setValue(QStringLiteral("xml"), syncScheduleXml(account, srv, templateProfile));
                    backupIni.endGroup();
                }
            }
            account->selectService(Accounts::Service());
        }
        backupIni.endGroup();
    }
    backupIni.endGroup();

    return true;
}

bool AccountBackupRestorer::restoreAccounts(const QString &backupFile,
                                            QMap<int, QMap<QString, QVariantMap> > *syncProfileSettings,
                                            QMap<int, QMap<QString, QString> > *syncScheduleXml)
{
    if (!QFile::exists(backupFile)) {
        qWarning() << Q_FUNC_INFO << "backup file does not exist, cannot restore accounts";
        return false;
    }

    m_oldAccountIds.clear();
    m_newAccountIds.clear();

    QSettings backupIni(backupFile, QSettings::IniFormat);
    QStringList oldAccountIds = backupIni.childGroups();
    Q_FOREACH (const QString &oldAccountId, oldAccountIds) {
        if (oldAccountId == QStringLiteral("metadata")) {
            continue; // not a backed up account, but metadata about the backup file.
        }

        // we need to create a new account, and set its settings to match the backed up ones.
        Accounts::Account *newAccount = NULL;
        QString providerName;
        QString displayName;
        bool enabled = false;
        QMap<QString, bool> servicesEnabled;

        backupIni.beginGroup(oldAccountId);
        {
            backupIni.beginGroup(QStringLiteral("globalSettings"));
            {
                providerName = backupIni.value(QStringLiteral("providerName"), QVariant::fromValue<QString>(QString())).toString();
                displayName = backupIni.value(QStringLiteral("displayName"), QVariant::fromValue<QString>(QString())).toString();
                enabled = backupIni.value(QStringLiteral("enabled"), QVariant::fromValue<bool>(false)).toBool();
            }
            backupIni.endGroup();

            CalDavMigrationData calDavMigrationData;
            getCalDavMigrationParameters(providerName, &backupIni, &calDavMigrationData);
            if (calDavMigrationData.migrationRequired) {
                providerName = calDavMigrationData.providerName;
            }

            if (providerName.isEmpty()) {
                qWarning() << Q_FUNC_INFO << "no providerName specified for backed-up account" << oldAccountId << ", skipping";
            } else {
                newAccount = m_accountManager->createAccount(providerName);
                newAccount->selectService(Accounts::Service());
                newAccount->setDisplayName(displayName);
                newAccount->setEnabled(enabled);
                Accounts::ServiceList accountServices(newAccount->services());

                // first we restore the credentials.  This allows us to create a mapping
                // from the old credentials id to the new credentials id, which becomes
                // important later when we restore the service settings.
                QMap<quint32, quint32> oldToNewCredentialsIds;
                QVariantMap allCredentialsSettings;
                backupIni.beginGroup(QStringLiteral("credentialsSettings"));
                {
                    QStringList oldCredentialsIds = backupIni.childGroups();
                    Q_FOREACH (const QString &oldCredIdStr, oldCredentialsIds) {
                        backupIni.beginGroup(oldCredIdStr);

                        // read the identity info values.  these shouldn't change across method/mechs
                        QVariantMap infoValues;
                        backupIni.beginGroup(QStringLiteral("infoValues"));
                        Q_FOREACH (const QString &key, backupIni.allKeys()) {
                            infoValues.insert(key, deduce(backupIni, key));
                        }
                        backupIni.endGroup();

                        // read the secrets for this method/mechanism.  these can be different.
                        QVariantMap methodMechanismSecrets;
                        backupIni.beginGroup(QStringLiteral("methodMechanismSecrets"));
                        Q_FOREACH (const QString &method, backupIni.childGroups()) {
                            backupIni.beginGroup(method);
                            QVariantMap mechanismSecrets;
                            Q_FOREACH (const QString &mechanism, backupIni.childGroups()) {
                                backupIni.beginGroup(mechanism);
                                QVariantMap secrets;
                                Q_FOREACH (const QString &key, backupIni.allKeys()) {
                                    secrets.insert(key, deduce(backupIni, key));
                                }
                                mechanismSecrets.insert(mechanism, secrets);
                                backupIni.endGroup();
                            }
                            methodMechanismSecrets.insert(method, mechanismSecrets);
                            backupIni.endGroup();
                        }
                        backupIni.endGroup();

                        // insert the information for this credential into our settings map
                        QVariantMap credentialSettings;
                        credentialSettings.insert(QStringLiteral("infoValues"), infoValues);
                        credentialSettings.insert(QStringLiteral("methodMechanismSecrets"), methodMechanismSecrets);
                        allCredentialsSettings.insert(oldCredIdStr, credentialSettings);

                        // done with this credential.
                        backupIni.endGroup();
                    }

                    // we now have the information required to create ALL backed-up credentials.
                    oldToNewCredentialsIds = createCredentials(allCredentialsSettings);
                }
                backupIni.endGroup();

                // now we can restore the service settings
                backupIni.beginGroup(QStringLiteral("serviceSettings"));
                {
                    // restore the global service settings
                    restoreAccountServiceSettings(backupIni, Accounts::Service(), newAccount,
                                                  oldAccountId, oldToNewCredentialsIds, &servicesEnabled);

                    // restore real service settings
                    Q_FOREACH (const Accounts::Service &srv, accountServices) {
                        if (calDavMigrationData.migrationRequired) {
                            if (srv.name() != calDavMigrationData.newCalDavServiceName) {
                                // only the legacy service needs to be restored
                                continue;
                            } else {
                                // move settings from the legacy service to the new one
                                restoreAccountServiceSettings(backupIni, srv, newAccount, oldAccountId,
                                                              oldToNewCredentialsIds, &servicesEnabled,
                                                              calDavMigrationData.oldCalDavServiceName);
                            }
                        } else {
                            restoreAccountServiceSettings(backupIni, srv, newAccount, oldAccountId,
                                                          oldToNewCredentialsIds, &servicesEnabled);
                        }
                    }
                }
                backupIni.endGroup();

                // we have restored the account.  Write it to disk.
                newAccount->syncAndBlock();

                // now set the enablement statuses - we have to do this
                // separately to the creation sync, for some reason.
                Q_FOREACH (const QString &srvName, servicesEnabled.keys()) {
                    newAccount->selectService(m_accountManager->service(srvName));
                    newAccount->setEnabled(servicesEnabled.value(srvName));
                }
                newAccount->selectService(Accounts::Service());
                newAccount->setEnabled(enabled);
                newAccount->syncAndBlock();

                // now load the sync profile settings
                // these will be set when createProfiles() step is run.
                backupIni.beginGroup(QStringLiteral("syncSettings"));
                {
                    QMap<QString, QVariantMap> settingsMap;
                    Q_FOREACH (const QString &templateProfileName, backupIni.childGroups()) {
                        backupIni.beginGroup(templateProfileName);
                        QVariantMap profileSettings;
                        Q_FOREACH (const QString &key, backupIni.childKeys()) {
                            if (key == Buteo::KEY_ACCOUNT_ID) {
                                profileSettings.insert(key, QString::number(newAccount->id()));
                            } else {
                                profileSettings.insert(key, deduce(backupIni, key));
                            }
                        }
                        settingsMap.insert(templateProfileName, profileSettings);
                        backupIni.endGroup();
                    }
                    syncProfileSettings->insert(newAccount->id(), settingsMap);
                }
                backupIni.endGroup();

                // now load the sync schedule
                backupIni.beginGroup(QStringLiteral("syncSchedule"));
                {
                    QMap<QString, QString> scheduleXmlMap;
                    Q_FOREACH (const QString &templateProfileName, backupIni.childGroups()) {
                        backupIni.beginGroup(templateProfileName);
                        scheduleXmlMap.insert(templateProfileName, backupIni.value(QStringLiteral("xml")).toString());
                        backupIni.endGroup();
                    }
                    syncScheduleXml->insert(newAccount->id(), scheduleXmlMap);
                }
                backupIni.endGroup();

                // finished.
                m_oldAccountIds.append(oldAccountId.toUInt());
                m_newAccountIds.append(newAccount->id());
                newAccount->deleteLater();
            }
        }
        backupIni.endGroup();
    }

    return true;
}

void AccountBackupRestorer::backupAccountServiceSettings(QSettings &backupIni,
                                                         const Accounts::ServiceList &accountServices,
                                                         const Accounts::Service &srv,
                                                         Accounts::Account *account,
                                                         QList<SignOnCredentials> &requiredCredentials,
                                                         const QString &clientId,
                                                         const QString &clientSecret,
                                                         const QString &consumerKey,
                                                         const QString &consumerSecret)
{
    account->selectService(srv);
    backupIni.beginGroup(srv.isValid() ? srv.name() : QStringLiteral("defaultService"));
    Q_FOREACH (const QString &key, account->allKeys()) {
        // Note: we ignore the per-account profile_id setting
        // because the profile will have to be regenerated for
        // the account anyway.
        if (!key.endsWith(QStringLiteral("profile_id"))) {
            backupIni.setValue(key, account->value(key, QVariant()));
        }
    }
    backupIni.endGroup();

    // If this is the default service, we need to find the default signon service
    // for the account.  Its name should end in -signon or -sync.
    int signonServiceIndex = -1;
    if (!srv.isValid()) {
        for (int i = 0; i < accountServices.size(); ++i) {
            if (accountServices[i].name().toLower().endsWith(QStringLiteral("-signon")) ||
                accountServices[i].name().toLower().endsWith(QStringLiteral("-sync"))) {
                // found the default signon service.
                signonServiceIndex = i;
            }
        }
    }

    // ensure that we backup the credentials needed for this account/service.
    if (account->credentialsId() && (srv.isValid() || signonServiceIndex >= 0)) {
        Accounts::AccountService accSrv(account, srv.isValid() ? srv : accountServices[signonServiceIndex]);
        QVariantMap signonSessionData = accSrv.authData().parameters();
        signonSessionData.insert("UiPolicy", SignOn::NoUserInteractionPolicy);
        if (!clientId.isEmpty())     signonSessionData.insert("ClientId", clientId);
        if (!clientSecret.isEmpty()) signonSessionData.insert("ClientSecret", clientSecret);
        if (!consumerKey.isEmpty())    signonSessionData.insert("ConsumerKey", consumerKey);
        if (!consumerSecret.isEmpty()) signonSessionData.insert("ConsumerSecret", consumerSecret);

        SignOnCredentials soc;
        soc.id = account->credentialsId();
        soc.method = accSrv.authData().method();
        soc.mechanism = accSrv.authData().mechanism();
        soc.sessionData = signonSessionData;

        // while theoretically there could be different credentials
        // stored for different session data, we assume that if we
        // have already retrieved the credentials for this combination
        // of id/method/mechanism that we don't need to do so again.
        bool found = false;
        Q_FOREACH (const SignOnCredentials &rsoc, requiredCredentials) {
            if (rsoc.id == soc.id && rsoc.method == soc.method && rsoc.mechanism == soc.mechanism) {
                found = true;
                break;
            }
        }

        if (!found) {
            requiredCredentials.append(soc);
        }
    }
}

void AccountBackupRestorer::restoreAccountServiceSettings(QSettings &backupIni,
                                                          const Accounts::Service &srv,
                                                          Accounts::Account *account,
                                                          const QString &oldAccountId,
                                                          const QMap<quint32, quint32> &oldToNewCredentialsIds,
                                                          QMap<QString, bool> *servicesEnabled,
                                                          const QString &sourceServiceName)
{
    account->selectService(srv);
    backupIni.beginGroup(!sourceServiceName.isEmpty()
                         ? sourceServiceName
                         : (srv.isValid() ? srv.name() : QStringLiteral("defaultService")));
    Q_FOREACH (const QString &key, backupIni.allKeys()) {
        if (key.contains(QLatin1String("CredentialsId")) ||
            key.contains(QLatin1String("segregated_credentials"))) {
            // update credentials id to the new one.
            quint32 oldacid = backupIni.value(key).toUInt();
            if (oldacid) {
                if (oldToNewCredentialsIds.contains(oldacid)) {
                    account->setValue(key, QVariant::fromValue<quint32>(oldToNewCredentialsIds.value(oldacid)));
                } else {
                    qWarning() << Q_FUNC_INFO << "unable to restore credentials" << oldacid
                               << "for old account" << oldAccountId << "(" << account->id() << ")";
                }
            }
        } else if (key == QLatin1String("enabled") && srv.isValid()) {
            servicesEnabled->insert(srv.name(), backupIni.value(key).toBool());
        } else {
            account->setValue(key, deduce(backupIni, key));
        }
    }
    backupIni.endGroup();
}

QMap<quint32, quint32> AccountBackupRestorer::createCredentials(const QVariantMap &allCredentialsSettings)
{
    QMap<quint32, quint32> oldToNewIds;

    Q_FOREACH (const QString &oldCredIdStr, allCredentialsSettings.keys()) {
        QVariantMap infoAndMethodMechSecrets = allCredentialsSettings.value(oldCredIdStr).toMap();
        QVariantMap infoValues = infoAndMethodMechSecrets.value(QStringLiteral("infoValues")).toMap();
        QVariantMap methodMechSecrets = infoAndMethodMechSecrets.value(QStringLiteral("methodMechanismSecrets")).toMap();

        // some ugliness to create the credentials "synchronously"
        QThread thread;
        CredentialCreationRequest *request = new CredentialCreationRequest(infoValues, methodMechSecrets);
        connect(&thread, SIGNAL(finished()), request, SLOT(deleteLater()));
        request->moveToThread(&thread);
        QTimer::singleShot(5, request, SLOT(createCredentials()));
        thread.start();
        while (!request->finished()) {
            QCoreApplication::processEvents();
            QThread::msleep(100);
        }
        if (!request->error()) {
            quint32 newCredId = request->newCredentialsId();
            oldToNewIds.insert(oldCredIdStr.toUInt(), newCredId);
        }
        thread.quit();
        thread.wait();
    }

    return oldToNewIds;
}

QMap<QString, QString> AccountBackupRestorer::syncProfileSettings(Accounts::Account *account,
                                                                  const Accounts::Service &srv,
                                                                  const QString &templateProfileName)
{
    Q_FOREACH (const QString &profileId, m_syncManager->profileIds(account->id(), srv.name())) {
        if (profileId.contains(templateProfileName)) {
            // found the per-account profile for this template
            // load its properties and return them.
            return m_syncManager->profileProperties(profileId);
        }
    }

    return QMap<QString, QString>();
}

QString AccountBackupRestorer::syncScheduleXml(Accounts::Account *account,
                                               const Accounts::Service &srv,
                                               const QString &templateProfileName)
{
    Q_FOREACH (const QString &profileId, m_syncManager->profileIds(account->id(), srv.name())) {
        if (profileId.contains(templateProfileName)) {
            // found the per-account profile for this template
            // return the schedule xml
            return m_syncManager->syncScheduleXml(profileId);
        }
    }
    return QString();
}

//----- to turn asynchronous credentials info query into synchronous method call:
CredentialKeysQuery::CredentialKeysQuery(int credentialsId,
                                         const QString &method,
                                         const QString &mechanism,
                                         const QVariantMap &sessionData,
                                         QObject *parent)
    : QObject(parent)
    , m_finished(false)
    , m_error(false)
    , m_credentialsId(credentialsId)
    , m_method(method)
    , m_mechanism(mechanism)
    , m_sessionData(sessionData)
    , m_identity(0)
{
}

CredentialKeysQuery::~CredentialKeysQuery()
{
    delete m_identity;
}

bool CredentialKeysQuery::finished() const
{
    QMutexLocker locker(&m_mutex);
    return m_finished;
}

bool CredentialKeysQuery::error() const
{
    QMutexLocker locker(&m_mutex);
    return m_error;
}

QVariantMap CredentialKeysQuery::infoValues() const
{
    QMutexLocker locker(&m_mutex);
    return m_infoValues;
}

QVariantMap CredentialKeysQuery::methodMechanismSecrets() const
{
    QMutexLocker locker(&m_mutex);
    return m_methodMechanismSecrets;
}

void CredentialKeysQuery::queryCredentials()
{
    QMutexLocker locker(&m_mutex);
    m_identity = SignOn::Identity::existingIdentity(m_credentialsId);
    if (!m_identity) {
        // not a valid identity.
        m_error = true;
        m_finished = true;
    } else {
        // step one, query the info from the credentials
        connect(m_identity, SIGNAL(info(SignOn::IdentityInfo)), this, SLOT(credentialsInfo(SignOn::IdentityInfo)));
        m_identity->queryInfo();
    }
}

void CredentialKeysQuery::credentialsInfo(const SignOn::IdentityInfo &info)
{
    QMutexLocker locker(&m_mutex);
    m_identity->disconnect(this);

    QVariantMap methodMechanisms;
    foreach (const QString &method, info.methods()) {
        methodMechanisms.insert(method, info.mechanisms(method));
    }

    m_infoValues.insert(QStringLiteral("userName"), QVariant::fromValue<QString>(info.userName()));
    m_infoValues.insert(QStringLiteral("caption"), QVariant::fromValue<QString>(info.caption()));
    m_infoValues.insert(QStringLiteral("realms"), QVariant::fromValue<QStringList>(info.realms()));
    m_infoValues.insert(QStringLiteral("owner"), QVariant::fromValue<QString>(info.owner()));
    m_infoValues.insert(QStringLiteral("accessControlList"), QVariant::fromValue<QStringList>(info.accessControlList()));
    m_infoValues.insert(QStringLiteral("methodMechanisms"), QVariant::fromValue<QVariantMap>(methodMechanisms));
    m_infoValues.insert(QStringLiteral("type"), QVariant::fromValue<int>(static_cast<int>(info.type())));

    // now we need to retrieve the secret / access tokens / whatever, for the current method/mechanism.
    SignOn::AuthSession *session = m_identity->createSession(m_method);
    if (!session) {
        qWarning() << Q_FUNC_INFO << "unable to retrieve secrets for credentials" << m_credentialsId << "using method:" << m_method;
        m_error = true;
        m_finished = true;
        return;
    }

    connect(session, SIGNAL(response(SignOn::SessionData)), this, SLOT(signOnResponse(SignOn::SessionData)));
    connect(session, SIGNAL(error(SignOn::Error)), this, SLOT(signOnError(SignOn::Error)));
    session->process(m_sessionData, m_mechanism);
}

void CredentialKeysQuery::signOnError(const SignOn::Error &error)
{
    QMutexLocker locker(&m_mutex);
    SignOn::AuthSession *session = qobject_cast<SignOn::AuthSession*>(sender());
    m_identity->destroySession(session);

    qWarning() << Q_FUNC_INFO << "signon error while retrieving secrets for credentials" << m_credentialsId << ":" << error.message();
    m_error = true;
    m_finished = true;
}

void CredentialKeysQuery::signOnResponse(const SignOn::SessionData &responseData)
{
    QMutexLocker locker(&m_mutex);
    SignOn::AuthSession *session = qobject_cast<SignOn::AuthSession*>(sender());
    m_identity->destroySession(session);

    QVariantMap mechanismSecrets;
    QVariantMap secret;
    Q_FOREACH (const QString &key, responseData.propertyNames()) {
        secret.insert(key, responseData.getProperty(key));
    }
    QVariantMap providedTokensSessionData = m_sessionData;
    providedTokensSessionData.insert(QStringLiteral("method"), m_method);
    providedTokensSessionData.insert(QStringLiteral("mechanism"), m_mechanism);
    secret.insert(QStringLiteral("sessionData"), providedTokensSessionData);
    mechanismSecrets.insert(m_mechanism, secret);
    m_methodMechanismSecrets.insert(m_method, mechanismSecrets);

    m_error = false;
    m_finished = true;
}



//----- to turn asynchronous credentials creation into synchronous method call:
CredentialCreationRequest::CredentialCreationRequest(const QVariantMap &infoValues,
                                                     const QVariantMap &methodMechSecrets,
                                                     QObject *parent)
    : QObject(parent)
    , m_finished(false)
    , m_error(false)
    , m_credentialsId(0)
    , m_infoValues(infoValues)
    , m_methodMechSecrets(methodMechSecrets)
    , m_identity(0)
{
}

CredentialCreationRequest::~CredentialCreationRequest()
{
    delete m_identity;
}

void CredentialCreationRequest::createCredentials()
{
    QString userName = m_infoValues.value(QStringLiteral("userName")).toString();
    QString caption = m_infoValues.value(QStringLiteral("caption")).toString();
    QStringList realms = m_infoValues.value(QStringLiteral("realms")).toStringList();
    QString owner = m_infoValues.value(QStringLiteral("owner")).toString();
    QStringList accessControlList = m_infoValues.value(QStringLiteral("accessControlList")).toStringList();
    QVariantMap methodMechanisms = m_infoValues.value(QStringLiteral("methodMechanisms")).toMap();
    int type = m_infoValues.value(QStringLiteral("type")).toInt();

    // here we scan for username and password information, and also provided tokens.
    QString scannedUsername;
    QString scannedSecret;
    Q_FOREACH (const QString &method, m_methodMechSecrets.keys()) {
        QVariantMap mechanismSecrets = m_methodMechSecrets.value(method).toMap();
        Q_FOREACH (const QString &mechanism, mechanismSecrets.keys()) {
            QVariantMap secrets = mechanismSecrets.value(mechanism).toMap();
            QVariantMap providedTokens;
            Q_FOREACH (const QString &key, secrets.keys()) {
                // If the secrets map contains username/password information, store it.
                if (key.toLower() == QStringLiteral("username")) {
                    scannedUsername = secrets.value(key).toString().isEmpty() ? scannedUsername : secrets.value(key).toString();
                } else if (key.toLower() == QStringLiteral("password")) {
                    scannedSecret = secrets.value(key).toString().isEmpty()   ? scannedSecret   : secrets.value(key).toString();
                } else if (key.toLower() == QStringLiteral("secret")) {
                    scannedSecret = secrets.value(key).toString().isEmpty()   ? scannedSecret   : secrets.value(key).toString();
                }

                // If the secrets map contains tokens, cache them
                if (key.toLower() != QStringLiteral("sessionData")) {
                    providedTokens.insert(key, secrets.value(key));
                }
            }
            // queue the provided tokens (along with session data) for OAuth2/OAuth1.0a credentials
            if (secrets.contains(QStringLiteral("sessionData")) && method.toLower() != QStringLiteral("password")) {
                m_providedTokensQueue.append(qMakePair(secrets.value(QStringLiteral("sessionData")).toMap(), providedTokens));
            }
        }
    }

    // fill out some variables required to create the identity info.
    if (userName.isEmpty() && !scannedUsername.isEmpty()) {
        userName = scannedUsername;
    }

    QMap<QString, QStringList> methodMechs;
    Q_FOREACH (const QString &method, methodMechanisms.keys()) {
        methodMechs.insert(method, methodMechanisms.value(method).toStringList());
    }

    // now we have enough information to create the identity info.
    SignOn::IdentityInfo newInfo(caption, userName, methodMechs);
    if (!scannedSecret.isEmpty()) {
        newInfo.setSecret(scannedSecret, true);
    }
    newInfo.setRealms(realms);
    newInfo.setOwner(owner);
    newInfo.setAccessControlList(accessControlList);
    newInfo.setType(static_cast<SignOn::IdentityInfo::CredentialsType>(type));

    // and we can create the new identity based on the info.
    m_identity = SignOn::Identity::newIdentity(newInfo);
    connect(m_identity, SIGNAL(credentialsStored(quint32)), this, SLOT(credentialsStored(quint32)));
    connect(m_identity, SIGNAL(error(SignOn::Error)), this, SLOT(credentialsError(SignOn::Error)));
    m_identity->storeCredentials(newInfo);
}

bool CredentialCreationRequest::finished() const
{
    QMutexLocker locker(&m_mutex);
    return m_finished;
}

bool CredentialCreationRequest::error() const
{
    QMutexLocker locker(&m_mutex);
    return m_error;
}

quint32 CredentialCreationRequest::newCredentialsId() const
{
    QMutexLocker locker(&m_mutex);
    return m_credentialsId;
}

void CredentialCreationRequest::storeProvidedTokens()
{
    QPair<QVariantMap, QVariantMap> sdpt = m_providedTokensQueue.takeFirst();
    QVariantMap sessionData = sdpt.first;
    QVariantMap providedTokens = sdpt.second;
    QString method = sessionData.value(QStringLiteral("method")).toString();
    QString mechanism = sessionData.value(QStringLiteral("mechanism")).toString();
    sessionData.remove(QStringLiteral("method"));
    sessionData.remove(QStringLiteral("mechanism"));
    sessionData.insert(QStringLiteral("ProvidedTokens"), providedTokens);

    if (method.isEmpty()) {
        qWarning() << Q_FUNC_INFO << "no method associated with session data, cannot store tokens for credentials";
        m_error = true;
        m_finished = true;
        return;
    }

    SignOn::AuthSession *session = m_identity->createSession(method);
    if (!session) {
        qWarning() << Q_FUNC_INFO << "failed to create session to store tokens for credentials";
        m_error = true;
        m_finished = true;
        return;
    }

    connect(session, SIGNAL(response(SignOn::SessionData)), this, SLOT(signOnResponse(SignOn::SessionData)));
    connect(session, SIGNAL(error(SignOn::Error)), this, SLOT(signOnError(SignOn::Error)));
    session->process(sessionData, mechanism);
}

void CredentialCreationRequest::credentialsStored(quint32 id)
{
    QMutexLocker locker(&m_mutex);
    m_credentialsId = id;

    // check to see if we need to store any tokens via the provided tokens hook
    if (!m_providedTokensQueue.isEmpty()) {
        storeProvidedTokens();
    } else {
        // otherwise, we're finished
        m_error = false;
        m_finished = true;
    }
}

void CredentialCreationRequest::credentialsError(const SignOn::Error &error)
{
    QMutexLocker locker(&m_mutex);
    qWarning() << Q_FUNC_INFO << "error storing credentials:" << error.message();
    m_error = true;
    m_finished = true;
}

void CredentialCreationRequest::signOnResponse(const SignOn::SessionData &)
{
    QMutexLocker locker(&m_mutex);
    SignOn::AuthSession *session = qobject_cast<SignOn::AuthSession*>(sender());
    m_identity->destroySession(session);

    // successfully stored the provided tokens.
    // check to see if we need to store any (more) tokens via the provided tokens hook
    if (!m_providedTokensQueue.isEmpty()) {
        storeProvidedTokens();
    } else {
        // otherwise, we're finished
        m_finished = true;
    }
}

void CredentialCreationRequest::signOnError(const SignOn::Error &error)
{
    QMutexLocker locker(&m_mutex);
    qWarning() << Q_FUNC_INFO << "error storing provided tokens:" << error.message();
    SignOn::AuthSession *session = qobject_cast<SignOn::AuthSession*>(sender());
    m_identity->destroySession(session);

    // check to see if we need to store any (more) tokens via the provided tokens hook
    if (!m_providedTokensQueue.isEmpty()) {
        storeProvidedTokens();
    } else {
        // otherwise, we're finished
        m_error = true;
        m_finished = true;
    }
}

