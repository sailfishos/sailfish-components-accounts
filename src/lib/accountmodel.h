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

class Q_DECL_EXPORT AccountModel : public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(FilterType filterType READ filterType WRITE setFilterType NOTIFY filterTypeChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_ENUMS(FilterType)
    Q_ENUMS(AccountError)
    class AccountModelPrivate;

public:
    enum Roles {
        AccountIdRole = Qt::UserRole + 1,
        AccountDisplayNameRole,
        AccountIconRole,
        ProviderNameRole,
        ProviderDisplayNameRole,
        AccountEnabledRole,
        AccountErrorRole,
        PerformingInitialSyncRole,
        AccountUserNameRole
    };

    enum FilterType {
        NoFilter,
        ProviderFilter,
        ServiceFilter,
        ServiceTypeFilter
    };

    enum AccountError {
        NoAccountError,
        AccountNotSignedInError,
        UnknownAccountError = 100
    };

    AccountModel(QObject *parent = 0);
    virtual ~AccountModel();

    FilterType filterType() const;
    void setFilterType(FilterType filterType);

    QString filter() const;
    void setFilter(const QString &filter);

    int count() const;

    Q_INVOKABLE void setAccountEnabled(int index, bool enabled);
    Q_INVOKABLE QVariantMap getByAccount(int accountId);
    Q_INVOKABLE QVariantMap get(int index);
    Q_INVOKABLE bool accountHasServiceOfTypeEnabled(int accountId, const QString &serviceTypeName);

    int rowCount(const QModelIndex &index = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;

    void classBegin();
    void componentComplete();

signals:
    void filterTypeChanged();
    void filterChanged();
    void countChanged();

protected:
    QHash<int, QByteArray> roleNames() const;

private slots:
    void accountCreated(Accounts::AccountId id);
    void accountRemoved(Accounts::AccountId id);
    void accountUpdated(Accounts::AccountId id);
    void accountDisplayNameChanged();
    void accountEnabledChanged();
    void delayedIndexUpdate();
    void exchangeSyncStarted(qulonglong accountId);
    void exchangeSyncCompleted(qulonglong accountId, int result);
    void profileSyncStatusChanged(const QString &profileId, int status, const QString &errorString);

private:
    int getAccountIndex(Accounts::AccountId id) const;
    int getFilteredAccountsIndex(Accounts::AccountId id) const;
    void addedAccount(Accounts::Account *account);
    void removedAccount(Accounts::Account *account);
    void reload();
    void monitorSyncStatus(Accounts::Account *account);

private:
    AccountModelPrivate* d_ptr;
    Q_DISABLE_COPY(AccountModel)
    Q_DECLARE_PRIVATE(AccountModel)
};
Q_DECLARE_METATYPE(Accounts::Account *)

#endif
