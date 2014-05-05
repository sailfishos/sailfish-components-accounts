/*
** Copyright (C) 2014 Jolla Ltd.
*/

#include "accountmodifier_p.h"
#include <Accounts/Manager>
#include <Accounts/Account>

// buteo-syncfw
#include <ProfileEngineDefs.h>

#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <QtDebug>

static const QString AccountUpdateMarkerFilename = ".accounts-update-sync-services";

AccountModifier::AccountModifier(QObject *parent)
    : QObject(parent)
    , mode(UnknownMode)
    , m_accountManager(new Accounts::Manager)
    , m_currAccount(0)
    , m_buteoClient(0)
    , m_currAccountIdx(-1)
    , m_error(false)
{
}

AccountModifier::~AccountModifier()
{
    delete m_accountManager;
}

bool AccountModifier::errorOccurred() const
{
    return m_error;
}

// helper to perform conversion from argument passed via commandline into QVariant for storage/setting.
QVariant convertSettingValue(const QString &settingName, const QString &settingType, const QString &settingValue)
{
    QVariant variantSettingValue;
    if (settingType == QString::fromLatin1("bool")) {
        if (settingValue == QString::fromLatin1("false") ||
                settingValue == QString::fromLatin1("0")) {
            variantSettingValue = QVariant::fromValue<bool>(false);
        } else {
            variantSettingValue = QVariant::fromValue<bool>(true);
        }
    } else if (settingType == QString::fromLatin1("string")) {
        variantSettingValue = QVariant::fromValue<QString>(settingValue);
    } else if (settingType == QString::fromLatin1("int")) {
        int intSettingValue = settingValue.toInt();
        variantSettingValue = QVariant::fromValue<int>(intSettingValue);
    } else if (settingType == QString::fromLatin1("uint")) {
        uint uintSettingValue = settingValue.toUInt();
        variantSettingValue = QVariant::fromValue<uint>(uintSettingValue);
    } else if (settingType == QString::fromLatin1("longlong")) {
        qlonglong longlongSettingValue = settingValue.toLongLong();
        variantSettingValue = QVariant::fromValue<qlonglong>(longlongSettingValue);
    } else if (settingType == QString::fromLatin1("ulonglong")) {
        qulonglong ulonglongSettingValue = settingValue.toULongLong();
        variantSettingValue = QVariant::fromValue<qulonglong>(ulonglongSettingValue);
    } else {
        // not a valid setting type, return invalid variant.
        qWarning() << Q_FUNC_INFO << "invalid setting type specified for" << settingName;
    }

    return variantSettingValue;
}

void AccountModifier::start()
{
    m_error = false;

    if (mode == UnknownMode) {
        qWarning() << Q_FUNC_INFO << "No mode set for AccountModifier!";
        emit done();
        return;
    }

    if (!m_accountManager) {
        qWarning() << Q_FUNC_INFO << "could not instantiate account manager!";
        m_error = true;
        emit done();
        return;
    }

    m_allAccountIds = m_accountManager->accountList();
    if (m_allAccountIds.size() == 0) {
        qWarning() << Q_FUNC_INFO << "no accounts to modify";
        emit done();
        return;
    }

    qWarning() << "Found" << m_allAccountIds.size() << "accounts in total";

    if (mode == ModifyServiceSettings) {
        checkServiceSettingArgs();
    }

    m_currAccountIdx = -1;
    next();
}

void AccountModifier::checkServiceSettingArgs()
{
    bool emptyProvider = providerName.isEmpty();
    bool emptyService = serviceName.isEmpty();
    bool modifyMode = modeSwitch == QString::fromLatin1("-m");

    if (emptyProvider && emptyService) {
        qWarning() << "Untested codepath: provider name and service name is empty!"
                   << settingName << "will be"
                   << (modifyMode ? "modified in" : "removed from")
                   << "all services of all accounts!";
    } else if (emptyProvider) {
        qWarning() << "Untested codepath: provider name is empty!"
                   << settingName << "will be"
                   << (modifyMode ? "modified in" : "removed from")
                   << serviceName << "of all accounts!";
    } else if (emptyService) {
        qWarning() << "Untested codepath: service name is empty!"
                   << settingName << "will be"
                   << (modifyMode ? "modified in" : "removed from")
                   << "all services of" << providerName << "accounts!";
    } else {
        qWarning() << settingName << "will be"
                   << (modifyMode ? "modified in" : "removed from")
                   << serviceName << "of" << providerName << "accounts!";
    }
}

