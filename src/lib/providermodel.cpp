/*
 * Copyright (C) 2013 Jolla Ltd.
 * Contact: Chris Adams <chris.adams@jollamobile.com>
 *
 * License: Proprietary
 */

//project
#include "providermodel.h"
#include "provider.h"
#include "globalaccountmanager_p.h"
#include "globaltranslatorcache_p.h"

//Qt
#include <QDebug>
#include <QHash>

//libaccounts-qt
#include <Accounts/Manager>

class ProviderModel::ProviderModelPrivate
{
public:
    ProviderModelPrivate()
        : componentComplete(false)
    {
    }

    ~ProviderModelPrivate() {}

    bool providerMatchesServiceFilter(const QString &providerName) {
        if (serviceFilters.isEmpty()) {
            return true;
        }
        QStringList supportedServices = providerServiceTypes[providerName];
        if (!supportedServices.isEmpty()) {
            Q_FOREACH (const QString &service, serviceFilters) {
                if (supportedServices.contains(service, Qt::CaseInsensitive)) {
                    return true;
                }
            }
        }
        return false;
    }

    void reloadProviders() {
        QList<Accounts::Provider> result;
        for (int i=0; i<providerList.count(); i++) {
            const Accounts::Provider &p = providerList.at(i);
            if (providerMatchesServiceFilter(p.name())) {
                result.append(p);
            }
        }
        filteredProviderList = result;
    }

    QList<Accounts::Provider> providerList;
    QList<Accounts::Provider> filteredProviderList;
    QHash<QString, QStringList> providerServiceTypes;
    QHash<int, QByteArray> headerData;
    QStringList serviceFilters;
    bool componentComplete;
};

static QString retrieveDescription(const Accounts::Provider &provider)
{
    QDomElement root = provider.domDocument().documentElement();
    QDomElement descriptionElement = root.firstChildElement("description");
    if (!descriptionElement.text().isEmpty()) {
        return descriptionElement.text();
    } else {
        return QString();
    }
}

/*!
    \qmltype ProviderModel
    \instantiates ProviderModel
    \inherits QAbstractListModel
    \inqmlmodule Sailfish.Accounts 1
    \brief Provides a model of existing account providers

    The ProviderModel can be used to provide account provider data to a view.
    For each account provider in the database, it exposes the following roles:
    \list
    \li \c providerName
    \li \c providerDisplayName
    \li \c providerDescription
    \li \c providerIcon
    \endlist
*/

ProviderModel::ProviderModel(QObject* parent)
    : QAbstractListModel(parent)
    , d_ptr(new ProviderModelPrivate)
{
    Q_D(ProviderModel);

    d->headerData.insert(ProviderNameRole, "providerName");
    d->headerData.insert(ProviderDisplayNameRole, "providerDisplayName");
    d->headerData.insert(ProviderDescriptionRole, "providerDescription" );
    d->headerData.insert(ProviderIconRole, "providerIcon");

    Accounts::Manager *m = globalAccountManager();
    Accounts::ServiceList allServices = m->serviceList(); // force reload of service files.
    Accounts::ProviderList providers = m->providerList();

    foreach (const Accounts::Provider &provider, providers) {
        for (QList<Accounts::Service>::iterator it = allServices.begin(); it != allServices.end();) {
            const Accounts::Service &service = *it;
            if (service.provider() == provider.name()) {
                d->providerServiceTypes[service.provider()].append(service.serviceType());
                it = allServices.erase(it);
            } else {
                ++it;
            }
        }
        d->providerList.append(provider);
    }
    std::sort(d->providerList.begin(), d->providerList.end(), [](const Accounts::Provider &a, const Accounts::Provider &b) {
        return SailfishAccounts::translatedDisplayName(a).localeAwareCompare(SailfishAccounts::translatedDisplayName(b)) < 0;
    });
    d->filteredProviderList = d->providerList;
}

QHash<int, QByteArray> ProviderModel::roleNames() const
{
    Q_D(const ProviderModel);
    return d->headerData;
}

ProviderModel::~ProviderModel()
{
    Q_D(ProviderModel);

    delete d;
}

/*!
    \qmlproperty list ProviderModel::serviceFilter

    Controls the providers that are listed in the model. This is a list of strings of the services
    that must be supported by the listed providers.

    For example, if the serviceFilter is ["e-mail", "IM"], then the model will only include
    providers with "e-mail" or "IM" services. The service string comparison is case-insensitive.

    If the serviceFilter is an empty string, then all providers are included in the model regardless
    of their supported services. This is the default value.
 */

QStringList ProviderModel::serviceFilter() const
{
    Q_D(const ProviderModel);
    return d->serviceFilters;
}

void ProviderModel::setServiceFilter(const QStringList &serviceFilter)
{
    Q_D(ProviderModel);
    if (serviceFilter != d->serviceFilters) {
        d->serviceFilters = serviceFilter;
        if (d->componentComplete) {
            beginResetModel();
            d->reloadProviders();
            endResetModel();
        }
        emit serviceFilterChanged();
    }
}

int ProviderModel::rowCount(const QModelIndex &) const
{
    Q_D(const ProviderModel);
    return d->filteredProviderList.count();
}

QVariant ProviderModel::data(const QModelIndex& index, int role) const
{
    Q_D(const ProviderModel);
    if (!index.isValid() || index.row() >= d->filteredProviderList.length())
        return QVariant();

    Accounts::Provider provider = d->filteredProviderList.at(index.row());
    if (!provider.isValid())
        return QVariant();

    const Role modelRole = static_cast<Role>(role);
    switch (modelRole) {
    case ProviderNameRole:
        return provider.name();
    case ProviderDisplayNameRole:
        return SailfishAccounts::translatedDisplayName(provider);
    case ProviderDescriptionRole:
        return retrieveDescription(provider);
    case ProviderIconRole:
        return provider.iconName();
    }

    return QVariant();
}

void ProviderModel::classBegin()
{
}

void ProviderModel::componentComplete()
{
    Q_D(ProviderModel);
    if (!d->serviceFilters.isEmpty()) {
        beginResetModel();
        d->reloadProviders();
        endResetModel();
    }
    d->componentComplete = true;
}

Q_DECLARE_METATYPE(Accounts::Provider)
