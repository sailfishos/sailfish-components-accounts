/*
** Copyright (C) 2014 Jolla Ltd.
*/

#include "accountmodifier_p.h"

// buteo-syncfw
#include <ProfileEngineDefs.h>

// libaccounts-qt
#include <Accounts/Manager>
#include <Accounts/Account>

#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <QtDebug>

static const QString AccountToolMarkerFilename = ".sailfish-accounts-tool";
static const QString KeyProviderAvailable = QStringLiteral("provider-available");
static const QString KeyEnableWhenProviderAvailable = QStringLiteral("enable-when-provider-available");

AccountModifier::AccountModifier(QObject *parent)
    : QObject(parent)
    , providerAvailable(false)
    , scheduleCommandForNextBoot(false)
    , runScheduledCommands(false)
    , runningMultipleCommands(false)
    , mode(UnknownMode)
    , m_accountManager(new Accounts::Manager)
    , m_currAccount(0)
    , m_buteoClient(0)
    , m_accountBackupRestorer(&m_accountSyncManager, m_accountManager)
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

    if (scheduleCommandForNextBoot) {
        addScheduledCommand(mode);
        emit done();
        return;
    }

    if (!m_accountManager) {
        qWarning() << Q_FUNC_INFO << "could not instantiate account manager!";
        m_error = true;
        emit done();
        return;
    }

    if (runScheduledCommands) {
        runningMultipleCommands = true;
    } else if (mode == RestoreAccounts) {
        // we do several things:
        // 1) we restore the accounts + credentials from file
        // 2) we update any service settings of those accounts
        // 3) we generate sync profiles for the restored accounts
        // 4) we trigger sync for all accounts
        if (!restoreAccounts()) {
            qWarning() << Q_FUNC_INFO << "could not restore accounts from" << backupFile;
            m_error = true;
            emit done();
            return;
        }
        m_commands.append(UpdateSyncServices);
        m_commands.append(CreateProfiles);
        m_commands.append(TriggerProfiles);
        runningMultipleCommands = true;
    }

    if (runningMultipleCommands) {
        if (m_commands.isEmpty() && runScheduledCommands) {
            QFile file(markerFilePath());
            if (!file.open(QIODevice::ReadOnly)) {
                qWarning() << "Cannot open" << file.fileName() << "to run commands!";
                done();
                return;
            }
            m_commands = loadScheduledCommands(&file);
            if (m_commands.isEmpty()) {
                qWarning() << "No scheduled commands to run";
                done();
                return;
            }
            qWarning() << "AccountsModifier: running" << m_commands.count() << "scheduled commands";
            runningMultipleCommands = true;
            mode = m_commands.takeFirst();
        } else {
            Q_ASSERT(!m_commands.isEmpty());
            mode = m_commands.takeFirst();
        }
    }

    if (mode == UnknownMode) {
        qWarning() << Q_FUNC_INFO << "No mode set for AccountModifier!";
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
        // finished all accounts.  check to see if we need to run another command.
        if (runningMultipleCommands) {
            if (!m_commands.isEmpty()) {
                // run the next command
                start();
                return;
            } else if (runScheduledCommands) {
                // no more commands to run, clean up our schedule file before emitting done().
                QFile::remove(markerFilePath());
            } // else RestoreAccounts mode, finished and don't need to cleanup anything.
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
    case UpdateProviderAvailability:
        needsSync = applyProviderAvailabilityChanges();
        break;
    case UpdateSyncServices:
        needsSync = applySyncUpdateChanges();
        break;
    case CreateProfiles:
        needsSync = createProfiles();
        break;
    case BackupAccounts:
        needsSync = false;
        backupAccount(m_currAccount);
        break;
    case TriggerProfiles:
        needsSync = false;
        triggerProfiles(m_currAccount);
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

    bool enableSyncServices = false;
    bool shouldUpdateServices = false;
    uint credentialsId = 0;

    // check if is a generic email account
    if (m_currAccount->providerName() == QStringLiteral("email")) {
        foreach (const Accounts::Service &srv, allServices) {
            if (srv.name() == QStringLiteral("email")) {
                m_currAccount->selectService(srv);
                shouldUpdateServices = true;
                enableSyncServices = m_currAccount->enabled();
                break;
            }
        }
    } else {
        // Find the generic sync service. This will be the one called "<provider>-sync".
        foreach (const Accounts::Service &srv, allServices) {
            bool isGenericSyncService = srv.serviceType() == QStringLiteral("sync")
                    && srv.name() == (srv.provider() + "-sync");
            if (isGenericSyncService) {
                m_currAccount->selectService(srv);
                shouldUpdateServices = true;
                enableSyncServices = m_currAccount->enabled();
                credentialsId = m_currAccount->credentialsId();
                if (credentialsId == 0) {
                    qWarning() << "Cannot update sync services, the generic sync service doesn't have a valid credentialsId!";
                    m_error = true;
                    return false;
                }
                break;
            }
        }
    }

    m_currAccount->selectService(Accounts::Service());
    if (!shouldUpdateServices) {
        return false;
    }

    bool madeChanges = false;
    foreach (const Accounts::Service &srv, allServices) {
        // ensure that the sync service settings are set up appropriately
        m_currAccount->selectService(srv);
        if (srv.name() == QStringLiteral("facebook-sync") ||
            srv.name() == QStringLiteral("facebook-calendars") ||
            srv.name() == QStringLiteral("facebook-microblog")) {
            // prior to version 1.0.7 these services did not include the rsvp_event scope
            QVariant scopesValue = m_currAccount->value(QStringLiteral("auth/oauth2/user_agent/Scope"), QVariant());
            if (scopesValue.isValid()) {
                QStringList scopes = scopesValue.toStringList();
                if (!scopes.contains(QStringLiteral("rsvp_event"))) {
                    scopes.append(QStringLiteral("rsvp_event"));
                    m_currAccount->setValue(QStringLiteral("auth/oauth2/user_agent/Scope"), QVariant::fromValue<QStringList>(scopes));
                    madeChanges = true;
                }
            } else {
                qWarning() << "unable to load scopes information for" << srv.name()
                           << "of account" << m_currAccount->id() << ", not updating!";
            }
        }

        // ensure that the sync profiles are set up appropriately
        QStringList templateProfiles = m_accountSyncManager.defaultTemplateProfiles(m_currAccount, srv);
        Q_FOREACH(const QString &templateProfile, templateProfiles) {
            if (templateProfile.isEmpty() || m_accountSyncManager.hasProfile(m_currAccount, srv)) {
                continue;
            }
            if (credentialsId != 0) {
                // copy credentials and set enabled status according to generic sync service
                m_currAccount->setCredentialsId(credentialsId);
            }
            m_currAccount->setEnabled(enableSyncServices);

            Buteo::SyncProfile *profile = m_accountSyncManager.newProfileFromTemplate(templateProfile, m_currAccount, srv, enableSyncServices);
            if (!profile) {
                qWarning() << "Profile could not created for template:" << templateProfile;
                m_error = true;
                return false;
            }

            // In version 1.0.5 and earlier, google-contacts were not synced by default, so updated
            // profiles should not use 2-way sync by default in case this is not the preferred option.
            if (srv.name() == QStringLiteral("google-contacts")) {
                profile->setSyncDirection(Buteo::SyncProfile::SYNC_DIRECTION_FROM_REMOTE);
            }

            if (!saveProfileViaMsyncd(m_currAccount, srv, profile, templateProfile)) {
                m_error = true;
                return false;
            }

            delete profile;
            madeChanges = true;
        }
    }
    m_currAccount->selectService(Accounts::Service());
    return madeChanges;
}

bool AccountModifier::applyProviderAvailabilityChanges()
{
    if (m_currAccount->providerName() != providerName) {
        return false;
    }
    if (!serviceType.isEmpty()) {
        Accounts::Service matchingService;
        Accounts::ServiceList allServices = m_currAccount->services();
        foreach (const Accounts::Service &srv, allServices) {
            if (srv.serviceType() == serviceType) {
                m_currAccount->selectService(srv);
                if (m_currAccount->enabled()) {
                    matchingService = srv;
                    break;
                }
            }
        }
        if (!matchingService.isValid()) {
            qWarning() << providerName << "provider does not have any enabled services of type"
                       << serviceType << ", provider-available status will not be updated";
            return false;
        }
    }
    m_currAccount->selectService(Accounts::Service());
    if (m_currAccount->value(KeyProviderAvailable).toBool() == providerAvailable) {
        return false;
    }
    if (providerAvailable) {
        QVariant shouldEnable = m_currAccount->value(KeyEnableWhenProviderAvailable);
        m_currAccount->setEnabled(!shouldEnable.isValid() || shouldEnable.toBool());
        m_currAccount->remove(KeyEnableWhenProviderAvailable);
    } else {
        m_currAccount->setValue(KeyEnableWhenProviderAvailable, m_currAccount->enabled());
        m_currAccount->setEnabled(false);
    }
    m_currAccount->setValue(KeyProviderAvailable, providerAvailable);
    return true;
}

bool AccountModifier::createProfiles()
{
    bool created = false;
    Q_FOREACH (const Accounts::Service &srv, m_currAccount->services()) {
        m_currAccount->selectService(srv);
        Q_FOREACH (const QString &templateProfile, m_accountSyncManager.defaultTemplateProfiles(m_currAccount, srv)) {
            if (!m_accountSyncManager.hasProfile(m_currAccount, srv, templateProfile)) {
                // Don't fail if any of the profiles cannot be created.
                Buteo::SyncProfile *profile = m_accountSyncManager.newProfileFromTemplate(templateProfile, m_currAccount, srv, m_currAccount->enabled());
                if (!profile) {
                    qWarning() << "Profile could not created for template:" << templateProfile;
                } else {
                    created = true;
                    saveProfileViaMsyncd(m_currAccount, srv, profile, templateProfile);
                }
                delete profile;
            }
        }
    }
    return created;
}

void AccountModifier::error(Accounts::Error err)
{
    qWarning() << Q_FUNC_INFO << "error during sync of account" << m_currAccount->id()
               << ":" << err.type() << err.message();
    m_error = true;
    emit done();
}

/*
    Saves a profile out-of-process through Buteo::SyncClientInterface, rather than in-process through
    Buteo::ProfileManager. This ensures the profile can be updated even when sailfish-accounts-tool
    is run without privileged permissions.
 */
bool AccountModifier::saveProfileViaMsyncd(Accounts::Account *account, const Accounts::Service &srv, Buteo::SyncProfile *profile, const QString &templateProfile)
{
    if (!m_buteoClient) {
        m_buteoClient = new Buteo::SyncClientInterface;
    }
    if (!m_buteoClient->updateProfile(*profile)) {
        qWarning() << "SyncClientInterface::updateProfile() failed for" << srv.name() << "!";
        return false;
    }

    // save the profile name to the account
    QString profileKey = QStringLiteral("%1/%2").arg(templateProfile).arg(Buteo::KEY_PROFILE_ID);
    QString newProfileId = QStringLiteral("%1-%2").arg(templateProfile).arg(account->id());
    Accounts::Service prevService = account->selectedService();
    account->selectService(srv);
    account->setValue(profileKey, newProfileId);
    account->selectService(prevService);
    return true;
}

void AccountModifier::addScheduledCommand(Mode command)
{
    QFile file(markerFilePath());
    if (!file.open(QIODevice::ReadWrite)) {
        qWarning() << "Cannot open" << file.fileName() << "to schedule command!";
        return;
    }
    QList<AccountModifier::Mode> cmds = loadScheduledCommands(&file);
    if (!cmds.contains(command)) {
        QString str = QString("%1,").arg(int(command));
        file.write(str.toUtf8().constData());
    }
}

QList<AccountModifier::Mode> AccountModifier::loadScheduledCommands(QFile *file)
{
    QList<AccountModifier::Mode> cmds;
    if (!file) {
        return cmds;
    }
    QByteArray ba = file->readAll();
    if (!ba.isEmpty()) {
        Q_FOREACH (const QString &cmd, QString::fromUtf8(ba.constData()).split(',')) {
            if (cmd.isEmpty()) {
                continue;
            }
            bool ok = false;
            int cmdInt = cmd.toInt(&ok);
            if (ok) {
                cmds << Mode(cmdInt);
            }
        }
    }
    return cmds;
}

QString AccountModifier::markerFilePath()
{
    QStringList homePaths = QStandardPaths::standardLocations(QStandardPaths::HomeLocation);
    if (homePaths.count()) {
        return homePaths[0] + QDir::separator() + AccountToolMarkerFilename;
    }
    qWarning() << "Error: QStandardPaths cannot find HomeLocation!";
    return QString();
}

bool AccountModifier::backupAccount(Accounts::Account *account)
{
    return m_accountBackupRestorer.backupAccount(account, backupFile);
}

bool AccountModifier::restoreAccounts()
{
    return m_accountBackupRestorer.restoreAccounts(backupFile);
}

void AccountModifier::triggerProfiles(Accounts::Account *account)
{
    QStringList profileIds = m_accountSyncManager.profileIds(account->id());
    Q_FOREACH (const QString &profileId, profileIds) {
        m_accountSyncManager.syncProfile(profileId);
    }
}