void AccountModifier::next()
{
    if (m_currAccount) {
        m_currAccount->disconnect(this);
    }

    m_currAccountIdx += 1;
    if (m_currAccountIdx >= m_allAccountIds.size()) {
        if (mode == UpdateSyncServices) {
            QStringList homePaths = QStandardPaths::standardLocations(QStandardPaths::HomeLocation);
            if (homePaths.count()) {
                QString path = homePaths[0] + QDir::separator() + AccountUpdateMarkerFilename;
                if (!QFile::remove(path)) {
                    qWarning() << "sailfish-accounts-tool marker file:" << path << "doesn't exist, not removing";
                }
            }
        }
        emit done();
        return;
    }

    Accounts::AccountId currAccountId = m_allAccountIds.at(m_currAccountIdx);
    m_currAccount = currAccountId > 0 ? m_accountManager->account(currAccountId) : 0;
    if (!m_currAccount) {
        qWarning() << Q_FUNC_INFO << "Unknown error: account could not be retrieved:" << currAccountId;
        next();
        return;
    }

    bool needsSync = false;
    switch (mode) {
    case ModifyServiceSettings:
        needsSync = applyServiceSettingChanges();
        break;
    case UpdateSyncServices:
        needsSync = applySyncUpdateChanges();
        break;
    default:
        qWarning() << Q_FUNC_INFO << "Unhandled AccountModifier mode!";
        emit done();
        return;
    }
    if (m_error) {
        qWarning() << "Failed to update account" << currAccountId;
        emit done();
        return;
    }
    if (needsSync) {
        connect(m_currAccount, SIGNAL(error(Accounts::Error)), this, SLOT(error(Accounts::Error)));
        connect(m_currAccount, SIGNAL(synced()), this, SLOT(next()));
        m_currAccount->sync(); // can't use syncAndBlock() or it causes deadlock in some cases.
    } else {
        next();
    }
}

bool AccountModifier::applyServiceSettingChanges()
{
    if (providerName.isEmpty() || m_currAccount->providerName() == providerName) {
        QVariant variantSettingValue = convertSettingValue(settingName, settingType, settingValue);
        if (serviceName == QString::fromLatin1("--global")) {
            // modify/remove the global service setting only
            m_currAccount->selectService(Accounts::Service());
            if (modeSwitch == QString::fromLatin1("-r")) {
                m_currAccount->remove(settingName);
            } else {
                m_currAccount->setValue(settingName, variantSettingValue);
            }
        } else {
            // modify/remove the setting for a particular service or for all services
            Accounts::ServiceList allServices = m_currAccount->services();
            foreach (const Accounts::Service &srv, allServices) {
                if (serviceName.isEmpty() || srv.name() == serviceName) {
                    m_currAccount->selectService(srv);
                    if (modeSwitch == QString::fromLatin1("-r")) {
                        m_currAccount->remove(settingName);
                    } else {
                        m_currAccount->setValue(settingName, variantSettingValue);
                    }
                    m_currAccount->selectService(Accounts::Service());
                }
            }
        }
        return true;
    }
    return false;
}

bool AccountModifier::applySyncUpdateChanges()
{
    Accounts::ServiceList allServices = m_currAccount->services();

    bool genericSyncServiceEnabled = false;
    bool hasGenericSyncService = false;
    uint credentialsId = 0;

    // Find the generic sync service. This will be the one called "<provider>-sync".
    foreach (const Accounts::Service &srv, allServices) {
        bool isGenericSyncService = srv.serviceType() == QStringLiteral("sync")
                && srv.name() == (srv.provider() + "-sync");
        if (isGenericSyncService) {
            m_currAccount->selectService(srv);
            hasGenericSyncService = true;
            genericSyncServiceEnabled = m_currAccount->enabled();
            credentialsId = m_currAccount->credentialsId();
            if (credentialsId == 0) {
                qWarning() << "Cannot update sync services, the generic sync service doesn't have a valid credentialsId!";
                m_error = true;
                return false;
            }
            break;
        }
    }
    m_currAccount->selectService(Accounts::Service());
    if (!hasGenericSyncService) {
        return false;
    }

    bool madeChanges = false;
    foreach (const Accounts::Service &srv, allServices) {
        QStringList templateProfiles = accountSyncManager.defaultTemplateProfiles(m_currAccount, srv);
        m_currAccount->selectService(srv);
        Q_FOREACH(const QString &templateProfile, templateProfiles) {
            if (templateProfile.isEmpty() || accountSyncManager.hasProfile(m_currAccount, srv)) {
                continue;
            }

            // copy credentials and set enabled status according to generic sync service
            m_currAccount->setCredentialsId(credentialsId);
            m_currAccount->setEnabled(genericSyncServiceEnabled);

            // Create the profile. This can't use AccountSyncManager::createProfile() because this
            // tool may be run without privileged permissions, so use SyncClientInterface to talk
            // to msyncd to save the profiles instead of doing it in-process.
            Buteo::SyncProfile *profile = accountSyncManager.newProfileFromTemplate(templateProfile, m_currAccount, srv, genericSyncServiceEnabled);

            if (!profile) {
                qWarning() << "Profile could not created for template:" << templateProfile;
                m_error = true;
                return false;
            }

            QString newProfileId = QStringLiteral("%1-%2").arg(templateProfile).arg(m_currAccount->id());

            if (!m_buteoClient) {
                m_buteoClient = new Buteo::SyncClientInterface;
            }
            if (!m_buteoClient->updateProfile(*profile)) {
                qWarning() << "SyncClientInterface::updateProfile() failed for" << srv.name() << "!";
                m_error = true;
                return false;
            }

            // save the profile name to the account
            QString profileKey = QStringLiteral("%1/%2").arg(templateProfile).arg(Buteo::KEY_PROFILE_ID);
            m_currAccount->setValue(profileKey, newProfileId);

            delete profile;
            madeChanges = true;
        }
    }
    m_currAccount->selectService(Accounts::Service());
    return madeChanges;
}

void AccountModifier::error(Accounts::Error err)
{
    qWarning() << Q_FUNC_INFO << "error during sync of account" << m_currAccount->id()
               << ":" << err.type() << err.message();
    m_error = true;
    emit done();
}
