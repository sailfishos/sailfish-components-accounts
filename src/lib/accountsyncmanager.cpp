/*
 * Copyright (C) 2013 Jolla Ltd.
 * Contact: Bea Lam <bea.lam@jollamobile.com>
 *
 * License: Proprietary
 */

#include "accountsyncmanager.h"

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

static const QString SyncProfileTemplateKey = QStringLiteral("sync_profile");

static QString SyncProfileIdKey(const QString &templateProfileName)
{
    return QStringLiteral("%1/%2").arg(templateProfileName).arg(Buteo::KEY_PROFILE_ID);
}


class AccountSyncProfileManagerPrivate : public QObject
{
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

    void creatingProfile(Accounts::Account *account, const QString &serviceName, const QString &profileId);
    bool startSync(const QString &profileId);

    QString syncProfileId(Accounts::Account *account, const Accounts::Service &srv) const;

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

void AccountSyncProfileManagerPrivate::creatingProfile(Accounts::Account *account, const QString &serviceName, const QString &profileId)
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

QString AccountSyncProfileManagerPrivate::syncProfileId(Accounts::Account *account, const Accounts::Service &srv) const
{
    if (!account || !srv.isValid()) {
        return QString();
    }
    account->selectService(srv);
    QString syncProfileTemplate = account->value(SyncProfileTemplateKey).toString();
    QString syncProfileId = account->value(SyncProfileIdKey(syncProfileTemplate)).toString();
    account->selectService(Accounts::Service());
    return syncProfileId;
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
    d->creatingProfile(account, serviceName, profileId);
}

void AccountSyncManager::updateProfile(const QString &profileId, const QVariantMap &properties)
{
    if (profileId.isEmpty()) {
        emit profileUpdateError(profileId, QStringLiteral("specified profileId is an empty string"));
        return;
    }
    if (!updateSyncProfile(profileId, properties)) {
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

QString AccountSyncManager::profileId(int accountId, const QString &serviceName) const
{
    Accounts::Account *account = d->m_accountManager->account(accountId);
    if (account) {
        Accounts::Service srv = d->m_accountManager->service(serviceName);
        if (srv.isValid()) {
            return d->syncProfileId(account, srv);
        }
    }
    return QString();
}

QString AccountSyncManager::createProfile(const QString &templateProfileName,
                                          Accounts::Account *account,
                                          const Accounts::Service &srv,
                                          bool enableProfile,
                                          const QVariantMap &properties)
{
    if (!account || !srv.isValid()) {
        qWarning() << "Invalid account or service";
        return QString();
    }
    if (templateProfileName.isEmpty()) {
        qWarning() << "Invalid templateProfileName";
        return QString();
    }

    account->selectService(srv);

    Buteo::SyncProfile *templateProfile = d->m_profileManager->syncProfile(templateProfileName);
    if (!templateProfile) {
        account->selectService(Accounts::Service());
        qWarning() << "Unable to load template profile:" << templateProfileName;
        return QString();
    }

    Buteo::SyncProfile *profile = templateProfile->clone();
    if (!profile) {
        delete templateProfile;
        account->selectService(Accounts::Service());
        qWarning() << "unable to clone template profile:" << templateProfileName;
        return QString();
    }

    QString accountIdStr = QString::number(account->id());
    profile->setName(templateProfileName + "-" + accountIdStr);
    profile->setKey(Buteo::KEY_DISPLAY_NAME, templateProfileName + "-" + account->displayName().toHtmlEscaped());
    profile->setKey(Buteo::KEY_ACCOUNT_ID, accountIdStr);
    profile->setBoolKey(Buteo::KEY_USE_ACCOUNTS, true);
    profile->setEnabled(enableProfile);

    // disable the schedule by default
    Buteo::SyncSchedule schedule = profile->syncSchedule();
    schedule.setScheduleEnabled(false);
    profile->setSyncSchedule(schedule);

    // set custom properties; note this may override any properties already set
    Q_FOREACH (const QString &key, properties.keys()) {
        profile->setKey(key, properties[key].toString());
    }

    QString profileId = d->m_profileManager->updateProfile(*profile);
    if (profileId.isEmpty()) {
        qWarning() << "Unable to save sync profile" << templateProfile->name();
    } else {
        account->setValue(SyncProfileIdKey(templateProfile->name()), profile->name());
    }

    account->selectService(Accounts::Service());
    delete profile;
    delete templateProfile;

    return profileId;
}

bool AccountSyncManager::updateSyncProfile(const QString &profileId, const QVariantMap &properties)
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
    QString savedProfileId = d->m_profileManager->updateProfile(*profile);
    delete profile;
    return !savedProfileId.isEmpty();
}

bool AccountSyncManager::hasProfile(Accounts::Account *account, const Accounts::Service &srv) const
{
    return !d->syncProfileId(account, srv).isEmpty();
}

QString AccountSyncManager::defaultTemplateProfile(Accounts::Account *account, const Accounts::Service &srv) const
{
    if (!account || !srv.isValid()) {
        return QString();
    }
    account->selectService(srv);
    QString defaultTemplate = account->value(SyncProfileTemplateKey).toString();
    account->selectService(Accounts::Service());
    return defaultTemplate;
}
