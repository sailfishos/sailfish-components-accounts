/*
 * Copyright (C) 2013-2019 Jolla Ltd.
 * Copyright (C) 2019 Open Mobile Platform LLC
 * Contact: Bea Lam <bea.lam@jollamobile.com>
 *
 * License: Proprietary
 */

#include "accountsyncmanager.h"
#include "accountsyncoptions_p.h"

// buteo-syncfw
#include <ProfileEngineDefs.h>
#include <SyncProfile.h>
#include <SyncCommonDefs.h>
#include <ProfileManager.h>
#include <buteosyncfw5/SyncClientInterface.h>

// libaccounts-qt
#include <Accounts/Account>
#include <Accounts/AccountService>
#include <Accounts/Manager>

#include <QDebug>
#include <QSet>
#include <QList>
#include <QHash>

#include <QStandardPaths>
#include <QFileInfo>
#include <QFile>
#include <QDir>

namespace {

const QString SyncProfileTemplatesKey = QStringLiteral("sync_profile_templates");

QString SyncProfileIdKey(const QString &templateProfileName) {
    return QStringLiteral("%1/%2").arg(templateProfileName).arg(Buteo::KEY_PROFILE_ID);
}

}

QStringList AccountSyncManager::BackupRestoreOptions::localDirFileNames() const
{
    QDir localBackupDir(localDirPath);
    if (localBackupDir.exists()) {
        return localBackupDir.entryList(QDir::Files);
    }
    return QStringList();
}

class AccountSyncProfileManagerPrivate : public QObject
{
    Q_OBJECT
public:
    class ProfileCreationDetails
    {
    public:
        ProfileCreationDetails()
            : accountId(0) {}
        ProfileCreationDetails(const QString &profId, int accId, const QString &service)
            : profileId(profId), accountId(accId), serviceName(service) {}
        ProfileCreationDetails(int accId, const QHash<QString, QStringList> &multiple)
            : multipleCreatedProfiles(multiple), accountId(accId) {}
        ~ProfileCreationDetails() {}

        QHash<QString, QStringList> multipleCreatedProfiles;
        QString profileId;
        int accountId;
        QString serviceName;
    };

    AccountSyncProfileManagerPrivate(AccountSyncManager *parent = 0);
    ~AccountSyncProfileManagerPrivate();

    void finalizeProfileCreation(Accounts::Account *account, const QString &serviceName, const QString &profileId);
    void finalizeProfileCreation(Accounts::Account *account, const QHash<QString, QStringList> &multipleCreatedProfiles);
    bool startSync(const QString &profileId);

    bool syncProfileFileExists(const QString &profileId) const;
    QStringList syncProfileIds(Accounts::Account *account, const Accounts::Service &srv, const QString &templateProfileMatch = QString()) const;

    QList<ProfileCreationDetails> profilesUnderCreation;
    AccountSyncManager *q;
    Accounts::Manager *m_accountManager;
    Buteo::ProfileManager *m_profileManager;
    Buteo::SyncClientInterface *m_buteoClient;

public slots:
    void handleAccountSynced();
    void handleAccountSyncError();
    void handleSyncStatus(const QString &profileId, int status, const QString &message, int statusDetails);

private:
    ProfileCreationDetails popProfileCreationDetails(Accounts::Account *account);
};

AccountSyncProfileManagerPrivate::AccountSyncProfileManagerPrivate(AccountSyncManager *parent)
    : QObject(parent)
    , q(parent)
    , m_accountManager(new Accounts::Manager)
    , m_profileManager(new Buteo::ProfileManager)
    , m_buteoClient(0)
{
    m_buteoClient = new Buteo::SyncClientInterface;
    connect(m_buteoClient, SIGNAL(syncStatus(QString,int,QString,int)),
            this, SLOT(handleSyncStatus(QString,int,QString,int)));
}

AccountSyncProfileManagerPrivate::~AccountSyncProfileManagerPrivate()
{
    delete m_accountManager;
    delete m_profileManager;
    delete m_buteoClient;
}

void AccountSyncProfileManagerPrivate::finalizeProfileCreation(Accounts::Account *account, const QString &serviceName, const QString &profileId)
{
    ProfileCreationDetails details(profileId, account->id(), serviceName);
    profilesUnderCreation.append(details);
    connect(account, SIGNAL(synced()), this, SLOT(handleAccountSynced()));
    connect(account, SIGNAL(error(Accounts::Error)), this, SLOT(handleAccountSyncError()));
    account->sync();
}

