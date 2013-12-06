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

//Qt
#include <QDebug>
#include <QStringList>
#include <QDir>
#include <QPointer>
#include <QCoreApplication>
#include <QMap>
#include <QtAlgorithms>

//libaccounts-qt
#include <Accounts/Manager>

class ProviderModel::ProviderModelPrivate
{
public:
    ~ProviderModelPrivate() {}
    QList<Accounts::Provider> providerList;
    QHash<int, QByteArray> headerData;
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

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    setRoleNames(d->headerData);
#endif
    Accounts::Manager *m = globalAccountManager();
    Accounts::ServiceList allServices = m->serviceList(); // force reload of service files.
    Accounts::ProviderList providers = m->providerList();

    for (int i = 0; i < providers.size(); i++) {
        QDomDocument domDocument = providers[i].domDocument();

        // add it sorted by provider display name
        bool addedProvider = false;
        for (int j = 0; j < d->providerList.size(); ++j) {
            if (providers[i].displayName() < d->providerList[j].displayName()) {
                d->providerList.insert(j, providers[i]);
                addedProvider = true;
                break;
            }
        }

        if (!addedProvider) {
            d->providerList << providers[i];
        }
    }
}

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
QHash<int, QByteArray> ProviderModel::roleNames() const
{
    Q_D(const ProviderModel);
    return d->headerData;
}
#endif


ProviderModel::~ProviderModel()
{
    Q_D(ProviderModel);

    delete d;
}

int ProviderModel::rowCount(const QModelIndex& parent) const
{
    Q_D(const ProviderModel);
    if (parent.isValid()) {
        return 0;
    }

    return d->providerList.count();
}

QVariant ProviderModel::data(const QModelIndex& index, int role) const
{
    Q_D(const ProviderModel);
    if (!index.isValid() || index.row() >= d->providerList.length())
        return QVariant();

    Accounts::Provider provider = d->providerList.at(index.row());
    if (!provider.isValid())
        return QVariant();

    if (role == ProviderNameRole)
        return provider.name();

    if (role == ProviderDisplayNameRole)
        return provider.displayName();

    if (role == ProviderDescriptionRole)
        return retrieveDescription(provider);

    if (role == ProviderIconRole)
        return provider.iconName();

    return QVariant();
}

Q_DECLARE_METATYPE(Accounts::Provider)
