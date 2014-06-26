/*
 * Copyright (C) 2013 Jolla Ltd.
 * Contact: Chris Adams <chris.adams@jollamobile.com>
 *
 * License: Proprietary
 */

//project
#include "accountmodel.h"
#include "account.h"
#include "provider.h"
#include "accountsyncmanager.h"
#include "globalaccountmanager_p.h"
#include "globaltranslatorcache_p.h"

//Qt
#include <QtDebug>
#include <QTimer>
#include <QDBusConnection>

//libaccounts-qt
#include <Accounts/Manager>
#include <Accounts/Account>
#include <Accounts/Provider>


static const QString AccountCredentialsNeedUpdateKey = QStringLiteral("CredentialsNeedUpdate");

struct DisplayData {
    DisplayData(Accounts::Account *acct)
        : account(acct)
        , performingInitialSync(false)
        , monitorInitialSync(false)
    {
        Accounts::ServiceList services = account->services();
        foreach (const Accounts::Service &service, services) {
            serviceNames.append(service.name());
            serviceTypes.append(service.serviceType());
        }
    }
    ~DisplayData() { delete account; }

    bool matchesFilter(AccountModel::FilterType filterType, const QString &filter) const {
        switch (filterType) {
        case AccountModel::NoFilter:
            return true;
        case AccountModel::ProviderFilter:
            return filter == providerName;
        case AccountModel::ServiceFilter:
            return serviceNames.contains(filter);
        case AccountModel::ServiceTypeFilter:
            return serviceTypes.contains(filter);
        }
        return false;
    }

    QHash<QString, int> profilesSyncStatus;
    Accounts::Account *account;
    QString providerName;
    QString providerDisplayName;
    QString accountIcon;
    QStringList serviceNames;
    QStringList serviceTypes;
    bool performingInitialSync;
    bool monitorInitialSync;
    Q_DISABLE_COPY(DisplayData);
};

class AccountModel::AccountModelPrivate
{
public:
    AccountModelPrivate()
        : accountSyncManager(0)
        , filterType(AccountModel::NoFilter)
        , dbusConnection(QDBusConnection::sessionBus())
        , rowToUpdate(-1)
        , componentComplete(false)
        , dbusInitialized(false)
    {
    }

    ~AccountModelPrivate()
    {
        qDeleteAll(accountsList);
    }

    QVariant getModelData(DisplayData *displayData, int role) const {
        if (role == AccountIdRole) {
            return QVariant::fromValue(displayData->account->id());
        }
        if (role == AccountDisplayNameRole) {
            return QVariant::fromValue(displayData->account->displayName());
        }
        if (role == AccountIconRole) {
            if (displayData->accountIcon.isNull()) {
                displayData->accountIcon = displayData->account->provider().iconName();
            }
            return QVariant::fromValue(displayData->accountIcon);
        }
        if (role == ProviderNameRole) {
            if (displayData->providerName.isEmpty()) {
                displayData->providerName = displayData->account->providerName();
            }
            return QVariant::fromValue(displayData->providerName);
        }
        if (role == ProviderDisplayNameRole) {
            if (displayData->providerDisplayName.isEmpty()) {
                displayData->providerDisplayName = SailfishAccounts::translatedDisplayName(displayData->account->provider());
            }
            return QVariant::fromValue(displayData->providerDisplayName);
        }
        if (role == AccountEnabledRole) {
            return QVariant::fromValue(displayData->account->enabled());
        }
        if (role == AccountErrorRole) {
            if (displayData->account->value(AccountCredentialsNeedUpdateKey).toBool()) {
                return AccountNotSignedInError;
            }
            return NoAccountError;
        }
        if (role == PerformingInitialSyncRole) {
            return displayData->performingInitialSync;
        }
        return QVariant();
    }

    QHash<int, QByteArray> headerData;
    Accounts::Manager *manager;
    AccountSyncManager *accountSyncManager;
    QList<DisplayData *> accountsList;
    QList<DisplayData *> filteredAccountsList;
    AccountModel::FilterType filterType;
    QDBusConnection dbusConnection;
    QString filter;
    int rowToUpdate;
    bool componentComplete;
    bool dbusInitialized;
};

namespace {

