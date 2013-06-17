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

//Qt
#include <QtDebug>

//libaccounts-qt
#include <Accounts/Manager>
#include <Accounts/Account>
#include <Accounts/Provider>

struct DisplayData {
    DisplayData(Accounts::Account *acct) : account(acct) {}
    ~DisplayData() { delete account; }
    Accounts::Account *account;
    QString providerName;
    QString providerDisplayName;
    QString accountIcon;
    Q_DISABLE_COPY(DisplayData);
};

class AccountModel::AccountModelPrivate
{
public:
    ~AccountModelPrivate()
    {
        qDeleteAll(accountsList);
    }

    QHash<int, QByteArray> headerData;
    Accounts::Manager *manager;
    QList<DisplayData *> accountsList;
};


/*!
    \qmltype AccountModel
    \instantiates AccountModel
    \inqmlmodule Sailfish.Accounts 1
    \brief Provides a model of existing accounts

    The AccountModel can be used to provide account data to a view.
    For each account in the database, it exposes:
    \list
    \li accountId
    \li accountDisplayName
    \li accountIcon
    \li providerName
    \li providerDisplayName
    \li accountEnabled
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
        addedAccount(account);
        d->accountsList.append(new DisplayData(account));
    }
}

AccountModel::~AccountModel()
{
}

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
QHash<int, QByteArray> AccountModel::roleNames() const
{
    Q_D(const AccountModel);
    return d->headerData;
}
#endif

int AccountModel::rowCount(const QModelIndex &parent) const
{
    Q_D(const AccountModel);
    if (parent.isValid()) {
        return 0;
    }

    return d->accountsList.length();
}

QVariant AccountModel::data(const QModelIndex &index, int role) const
{
    Q_D(const AccountModel);
    if (!index.isValid() || index.row() >= d->accountsList.length()) {
        return QVariant();
    }

    DisplayData *data = d->accountsList[index.row()];
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
        data->providerDisplayName = provider.displayName();
        return QVariant::fromValue(data->providerDisplayName);
    }

    if (role == AccountEnabledRole) {
        return QVariant::fromValue(account->enabled());
    }

    return QVariant();
}

void AccountModel::accountCreated(Accounts::AccountId id)
{
    Q_D(AccountModel);
    QModelIndex index;
    Accounts::Account *account = d->manager->account(id);
    addedAccount(account);

    if (account != 0) {
        beginInsertRows(index, 0, 0);
        d->accountsList.insert(0, new DisplayData(account));
        endInsertRows();
    }
}

void AccountModel::accountRemoved(Accounts::AccountId id)
{
    Q_D(AccountModel);

    int index = getAccountIndex(id);

    if (index < 0) {
        qWarning() << Q_FUNC_INFO << "Account not present in the list:" << id;
        return;
    }

    QModelIndex parent;
    beginRemoveRows(parent, index, index);
    DisplayData *data = d->accountsList.takeAt(index);
    endRemoveRows();

    removedAccount(data->account);
    delete data;
}

void AccountModel::accountUpdated(Accounts::AccountId id)
{
    int accountIndex = getAccountIndex(id);
    if (accountIndex < 0) {
        qWarning() << Q_FUNC_INFO << "Account not present in the list:" << id;
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

