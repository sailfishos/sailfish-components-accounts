/*
** Copyright (C) 2014 Jolla Ltd.
*/

#ifndef ACCOUNTMODIFIER_P_H
#define ACCOUNTMODIFIER_P_H

#include <QObject>
#include <QString>

#include <Accounts/Error>
#include <Accounts/Account>
#include <Accounts/Manager>

class AccountModifier : public QObject
{
    Q_OBJECT

public:
    QString providerName;
    QString serviceName;
    QString modeSwitch;
    QString settingName;
    QString settingType;
    QString settingValue;

    AccountModifier(QObject *parent = 0);
    ~AccountModifier();

public Q_SLOTS:
    void start();
    void next();
    void error(Accounts::Error err);

Q_SIGNALS:
    void done();

private:
    Accounts::Manager *m_accountManager;
    Accounts::AccountIdList m_allAccountIds;
    Accounts::Account *m_currAccount;
    int m_currAccountIdx;
};

#endif