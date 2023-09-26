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

#ifndef SAILFISH_ACCOUNTS__PROVIDERMODEL_H
#define SAILFISH_ACCOUNTS__PROVIDERMODEL_H

//Qt
#include <QAbstractListModel>
#include <QQmlParserStatus>
#include <QStringList>

/*
  Filter options:
  - serviceFilter: Only include providers with these services
  - providerFilter: Only include providers with these names
  - excludeProvidersForUncreatableAccounts: if ProviderIsSingleAccount=true and an account exists, exclude the provider
  - otherExcludedProviders: Additionally exclude providers with these names

  Providers must match all conditions to be included in the model.
*/

class Q_DECL_EXPORT ProviderModel : public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT
    class ProviderModelPrivate;
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(int count READ rowCount NOTIFY rowCountChanged)
    Q_PROPERTY(QStringList providerNames READ providerNames NOTIFY providerNamesChanged)

    Q_PROPERTY(QStringList serviceFilter READ serviceFilter WRITE setServiceFilter NOTIFY serviceFilterChanged)
    Q_PROPERTY(QStringList providerFilter READ providerFilter WRITE setProviderFilter NOTIFY providerFilterChanged)
    Q_PROPERTY(bool excludeProvidersForUncreatableAccounts READ excludeProvidersForUncreatableAccounts WRITE setExcludeProvidersForUncreatableAccounts NOTIFY excludeProvidersForUncreatableAccountsChanged)
    Q_PROPERTY(QStringList otherExcludedProviders READ otherExcludedProviders WRITE setOtherExcludedProviders NOTIFY otherExcludedProvidersChanged)

public:
    enum Role {
        ProviderNameRole = Qt::UserRole + 1,
        ProviderDisplayNameRole,
        ProviderDescriptionRole,
        ProviderIconRole,
        ProviderIsSingleAccountRole
    };

    ProviderModel(QObject* parent = 0);
    ~ProviderModel();

    QStringList serviceFilter() const;
    void setServiceFilter(const QStringList &serviceFilter);

    QStringList providerFilter() const;
    void setProviderFilter(const QStringList &providerFilter);

    QStringList otherExcludedProviders() const;
    void setOtherExcludedProviders(const QStringList &otherExcludedProviders);

    bool excludeProvidersForUncreatableAccounts() const;
    void setExcludeProvidersForUncreatableAccounts(bool excludeProvidersForUncreatableAccounts);

    QStringList providerNames() const;

    int rowCount( const QModelIndex & index = QModelIndex() ) const override;
    QVariant data( const QModelIndex &index, int role ) const override;

    void classBegin() override;
    void componentComplete() override;

Q_SIGNALS:
    void rowCountChanged();
    void providerNamesChanged();
    void serviceFilterChanged();
    void providerFilterChanged();
    void otherExcludedProvidersChanged();
    void excludeProvidersForUncreatableAccountsChanged();

protected:
    QHash<int, QByteArray> roleNames() const;

private:
    ProviderModelPrivate* d_ptr;
    Q_DISABLE_COPY(ProviderModel)
    Q_DECLARE_PRIVATE(ProviderModel);
};

#endif
