/*
** Copyright (C) 2014 Jolla Ltd.
*/

#ifndef ACCOUNTMODIFIER_P_H
#define ACCOUNTMODIFIER_P_H

#include <QObject>
#include <QString>

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

public Q_SLOTS:
    void start();

Q_SIGNALS:
    void done();
};

#endif