void AccountSyncProfileManagerPrivate::finalizeProfileCreation(Accounts::Account *account, const QHash<QString, QStringList> &multipleCreatedProfiles)
{
    ProfileCreationDetails details(account->id(), multipleCreatedProfiles);
    profilesUnderCreation.append(details);
    connect(account, SIGNAL(synced()), this, SLOT(handleAccountSynced()));
    connect(account, SIGNAL(error(Accounts::Error)), this, SLOT(handleAccountSyncError()));
    account->sync();
}

bool AccountSyncProfileManagerPrivate::startSync(const QString &profileId)
{
    if (!m_buteoClient->startSync(profileId)) {
        return false;
    }
    return true;
}

bool AccountSyncProfileManagerPrivate::syncProfileFileExists(const QString &profileId) const
{
    QString path = Sync::syncCacheDir()
            + QDir::separator() + QStringLiteral("sync")
            + QDir::separator() + profileId + QStringLiteral(".xml");
    return QFile::exists(path);
}

QStringList AccountSyncProfileManagerPrivate::syncProfileIds(Accounts::Account *account, const Accounts::Service &srv, const QString &templateProfileMatch) const
{
    if (!account || !srv.isValid()) {
        return QStringList();
    }
    Accounts::Service prevService = account->selectedService();
    account->selectService(srv);
    QStringList retn;
    QStringList syncProfileTemplates = account->value(SyncProfileTemplatesKey).toStringList();
    Q_FOREACH(const QString &syncProfileTemplate, syncProfileTemplates) {
        if (!templateProfileMatch.isEmpty() && syncProfileTemplate != templateProfileMatch) {
            continue;
        }
        QString profileId = account->value(SyncProfileIdKey(syncProfileTemplate)).toString();
        if (!profileId.isEmpty() && syncProfileFileExists(profileId)) {
            retn.append(profileId);
        }
    }
    account->selectService(prevService);
    return retn;
}

void AccountSyncProfileManagerPrivate::handleAccountSynced()
{
    Accounts::Account *account = qobject_cast<Accounts::Account *>(sender());
    ProfileCreationDetails details = popProfileCreationDetails(account);
    if (!details.multipleCreatedProfiles.isEmpty()) {
        QStringList profileIds;
        Q_FOREACH (const QString &serviceName, details.multipleCreatedProfiles.keys()) {
            profileIds.append(details.multipleCreatedProfiles[serviceName]);
        }
        emit q->allProfilesCreated(account->id(), profileIds);
    } else if (!details.profileId.isEmpty()) {
        emit q->profileCreated(details.profileId);
    } else {
        qWarning() << "AccountSyncProfileManager: invalid sync case!";
    }
}

void AccountSyncProfileManagerPrivate::handleAccountSyncError()
{
    QString errorString = QStringLiteral("Unable to sync account after profile creation");
    Accounts::Account *account = qobject_cast<Accounts::Account *>(sender());
    ProfileCreationDetails details = popProfileCreationDetails(account);
    if (!details.multipleCreatedProfiles.isEmpty()) {
        emit q->allProfileCreationError(details.accountId, errorString);
    } else if (!details.profileId.isEmpty()) {
        emit q->profileCreationError(details.accountId, details.serviceName, errorString);
    } else {
        qWarning() << "AccountSyncProfileManager: invalid sync error case!";
    }
}

void AccountSyncProfileManagerPrivate::handleSyncStatus(const QString &profileId, int status, const QString &message, int statusDetails)
{
    switch (status) {
    case Sync::SYNC_QUEUED:
    case Sync::SYNC_PROGRESS:
        // no action
        break;
    case Sync::SYNC_STARTED:
        emit q->profileSyncStatusChanged(profileId, AccountSyncManager::SyncStarted, message);
        break;
    case Sync::SYNC_DONE:
        emit q->profileSyncStatusChanged(profileId, AccountSyncManager::SyncFinished, message);
        break;
    case Sync::SYNC_ABORTED:
    case Sync::SYNC_CANCELLED:
        emit q->profileSyncStatusChanged(profileId, AccountSyncManager::SyncAborted, message);
        break;
    case Sync::SYNC_ERROR:
    case Sync::SYNC_STOPPING:
    case Sync::SYNC_NOTPOSSIBLE:
    case Sync::SYNC_AUTHENTICATION_FAILURE:
    case Sync::SYNC_DATABASE_FAILURE:
    case Sync::SYNC_CONNECTION_ERROR:
    case Sync::SYNC_SERVER_FAILURE:
    case Sync::SYNC_BAD_REQUEST:
        emit q->profileSyncStatusChanged(profileId, AccountSyncManager::SyncError, message + ", details:" + statusDetails);
        break;
    }
}

