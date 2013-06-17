/*
 * Copyright (C) 2013 Jolla Ltd.
 * Contact: Chris Adams <chris.adams@jollamobile.com>
 *
 * License: Proprietary
 */

#ifndef SAILFISH_ACCOUNTS__PROVIDERMODEL_H
#define SAILFISH_ACCOUNTS__PROVIDERMODEL_H

//accounts-qt
#include <Accounts/Manager>

//Qt
#include <QAbstractTableModel>
#include <QDomDocument>
#include <QString>

class Provider;
class ProviderModel : public QAbstractListModel
{
    Q_OBJECT
    class ProviderModelPrivate;

public:

    enum Roles{
        ProviderNameRole = Qt::UserRole + 1,
        ProviderDisplayNameRole,
        ProviderDescriptionRole,
        ProviderIconRole
    };

    ProviderModel(QObject* parent = 0);
    ~ProviderModel();

    int rowCount( const QModelIndex & index = QModelIndex() ) const;
    QVariant data( const QModelIndex &index, int role ) const;

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
protected:
    QHash<int, QByteArray> roleNames() const;
#endif

private:
    ProviderModelPrivate* d_ptr;
    Q_DISABLE_COPY(ProviderModel)
    Q_DECLARE_PRIVATE(ProviderModel);
};

#endif
