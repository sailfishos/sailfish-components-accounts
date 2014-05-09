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
#include "globalaccountmanager_p.h"
#include "globaltranslatorcache_p.h"

//Qt
#include <QtDebug>

//libaccounts-qt
#include <Accounts/Manager>
#include <Accounts/Account>
#include <Accounts/Provider>

struct DisplayData {
    DisplayData(Accounts::Account *acct)
        : account(acct)
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

    Accounts::Account *account;
    QString providerName;
    QString providerDisplayName;
    QString accountIcon;
    QStringList serviceNames;
    QStringList serviceTypes;
    Q_DISABLE_COPY(DisplayData);
};

class AccountModel::AccountModelPrivate
{
public:
    AccountModelPrivate()
        : filterType(AccountModel::NoFilter)
        , componentComplete(false)
    {
    }

    ~AccountModelPrivate()
    {
        qDeleteAll(accountsList);
    }

    QHash<int, QByteArray> headerData;
    Accounts::Manager *manager;
    QList<DisplayData *> accountsList;
    QList<DisplayData *> filteredAccountsList;
    AccountModel::FilterType filterType;
    QString filter;
    bool componentComplete;
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
    Accounts::Account *account = data->account;

    if (role == AccountIdRole)
        return QVariant::fromValue(account->id());

    if (role == AccountDisplayNameRole)
        return QVariant::fromValue(account->displayName());

    if (role == AccountIconRole) {
        if (data->accountIcon.isNull()) {
            Accounts::Provider provider = d->manager->provider(account->providerName());
            data->accountIcon = provider.iconName();
        }
        return QVariant::fromValue(data->accountIcon);
    }

    if (role == ProviderNameRole) {
        Accounts::Provider provider = d->manager->provider(account->providerName());
        data->providerName = provider.name();
        return QVariant::fromValue(data->providerName);
    }

    if (role == ProviderDisplayNameRole) {
        Accounts::Provider provider = d->manager->provider(account->providerName());
        data->providerDisplayName = SailfishAccounts::translatedDisplayName(provider);
        return QVariant::fromValue(data->providerDisplayName);
    }

    if (role == AccountEnabledRole) {
        return QVariant::fromValue(account->enabled());
    }

    return QVariant();
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
            reload();
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
    int accountIndex = getFilteredAccountsIndex(id);
    if (accountIndex < 0) {
        return;
    }
    emit dataChanged(index(accountIndex, 0), index(accountIndex, 0));
}

void AccountModel::accountDisplayNameChanged()
{
    Accounts::Account *account = qobject_cast<Accounts::Account*>(sender());
    if (account)
        accountUpdated(account->id());
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
    }
}

void AccountModel::removedAccount(Accounts::Account *account)
{
    if (account) {
        QObject::disconnect(account, SIGNAL(displayNameChanged(QString)),
                this, SLOT(accountDisplayNameChanged()));
    }
}