    void insertAccountSorted(DisplayData *data,
                             Accounts::Account *account,
                             QList<DisplayData *> *accountsList,
                             QList<DisplayData *> *filteredAccountsList)
    {
        // sort by provider display name then account display name
        bool addedAccount = false;
        for (int j = 0; j < accountsList->size(); ++j) {
            Accounts::Provider listAccountProvider = accountsList->at(j)->account->provider();
            Accounts::Provider thisAccountProvider = account->provider();
            if (SailfishAccounts::translatedDisplayName(thisAccountProvider) < SailfishAccounts::translatedDisplayName(listAccountProvider)
                    || (SailfishAccounts::translatedDisplayName(thisAccountProvider) == SailfishAccounts::translatedDisplayName(listAccountProvider)
                        && account->displayName() < accountsList->at(j)->account->displayName())) {
                accountsList->insert(j, data);
                filteredAccountsList->insert(j, data);
                addedAccount = true;
                break;
            }
        }

        if (!addedAccount) {
            accountsList->append(data);
            filteredAccountsList->append(data);
        }
    }
}


/*!
    \qmltype AccountModel
    \instantiates AccountModel
    \inherits QAbstractListModel
    \inqmlmodule Sailfish.Accounts 1
    \brief Provides a model of existing accounts

    The AccountModel can be used to provide account data to a view.
    For each account in the database, it exposes the following roles:
    \list
    \li \c accountId
    \li \c accountDisplayName
    \li \c accountIcon
    \li \c providerName
    \li \c providerDisplayName
    \li \c accountEnabled
    \li \c accountError
    \endlist
*/

AccountModel::AccountModel(QObject* parent)
    : QAbstractListModel(parent)
    , d_ptr(new AccountModelPrivate())
{
    Q_D(AccountModel);
    d->manager = globalAccountManager();
    Accounts::ServiceList allServices = d->manager->serviceList(); // force reload of service files.
    d->headerData.insert(AccountIdRole, "accountId");
    d->headerData.insert(AccountDisplayNameRole, "accountDisplayName");
    d->headerData.insert(AccountIconRole, "accountIcon" );
    d->headerData.insert(ProviderNameRole, "providerName");
    d->headerData.insert(ProviderDisplayNameRole, "providerDisplayName");
    d->headerData.insert(AccountEnabledRole, "accountEnabled");
    d->headerData.insert(AccountErrorRole, "accountError");
    d->headerData.insert(PerformingInitialSyncRole, "performingInitialSync");
    QObject::connect(d->manager, SIGNAL(accountCreated(Accounts::AccountId)),
                     this, SLOT(accountCreated(Accounts::AccountId)));
    QObject::connect(d->manager, SIGNAL(accountRemoved(Accounts::AccountId)),
                     this, SLOT(accountRemoved(Accounts::AccountId)));
    QObject::connect(d->manager, SIGNAL(accountUpdated(Accounts::AccountId)),
                     this, SLOT(accountUpdated(Accounts::AccountId)));
    QObject::connect(d->manager, SIGNAL(enabledEvent(Accounts::AccountId)),
                     this, SLOT(accountUpdated(Accounts::AccountId)));
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    setRoleNames(d->headerData);
#endif
    Accounts::AccountIdList idList = d->manager->accountList();
    foreach (Accounts::AccountId id, idList) {
        Accounts::Account *account = d->manager->account(id);
        if (!account->provider().isValid()) {
            continue;
        }
        addedAccount(account);
        DisplayData *data = new DisplayData(account);
        insertAccountSorted(data, account, &d->accountsList, &d->filteredAccountsList);
    }
}

AccountModel::~AccountModel()
{
}

AccountModel::FilterType AccountModel::filterType() const
{
    Q_D(const AccountModel);
    return d->filterType;
}

void AccountModel::setFilterType(FilterType filterType)
{
    Q_D(AccountModel);
    if (filterType != d->filterType) {
        d->filterType = filterType;
        if (d->componentComplete)
            reload();
        emit filterTypeChanged();
    }
}

QString AccountModel::filter() const
{
    Q_D(const AccountModel);
    return d->filter;
}

void AccountModel::setFilter(const QString &filter)
{
    Q_D(AccountModel);
    if (filter != d->filter) {
        d->filter = filter;
        if (d->componentComplete)
            reload();
        emit filterChanged();
    }
}

void AccountModel::setAccountEnabled(int accountId, bool enabled)
{
    Q_D(AccountModel);
    for (int i=0; i<d->accountsList.count(); i++) {
        if (d->accountsList[i]->account->id() == (uint)accountId) {
            d->accountsList[i]->account->setEnabled(enabled);
            d->accountsList[i]->account->sync();
            break;
        }
    }
}