AccountSyncProfileManagerPrivate::ProfileCreationDetails AccountSyncProfileManagerPrivate::popProfileCreationDetails(Accounts::Account *account)
{
    if (account) {
        int accountId = account->id();
        for (int i=0; i<profilesUnderCreation.count(); i++) {
            if (profilesUnderCreation[i].accountId == accountId) {
                return profilesUnderCreation.takeAt(i);
            }
        }
    }
    return ProfileCreationDetails();
}


AccountSyncManager::AccountSyncManager(QObject *parent)
    : QObject(parent)
    , d(new AccountSyncProfileManagerPrivate(this))
{
}

AccountSyncManager::~AccountSyncManager()
{
}

Accounts::Manager *AccountSyncManager::accountManager() const
{
    return d->m_accountManager;
}

void AccountSyncManager::createProfile(const QString &templateProfileName, int accountId, const QString &serviceName)
{
    Accounts::Account *account = Accounts::Account::fromId(d->m_accountManager, accountId, this);
    if (!account) {
        emit profileCreationError(accountId, serviceName, QString("Cannot find account with id: %1").arg(accountId));
        return;
    }
    Accounts::Service srv = d->m_accountManager->service(serviceName);
    if (!srv.isValid()) {
        emit profileCreationError(accountId, serviceName, QString("Unable to load service: %1").arg(serviceName));
        return;
    }
    QString profileId = createProfile(templateProfileName, account, srv, true);
    if (profileId.isEmpty()) {
        emit profileCreationError(accountId, serviceName, QString("Unable to create sync profile"));
        return;
    }
    d->finalizeProfileCreation(account, serviceName, profileId);
}

void AccountSyncManager::updateProfile(const QString &profileId, const QVariantMap &properties, AccountSyncOptions *options)
{
    if (profileId.isEmpty()) {
        emit profileUpdateError(profileId, QStringLiteral("specified profileId is an empty string"));
        return;
    }
    if (!updateSyncProfile(profileId, properties, options)) {
        emit profileUpdateError(profileId, QStringLiteral("Unable to update sync profile"));
        return;
    }
    emit profileUpdated(profileId);
}

void AccountSyncManager::syncProfile(const QString &profileId)
{
    if (profileId.isEmpty()) {
        emit profileUpdateError(profileId, QStringLiteral("specified profileId is an empty string"));
        return;
    }
    if (!d->startSync(profileId)) {
        emit profileSyncStatusChanged(profileId, SyncError, QStringLiteral("Unable to start sync!"));
    }
}

int AccountSyncManager::createAllProfiles(int accountId)
{
    int createdCount = 0;
    QHash<QString, QStringList> createdProfiles;
    Accounts::Account *account = Accounts::Account::fromId(d->m_accountManager, accountId, this);
    if (account) {
        Q_FOREACH (const Accounts::Service &srv, account->services()) {
            account->selectService(srv);
            if (account->enabled()) {
                Q_FOREACH (const QString &templateProfile, defaultTemplateProfiles(account, srv)) {
                    QString savedProfileId = account->value(SyncProfileIdKey(templateProfile)).toString();
                    if (savedProfileId.isEmpty() || !d->syncProfileFileExists(savedProfileId)) {
                        savedProfileId = createProfile(templateProfile, account, srv, account->enabled());
                        createdProfiles[srv.name()].append(savedProfileId);
                        createdCount++;
                    }
                }
            }
            account->selectService();
        }
    }
    if (createdCount > 0) {
        d->finalizeProfileCreation(account, createdProfiles);
    }
    return createdCount;
}

QStringList AccountSyncManager::profileIds(int accountId, const QString &serviceName) const
{
    AccountSyncManager *parentPtr = const_cast<AccountSyncManager*>(this);
    Accounts::Account *account = Accounts::Account::fromId(d->m_accountManager, accountId, parentPtr);
    if (account) {
        if (serviceName.isEmpty()) {
            QStringList ret;
            Q_FOREACH (const Accounts::Service &srv, account->services()) {
                account->selectService(srv);
                ret << d->syncProfileIds(account, srv);
            }
            return ret;
        } else {
            Accounts::Service srv = d->m_accountManager->service(serviceName);
            if (srv.isValid()) {
                return d->syncProfileIds(account, srv);
            }
        }
    }
    return QStringList();
}

