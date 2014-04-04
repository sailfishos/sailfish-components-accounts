/*
** Copyright (C) 2014 Jolla Ltd.
*/

#include "accountmodifier_p.h"
#include <Accounts/Manager>
#include <Accounts/Account>

#include <QtDebug>

AccountModifier::AccountModifier(QObject *parent)
    : QObject(parent)
    , mode(UnknownMode)
    , m_accountManager(new Accounts::Manager)
    , m_currAccount(0)
    , m_currAccountIdx(-1)
{
}

AccountModifier::~AccountModifier()
{
    delete m_accountManager;
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
    if (mode == UnknownMode) {
        qWarning() << Q_FUNC_INFO << "No mode set for AccountModifier!";
        emit done();
        return;
    }

    if (!m_accountManager) {
        qWarning() << Q_FUNC_INFO << "could not instantiate account manager!";
        emit done();
        return;
    }

    m_allAccountIds = m_accountManager->accountList();
    if (m_allAccountIds.size() == 0) {
        qWarning() << Q_FUNC_INFO << "no accounts to modify";
        emit done();
        return;
    }

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
    foreach (const Accounts::Service &srv, allServices) {
        bool isGenericSyncService = srv.serviceType() == QStringLiteral("sync")
                && accountSyncManager.defaultTemplateProfiles(m_currAccount, srv).isEmpty();
        if (isGenericSyncService) {
            m_currAccount->selectService(srv);
            hasGenericSyncService = true;
            genericSyncServiceEnabled = m_currAccount->enabled();
            credentialsId = m_currAccount->credentialsId();
            if (credentialsId == 0) {
                qWarning() << "Cannot update sync services, the generic sync service doesn't have a valid credentialsId!";
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

            // generate sync profile
            QString profileId = accountSyncManager.createProfile(templateProfile, m_currAccount, srv, genericSyncServiceEnabled);
            if (profileId.isEmpty()) {
                qWarning() << "Unable to create sync profile for" << srv.name() << "!";
                return false;
            }
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
    next();
}