QVariantMap AccountModel::getByAccount(int accountId)
{
    Q_D(AccountModel);
    QVariantMap ret;
    for (int i=0; i<d->accountsList.count(); i++) {
        if (d->accountsList[i]->account->id() == (uint)accountId) {
            Q_FOREACH (int role, d->headerData.keys()) {
                ret.insert(d->headerData[role], d->getModelData(d->accountsList[i], role));
            }
            break;
        }
    }
    return ret;
}

void AccountModel::reload()
{
    Q_D(AccountModel);
    QList<DisplayData *> result;
    for (int i=0; i<d->accountsList.count(); i++) {
        if (d->accountsList[i]->matchesFilter(d->filterType, d->filter)) {
            result.append(d->accountsList[i]);
        }
    }
    beginResetModel();
    d->filteredAccountsList = result;
    endResetModel();
}

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
QHash<int, QByteArray> AccountModel::roleNames() const
{
    Q_D(const AccountModel);
    return d->headerData;
}
#endif

int AccountModel::rowCount(const QModelIndex &) const
{
    Q_D(const AccountModel);
    return d->filteredAccountsList.count();
}

QVariant AccountModel::data(const QModelIndex &index, int role) const
{
    Q_D(const AccountModel);
    if (!index.isValid() || index.row() >= d->filteredAccountsList.length()) {
        return QVariant();
    }

    DisplayData *data = d->filteredAccountsList[index.row()];
    return d->getModelData(data, role);
}

void AccountModel::classBegin()
{
}

void AccountModel::componentComplete()
{
    Q_D(AccountModel);
    if (d->filterType != NoFilter && !d->filter.isEmpty()) {
        reload();
    }
    d->componentComplete = true;
}

void AccountModel::accountCreated(Accounts::AccountId id)
{
    if (id) {
        Q_D(AccountModel);
        Accounts::Account *account = d->manager->account(id);

        if (account != 0) {
            addedAccount(account);
            insertAccountSorted(new DisplayData(account), account, &d->accountsList, &d->filteredAccountsList);
            monitorSyncStatus(account);
            reload();
        }
    }
}

void AccountModel::exchangeSyncStarted(qulonglong accountId)
{
    Q_D(AccountModel);

    int i = getAccountIndex(accountId);
    if (i >= 0 && d->accountsList.at(i)->monitorInitialSync) {
        d->accountsList.at(i)->performingInitialSync = true;
        int filteredIndex = getFilteredAccountsIndex(accountId);
        emit dataChanged(index(filteredIndex, 0), index(filteredIndex, 0));
    }
}

void AccountModel::exchangeSyncCompleted(qulonglong accountId, int result)
{
    Q_D(AccountModel);

    Q_UNUSED(result);

    int i = getAccountIndex(accountId);
    if (i >= 0 && d->accountsList.at(i)->monitorInitialSync) {
        d->accountsList.at(i)->performingInitialSync = false;
        d->accountsList.at(i)->monitorInitialSync = false;
        int filteredIndex = getFilteredAccountsIndex(accountId);
        emit dataChanged(index(filteredIndex, 0), index(filteredIndex, 0));
    }
}

void AccountModel::monitorSyncStatus(Accounts::Account *account)
{
    Q_D(AccountModel);

    int accountIndex = getAccountIndex(account->id());
    DisplayData *displayData = d->accountsList.at(accountIndex);
    displayData->monitorInitialSync = true;

    if (account->providerName() == QStringLiteral("activesync")) {
        if (!d->dbusInitialized) {
            static const QString dbusAddress = QStringLiteral("com.nokia.asdbus");
            static const QString dbusPath = QStringLiteral("/com/nokia/asdbus");
            d->dbusConnection.connect(QString(), dbusPath, dbusAddress, "syncStarted", this, SLOT(exchangeSyncStarted(qulonglong)));
            d->dbusConnection.connect(QString(), dbusPath, dbusAddress, "syncCompleted", this, SLOT(exchangeSyncCompleted(qulonglong, int)));
            d->dbusInitialized = true;
        }
    } else {
        if (!d->accountSyncManager) {
            d->accountSyncManager = new AccountSyncManager(this);
            connect(d->accountSyncManager, SIGNAL(profileSyncStatusChanged(QString,int,QString)),
                    SLOT(profileSyncStatusChanged(QString,int,QString)));
        }
    }
}