AccountSyncOptions *AccountSyncManager::accountSyncOptions(const QString &profileId)
{
    Buteo::SyncProfile *profile = d->m_profileManager->syncProfile(profileId);
    if (!profile) {
        qWarning() << "Invalid profile name:" << profileId;
        return 0;
    }
    return AccountSyncOptionsPrivate::fromButeoProfile(*profile, this);
}

bool AccountSyncManager::templateProfilesAvailable(const QStringList &templateProfiles) const
{
    Q_FOREACH (const QString &profileName, templateProfiles) {
        Buteo::SyncProfile *profile = d->m_profileManager->syncProfile(profileName);
        bool exists = (profile != 0);
        delete profile;
        if (!exists) {
            return false;
        }
    }
    return true;
}

QStringList AccountSyncManager::defaultTemplateProfiles(int accountId, const QString &serviceName) const
{
    Accounts::Service srv = d->m_accountManager->service(serviceName);
    if (!srv.isValid()) {
        qWarning() << "Cannot find service with name:" << serviceName;
        return QStringList();
    }
    AccountSyncManager *parentPtr = const_cast<AccountSyncManager*>(this);
    return defaultTemplateProfiles(Accounts::Account::fromId(d->m_accountManager, accountId, parentPtr), srv);
}

QString AccountSyncManager::createProfile(const QString &templateProfileName,
                                          Accounts::Account *account,
                                          const Accounts::Service &srv,
                                          bool enableProfile,
                                          const QVariantMap &properties)
{
    Buteo::SyncProfile *profile = newProfileFromTemplate(templateProfileName,
                                                         account,
                                                         srv,
                                                         enableProfile,
                                                         properties);
    if (!profile) {
        return QString();
    }
    QString profileId = d->m_profileManager->updateProfile(*profile);
    if (profileId.isEmpty()) {
        qWarning() << "Unable to save sync profile with ProfileManager for template:" << templateProfileName;
    } else {
        Accounts::Service prevService = account->selectedService();
        account->selectService(srv);
        account->setValue(SyncProfileIdKey(templateProfileName), profile->name());
        account->selectService(prevService);
    }
    return profileId;
}

bool AccountSyncManager::checkProfile(const QString &templateProfileName,
                                      Accounts::Account *account,
                                      const Accounts::Service &srv)
{
    // check that the profile hasn't been corrupted.  returns true if it's ok.
    if (!account) {
        qWarning() << "Invalid account, cannot check profile";
        return false;
    }
    if (!srv.isValid()) {
        qWarning() << "Invalid service, cannot check profile";
        return false;
    }
    QString expectedProfileName = QStringLiteral("%1-%2").arg(templateProfileName).arg(account->id());
    QString fullPath = QStringLiteral("%1/.cache/msyncd/sync/%2.xml")
            .arg(QStandardPaths::writableLocation(QStandardPaths::HomeLocation))
            .arg(expectedProfileName);

    if (!d->syncProfileFileExists(expectedProfileName)) {
        // profile does not exist
        qWarning() << "Profile does not exist:" << fullPath;
        return false;
    }

    QFileInfo profileInfo(fullPath);
    if (profileInfo.size() == 0) {
        // profile is corrupted
        if (!QFile::remove(fullPath)) {
            qWarning() << "Profile is corrupted and cannot be removed:" << fullPath;
        } else {
            qWarning() << "Profile is corrupted:" << fullPath;
        }
        return false;
    }

    // TODO: more comprehensive corruption checks?  E.G. well-formed XML?

    // profile exists and is not corrupted.
    return true;
}

Buteo::SyncProfile *AccountSyncManager::newProfileFromTemplate(const QString &templateProfileName,
                                                               Accounts::Account *account,
                                                               const Accounts::Service &srv,
                                                               bool enableProfile,
                                                               const QVariantMap &properties)
{
    return newProfileFromTemplate(templateProfileName,
                                  account,
                                  srv,
                                  enableProfile,
                                  properties,
                                  QString());
}

