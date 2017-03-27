/*
 * Copyright (C) 2013 Jolla Ltd.
 * Contact: Chris Adams <chris.adams@jollamobile.com>
 *
 * License: Proprietary
 */

#ifndef SAILFISH_ACCOUNTS__PROVIDERMODEL_H
#define SAILFISH_ACCOUNTS__PROVIDERMODEL_H

//Qt
#include <QAbstractListModel>
#include <QQmlParserStatus>
#include <QStringList>

class Q_DECL_EXPORT ProviderModel : public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT
    class ProviderModelPrivate;
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QStringList serviceFilter READ serviceFilter WRITE setServiceFilter NOTIFY serviceFilterChanged)

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

    int rowCount( const QModelIndex & index = QModelIndex() ) const;
    QVariant data( const QModelIndex &index, int role ) const;

    void classBegin();
    void componentComplete();

signals:
    void serviceFilterChanged();

protected:
    QHash<int, QByteArray> roleNames() const;

private:
    ProviderModelPrivate* d_ptr;
    Q_DISABLE_COPY(ProviderModel)
    Q_DECLARE_PRIVATE(ProviderModel);
};

#endif