void AccountModel::profileSyncStatusChanged(const QString &profileId, int status, const QString &errorString)
{
    Q_D(AccountModel);
    Q_UNUSED(errorString);

    bool profileIsSyncing = (status == AccountSyncManager::SyncStarted);
    for (int i=0; i<d->accountsList.count(); i++) {
        DisplayData *displayData = d->accountsList.at(i);
        if (!displayData->monitorInitialSync
                || displayData->account->providerName() == QStringLiteral("activesync")) {
            continue;
        }
        if (displayData->profilesSyncStatus.isEmpty()) {
            Q_FOREACH(const QString &profileId, d->accountSyncManager->profileIds(displayData->account->id())) {
                displayData->profilesSyncStatus[profileId] = AccountSyncManager::UnknownSyncStatus;
            }
            if (displayData->profilesSyncStatus.isEmpty()) {
                // no profiles to be monitored for this account
                displayData->monitorInitialSync = false;
                continue;
            }
        }
        if (displayData->profilesSyncStatus.contains(profileId)) {
            bool wasSyncing = (displayData->profilesSyncStatus[profileId] == AccountSyncManager::SyncStarted);
            bool emitDataChanged = false;
            if (wasSyncing && !profileIsSyncing) {
                // profile has finished syncing, remove it from the map
                displayData->profilesSyncStatus.remove(profileId);
            } else {
                displayData->profilesSyncStatus[profileId] = status;
                if (!displayData->performingInitialSync && profileIsSyncing) {
                    displayData->performingInitialSync = true;
                    emitDataChanged = true;
                }
            }
            if (displayData->performingInitialSync && displayData->profilesSyncStatus.isEmpty()) {
                // all profiles for this account have been synced, stop monitoring the sync status
                displayData->performingInitialSync = false;
                displayData->monitorInitialSync = false;
                emitDataChanged = true;
            }
            if (emitDataChanged) {
                int filteredIndex = getFilteredAccountsIndex(displayData->account->id());
                emit dataChanged(index(filteredIndex, 0), index(filteredIndex, 0));
            }
            break;
        }
    }
}

void AccountModel::accountRemoved(Accounts::AccountId id)
{
    Q_D(AccountModel);

    int index = getAccountIndex(id);
    if (index < 0) {
        return;
    }

    int filteredIndex = getFilteredAccountsIndex(id);
    if (filteredIndex >= 0) {
        beginRemoveRows(QModelIndex(), filteredIndex, filteredIndex);
        d->filteredAccountsList.removeAt(filteredIndex);
        endRemoveRows();
    }

    DisplayData *data = d->accountsList.takeAt(index);
    removedAccount(data->account);
    delete data;
}

void AccountModel::accountUpdated(Accounts::AccountId id)
{
    Q_D(AccountModel);
    int accountIndex = getFilteredAccountsIndex(id);
    if (accountIndex < 0) {
        return;
    }
    // the service settings are updated in the account object asynchronously, so delay
    // the emission of dataChanged()
    d->rowToUpdate = accountIndex;
    QTimer::singleShot(0, this, SLOT(delayedIndexUpdate()));
}

void AccountModel::delayedIndexUpdate()
{
    Q_D(AccountModel);
    if (d->rowToUpdate >= 0) {
        emit dataChanged(index(d->rowToUpdate, 0), index(d->rowToUpdate, 0));
    }
}

void AccountModel::accountDisplayNameChanged()
{
    Accounts::Account *account = qobject_cast<Accounts::Account*>(sender());
    if (account)
        accountUpdated(account->id());
}

void AccountModel::accountEnabledChanged()
{
    Accounts::Account *account = qobject_cast<Accounts::Account*>(sender());
    if (account) {
        accountUpdated(account->id());
    }
}

int AccountModel::getAccountIndex(Accounts::AccountId id) const
{
    Q_D(const AccountModel);
    for (int i = 0; i < d->accountsList.count(); ++i) {
        if (d->accountsList.at(i)->account->id() == id) {
            return i;
        }
    }

    return -1;
}

int AccountModel::getFilteredAccountsIndex(Accounts::AccountId id) const
{
    Q_D(const AccountModel);
    for (int i = 0; i < d->filteredAccountsList.count(); ++i) {
        if (d->filteredAccountsList.at(i)->account->id() == id) {
            return i;
        }
    }

    return -1;
}

void AccountModel::addedAccount(Accounts::Account *account)
{
    if (account) {
        QObject::connect(account, SIGNAL(displayNameChanged(QString)),
                this, SLOT(accountDisplayNameChanged()));
        QObject::connect(account, SIGNAL(enabledChanged(QString,bool)),
                this, SLOT(accountEnabledChanged()));
    }
}

void AccountModel::removedAccount(Accounts::Account *account)
{
    if (account) {
        account->disconnect(this);
    }
}