Buteo::SyncProfile *AccountSyncManager::newProfileFromTemplate(const QString &templateProfileName,
                                           Accounts::Account *account,
                                           const Accounts::Service &srv,
                                           bool enableProfile,
                                           const QVariantMap &properties,
                                           const QString &scheduleXml)
{
    if (!account || !srv.isValid()) {
        qWarning() << "Invalid account or service";
        return 0;
    }
    if (templateProfileName.isEmpty()) {
        qWarning() << "Invalid templateProfileName";
        return 0;
    }

    Accounts::Service prevService = account->selectedService();
    account->selectService(srv);

    Buteo::SyncProfile *templateProfile = d->m_profileManager->syncProfile(templateProfileName);
    if (!templateProfile) {
        account->selectService(prevService);
        qWarning() << "Unable to load template profile:" << templateProfileName;
        return 0;
    }

    Buteo::SyncProfile *profile = templateProfile->clone();
    if (!profile) {
        delete templateProfile;
        account->selectService(prevService);
        qWarning() << "unable to clone template profile:" << templateProfileName;
        return 0;
    }

    QString accountIdStr = QString::number(account->id());
    profile->setName(templateProfileName + "-" + accountIdStr);
    profile->setKey(Buteo::KEY_DISPLAY_NAME, templateProfileName + "-" + account->displayName().toHtmlEscaped());
    profile->setKey(Buteo::KEY_ACCOUNT_ID, accountIdStr);
    profile->setBoolKey(Buteo::KEY_USE_ACCOUNTS, true);
    profile->setEnabled(enableProfile);

    // set custom properties; note this may override any properties already set
    Q_FOREACH (const QString &key, properties.keys()) {
        profile->setKey(key, properties[key].toString());
    }

    if (!scheduleXml.isEmpty()) {
        QDomDocument doc;
        QString errorMsg;
        int errLine = -1;
        int errCol = -1;
        if (doc.setContent(scheduleXml, &errorMsg, &errLine, &errCol)) {
            Buteo::SyncSchedule schedule(doc.documentElement());
            profile->setSyncSchedule(schedule);
        } else {
            qWarning() << "Unable to set sync schedule for" << profile->name() << "from" << scheduleXml
                       << "Error at line" << errLine << "and column" << errCol << ":" << errorMsg;
        }
    }

    account->selectService(prevService);
    delete templateProfile;
    return profile;
}

bool AccountSyncManager::updateSyncProfile(const QString &profileId, const QVariantMap &properties, AccountSyncOptions *options)
{
    if (profileId.isEmpty()) {
        qWarning() << "Invalid profileId";
        return false;
    }
    Buteo::SyncProfile *profile = d->m_profileManager->syncProfile(profileId);
    if (!profile) {
        qWarning() << "Invalid profile name:" << profileId;
        return false;
    }
    // TODO we don't support nested profile properties yet
    Q_FOREACH (const QString &key, properties.keys()) {
        profile->setKey(key, properties[key].toString());
    }

    QString savedProfileId;
    if (options) {
        AccountSyncOptionsPrivate::writeToButeoProfile(options, profile);
        savedProfileId = d->m_profileManager->updateProfile(*profile);
        if (!d->m_buteoClient) {
            d->m_buteoClient = new Buteo::SyncClientInterface;
        }
        QString pid = profileId;    // buteo api requires in-param
        Buteo::SyncSchedule schedule = profile->syncSchedule();     // buteo api requires in-param
        if (!d->m_buteoClient->setSyncSchedule(pid, schedule)) {
            qWarning() << "Buteo::SyncClientInterface::setSyncSchedule() failed for profile" << pid;
        }
    } else {
        savedProfileId = d->m_profileManager->updateProfile(*profile);
    }
    delete profile;
    return !savedProfileId.isEmpty();
}

bool AccountSyncManager::updateBackupRestoreOptions(const QString &profileId, const BackupRestoreOptions &options)
{
    if (profileId.isEmpty()) {
        qWarning() << "Invalid profileId";
        return false;
    }
    Buteo::SyncProfile *syncProfile = d->m_profileManager->syncProfile(profileId);
    if (!syncProfile) {
        qWarning() << "Invalid profile name:" << profileId;
        return false;
    }

    Buteo::Profile *clientProfile = syncProfile->clientProfile();
    if (!clientProfile) {
        qWarning() << "Cannot find client profile in sync profile:" << syncProfile->name();
        return false;
    }

    QString operationValue;
    switch (options.operation) {
    case AccountSyncManager::BackupRestoreOptions::DirectoryListing:
        operationValue = "dir-listing";
        break;
    case AccountSyncManager::BackupRestoreOptions::Upload:
        operationValue = "upload";
        break;
    case AccountSyncManager::BackupRestoreOptions::Download:
        operationValue = "download";
        break;
    };

    clientProfile->setKey("sfos-operation", operationValue);
    clientProfile->setKey("sfos-dir-local", options.localDirPath);
    clientProfile->setKey("sfos-dir-remote", options.remoteDirPath);
    clientProfile->setKey("sfos-filename", options.fileName);

    QString savedProfileId = d->m_profileManager->updateProfile(*syncProfile);
    delete syncProfile;
    return !savedProfileId.isEmpty();
}

