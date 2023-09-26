/****************************************************************************************
** Copyright (c) 2013 - 2023 Jolla Ltd.
**
** All rights reserved.
**
** This file is part of Sailfish Accounts components package.
**
** You may use this file under the terms of BSD license as follows:
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
** 1. Redistributions of source code must retain the above copyright notice, this
**    list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above copyright notice,
**    this list of conditions and the following disclaimer in the documentation
**    and/or other materials provided with the distribution.
**
** 3. Neither the name of the copyright holder nor the names of its
**    contributors may be used to endorse or promote products derived from
**    this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
** SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
** CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
** OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
****************************************************************************************/

#include "servicemodel.h"
#include "service.h"
#include "globalaccountmanager_p.h"
#include "globaltranslatorcache_p.h"

//Qt
#include <QDebug>
#include <QHash>

//libaccounts-qt
#include <Accounts/Manager>

class ServiceModel::ServiceModelPrivate
{
public:
    ServiceModelPrivate()
        : componentComplete(false)
    {
    }

    ~ServiceModelPrivate() {}

    void applyServiceTypeFilter() {
        QList<Accounts::Service> result;
        foreach (const Accounts::Service &service, serviceList) {
            if (service.serviceType() == serviceTypeFilter) {
                result.append(service);
            }
        }
        filteredServiceList = result;
    }

    QList<Accounts::Service> serviceList;
    QList<Accounts::Service> filteredServiceList;
    QHash<QString, QStringList> providerServiceTypes;
    QHash<int, QByteArray> roleNames;
    QString serviceTypeFilter;
    bool componentComplete;
};

/*!
    \qmltype ServiceModel
    \instantiates ServiceModel
    \inherits QAbstractListModel
    \inqmlmodule Sailfish.Accounts 1
    \brief Provides a model of existing account provider services

    The ServiceModel can be used to provide a model of account provider services.

    The following roles are exposed for each service:

    \list
    \li \c serviceName
    \li \c serviceDisplayName
    \li \c serviceIcon
    \endlist
*/

ServiceModel::ServiceModel(QObject* parent)
    : QAbstractListModel(parent)
    , d_ptr(new ServiceModelPrivate)
{
    Q_D(ServiceModel);

    d->roleNames.insert(ServiceNameRole, "serviceName");
    d->roleNames.insert(ServiceDisplayNameRole, "serviceDisplayName");
    d->roleNames.insert(ServiceIconRole, "serviceIcon");
    d->roleNames.insert(ServiceTypeRole, "serviceType");
    d->roleNames.insert(ServiceTagsRole, "serviceTags");
    d->roleNames.insert(ServiceProviderNameRole, "serviceProviderName");

    Accounts::Manager *m = globalAccountManager();
//    Accounts::ServiceList allServices = m->serviceList(); // force reload of service files.
    d->serviceList = m->serviceList(); // force reload of service files.
    d->filteredServiceList = d->serviceList;
}

QHash<int, QByteArray> ServiceModel::roleNames() const
{
    Q_D(const ServiceModel);
    return d->roleNames;
}

ServiceModel::~ServiceModel()
{
    Q_D(ServiceModel);
    delete d;
}

/*!
    \qmlproperty string ServiceModel::serviceTypeFilter

    If set, the model will only contain services with the specified service type.
 */

QString ServiceModel::serviceTypeFilter() const
{
    Q_D(const ServiceModel);
    return d->serviceTypeFilter;
}

void ServiceModel::setServiceTypeFilter(const QString &serviceTypeFilter)
{
    Q_D(ServiceModel);
    if (serviceTypeFilter != d->serviceTypeFilter) {
        d->serviceTypeFilter = serviceTypeFilter;
        if (d->componentComplete) {
            beginResetModel();
            d->applyServiceTypeFilter();
            endResetModel();
        }
        emit serviceTypeFilterChanged();
    }
}

int ServiceModel::rowCount(const QModelIndex &) const
{
    Q_D(const ServiceModel);
    return d->filteredServiceList.count();
}

QVariant ServiceModel::data(const QModelIndex& index, int role) const
{
    Q_D(const ServiceModel);
    if (!index.isValid() || index.row() >= d->filteredServiceList.length())
        return QVariant();

    Accounts::Service service = d->filteredServiceList.at(index.row());
    if (!service.isValid())
        return QVariant();

    switch (role) {
    case ServiceNameRole:
        return service.name();
    case ServiceDisplayNameRole:
        return SailfishAccounts::translatedDisplayName(service);
    case ServiceIconRole:
        return service.iconName();
    case ServiceTypeRole:
        return service.serviceType();
    case ServiceTagsRole:
        return QVariant::fromValue(service.tags().toList());
    case ServiceProviderNameRole:
        return service.provider();
    }
    return QVariant();
}

void ServiceModel::classBegin()
{
}

void ServiceModel::componentComplete()
{
    Q_D(ServiceModel);
    if (!d->serviceTypeFilter.isEmpty()) {
        beginResetModel();
        d->applyServiceTypeFilter();
        endResetModel();
    }
    d->componentComplete = true;
}
