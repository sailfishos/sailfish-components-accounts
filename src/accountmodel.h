/*
 * Copyright (C) 2013 Jolla Ltd.
 * Contact: Chris Adams <chris.adams@jollamobile.com>
 *
 * License: Proprietary
 */

#ifndef SAILFISH_ACCOUNTS__ACCOUNTMODEL_H
#define SAILFISH_ACCOUNTS__ACCOUNTMODEL_H

//accounts-qt
#include <Accounts/Manager>

//Qt
#include <QQmlParserStatus>
#include <QAbstractTableModel>
#include <QMap>
#include <QVariant>

class Provider;
class Account;

/*!
 * The Account Model is the model for created accounts
 */

class AccountModel : public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_PROPERTY(FilterType filterType READ filterType WRITE setFilterType NOTIFY filterTypeChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_ENUMS(FilterType)
    class AccountModelPrivate;

public:
    enum Roles {
        AccountIdRole = Qt::UserRole + 1,
        AccountDisplayNameRole,
        AccountIconRole,
        ProviderNameRole,
        ProviderDisplayNameRole,
        AccountEnabledRole
    };

    enum FilterType {
        NoFilter,
        ProviderFilter,
        ServiceFilter,
        ServiceTypeFilter
    };

    AccountModel(QObject *parent = 0);
    virtual ~AccountModel();

    FilterType filterType() const;
    void setFilterType(FilterType filterType);

    QString filter() const;
    void setFilter(const QString &filter);

    int rowCount(const QModelIndex &index = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;

    void classBegin();
    void componentComplete();

signals:
    void filterTypeChanged();
    void filterChanged();

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
protected:
    QHash<int, QByteArray> roleNames() const;
#endif

private slots:
    void accountCreated(Accounts::AccountId id);
    void accountRemoved(Accounts::AccountId id);
    void accountUpdated(Accounts::AccountId id);
    void accountDisplayNameChanged();

private:
    int getAccountIndex(Accounts::AccountId id) const;
    int getFilteredAccountsIndex(Accounts::AccountId id) const;
    void addedAccount(Accounts::Account *account);
    void removedAccount(Accounts::Account *account);
    void reload();

private:
    AccountModelPrivate* d_ptr;
    Q_DISABLE_COPY(AccountModel)
    Q_DECLARE_PRIVATE(AccountModel);
};
Q_DECLARE_METATYPE(Accounts::Account *)

#endif