AccountSyncManager::BackupRestoreOptions AccountSyncManager::backupRestoreOptions(const QString &profileId, bool *ok) const
{
    if (profileId.isEmpty()) {
        qWarning() << "Invalid profileId";
        return BackupRestoreOptions();
    }
    Buteo::SyncProfile *syncProfile = d->m_profileManager->syncProfile(profileId);
    if (!syncProfile) {
        qWarning() << "Invalid profile name:" << profileId;
        return BackupRestoreOptions();
    }

    Buteo::Profile *clientProfile = syncProfile->clientProfile();
    if (!clientProfile) {
        qWarning() << "Cannot find client profile in sync profile:" << syncProfile->name();
        return BackupRestoreOptions();
    }

    BackupRestoreOptions options;
    QString operation = clientProfile->key("sfos-operation");
    options.localDirPath = clientProfile->key("sfos-dir-local");
    options.remoteDirPath = clientProfile->key("sfos-dir-remote");  // can be empty to use defaults
    options.fileName = clientProfile->key("sfos-filename");     // can be empty for upload/download to use defaults

    if (operation == "dir-listing") {
        options.operation = BackupRestoreOptions::DirectoryListing;
    } else if (operation == "upload") {
        options.operation = BackupRestoreOptions::Upload;
    } else if (operation == "download") {
        options.operation = BackupRestoreOptions::Download;
    } else {
        qWarning() << "Backup/restore options for sync profile" << syncProfile->name()
                   << "has invalid operation type:" << operation;
        return BackupRestoreOptions();
    }

    if (options.localDirPath.isEmpty()) {
        qWarning() << "Backup/restore options for sync profile" << syncProfile->name()
                   << "do not specify a local directory!";
        return BackupRestoreOptions();
    }

    if (options.operation == BackupRestoreOptions::DirectoryListing && options.fileName.isEmpty()) {
        qWarning() << "Backup/restore options for sync profile" << syncProfile->name()
                   << "do not specify a file name for directory listing!";
        return BackupRestoreOptions();
    }

    if (ok) {
        *ok = true;
    }
    return options;
}

QMap<QString, QString> AccountSyncManager::profileProperties(const QString &profileId) const
{
    if (profileId.isEmpty()) {
        qWarning() << "Invalid profileId";
        return QMap<QString, QString>();
    }
    Buteo::SyncProfile *profile = d->m_profileManager->syncProfile(profileId);
    if (!profile) {
        qWarning() << "Invalid profile id:" << profileId;
        return QMap<QString, QString>();
    }
    return profile->allKeys();
}

QString AccountSyncManager::syncScheduleXml(const QString &profileId) const
{
    if (profileId.isEmpty()) {
        qWarning() << "Invalid profileId";
        return QString();
    }
    Buteo::SyncProfile *profile = d->m_profileManager->syncProfile(profileId);
    if (!profile) {
        qWarning() << "Invalid profile id:" << profileId;
        return QString();
    }
    return profile->syncSchedule().toString();
}

bool AccountSyncManager::hasProfile(Accounts::Account *account, const Accounts::Service &srv) const
{
    return !d->syncProfileIds(account, srv, QString()).isEmpty();
}

bool AccountSyncManager::hasProfile(Accounts::Account *account, const Accounts::Service &srv, const QString &templateProfile) const
{
    return !d->syncProfileIds(account, srv, templateProfile).isEmpty();
}

QStringList AccountSyncManager::defaultTemplateProfiles(Accounts::Account *account, const Accounts::Service &srv) const
{
    if (!account || !srv.isValid()) {
        return QStringList();
    }
    Accounts::Service prevService = account->selectedService();
    account->selectService(srv);
    QStringList defaultTemplates = account->value(SyncProfileTemplatesKey).toStringList();
    account->selectService(prevService);
    return defaultTemplates;
}

#include "accountsyncmanager.moc"
