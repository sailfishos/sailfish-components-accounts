/*
 * Copyright (C) 2013 Jolla Ltd.
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

static const QString SyncProfileTemplatesKey = QStringLiteral("sync_profile_templates");

static QString SyncProfileIdKey(const QString &templateProfileName)
{
    return QStringLiteral("%1/%2").arg(templateProfileName).arg(Buteo::KEY_PROFILE_ID);
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
        ~ProfileCreationDetails() {}

        QString profileId;
        int accountId;
        QString serviceName;
    };

    AccountSyncProfileManagerPrivate(AccountSyncManager *parent = 0);
    ~AccountSyncProfileManagerPrivate();

    void finalizeProfileCreation(Accounts::Account *account, const QString &serviceName, const QString &profileId);
    bool startSync(const QString &profileId);

    QStringList syncProfileIds(Accounts::Account *account, const Accounts::Service &srv) const;

    QList<ProfileCreationDetails> profilesUnderCreation;
    QSet<QString> m_profilesUnderSync;
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

bool AccountSyncProfileManagerPrivate::startSync(const QString &profileId)
{
    if (!m_buteoClient) {
        m_buteoClient = new Buteo::SyncClientInterface;
        connect(m_buteoClient, SIGNAL(syncStatus(QString,int,QString,int)),
                this, SLOT(handleSyncStatus(QString,int,QString,int)));
    }
    if (!m_buteoClient->startSync(profileId)) {
        return false;
    }
    m_profilesUnderSync.insert(profileId);
    return true;
}

QStringList AccountSyncProfileManagerPrivate::syncProfileIds(Accounts::Account *account, const Accounts::Service &srv) const
{
    if (!account || !srv.isValid()) {
        return QStringList();
    }
    Accounts::Service prevService = account->selectedService();
    account->selectService(srv);
    QStringList retn;
    QStringList syncProfileTemplates = account->value(SyncProfileTemplatesKey).toStringList();
    Q_FOREACH(const QString &syncProfileTemplate, syncProfileTemplates) {
        QString profileId = account->value(SyncProfileIdKey(syncProfileTemplate)).toString();
        if (!profileId.isEmpty()) {
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
    if (!details.profileId.isEmpty()) {
        emit q->profileCreated(details.profileId);
    }
}

void AccountSyncProfileManagerPrivate::handleAccountSyncError()
{
    Accounts::Account *account = qobject_cast<Accounts::Account *>(sender());
    ProfileCreationDetails details = popProfileCreationDetails(account);
    if (details.accountId != 0) {
        emit q->profileCreationError(details.accountId, details.serviceName, QStringLiteral("Unable to sync account after profile creation"));
    }
}

void AccountSyncProfileManagerPrivate::handleSyncStatus(const QString &profileId, int status, const QString &message, int statusDetails)
{
    if (!m_profilesUnderSync.contains(profileId)) {
        return;
    }
    switch (status) {
    case Sync::SYNC_QUEUED:
    case Sync::SYNC_PROGRESS:
        // no action
        break;
    case Sync::SYNC_STARTED:
        emit q->profileSyncStatusChanged(profileId, AccountSyncManager::SyncStarted, message);
        break;
    case Sync::SYNC_DONE:
        m_profilesUnderSync.remove(profileId);
        emit q->profileSyncStatusChanged(profileId, AccountSyncManager::SyncFinished, message);
        break;
    case Sync::SYNC_ABORTED:
    case Sync::SYNC_CANCELLED:
        m_profilesUnderSync.remove(profileId);
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
        m_profilesUnderSync.remove(profileId);
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

void AccountSyncManager::createProfile(const QString &templateProfileName, int accountId, const QString &serviceName)
{
    Accounts::Account *account = d->m_accountManager->account(accountId);
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

QStringList AccountSyncManager::profileIds(int accountId, const QString &serviceName) const
{
    Accounts::Account *account = d->m_accountManager->account(accountId);
    if (account) {
        Accounts::Service srv = d->m_accountManager->service(serviceName);
        if (srv.isValid()) {
            return d->syncProfileIds(account, srv);
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

Buteo::SyncProfile *AccountSyncManager::newProfileFromTemplate(const QString &templateProfileName,
                                                               Accounts::Account *account,
                                                               const Accounts::Service &srv,
                                                               bool enableProfile,
                                                               const QVariantMap &properties)
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

    // enable the profile schedule
    Buteo::SyncSchedule schedule = profile->syncSchedule();
    schedule.setScheduleEnabled(true);
    profile->setSyncSchedule(schedule);

    // set custom properties; note this may override any properties already set
    Q_FOREACH (const QString &key, properties.keys()) {
        profile->setKey(key, properties[key].toString());
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

bool AccountSyncManager::hasProfile(Accounts::Account *account, const Accounts::Service &srv) const
{
    return !d->syncProfileIds(account, srv).isEmpty();
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
