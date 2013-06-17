/*
 * Copyright (C) 2013 Jolla Ltd.
 * Contact: Chris Adams <chris.adams@jollamobile.com>
 *
 * License: Proprietary
 */

#ifndef SAILFISH_ACCOUNTS__SIGNINPARAMETERS_H
#define SAILFISH_ACCOUNTS__SIGNINPARAMETERS_H

#include <QObject>
#include <QVariantMap>
#include <QString>
#include <QStringList>
#include <QVariant>

class SignInParameters : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString method READ method CONSTANT)
    Q_PROPERTY(QString mechanism READ mechanism CONSTANT)
    Q_PROPERTY(QVariantMap parameters READ parameters NOTIFY parametersChanged)
    Q_PROPERTY(QString username READ username CONSTANT)
    Q_PROPERTY(QString password READ password CONSTANT)

public:
    ~SignInParameters();
    SignInParameters(const QString &method = QString(),
                     const QString &mechanism = QString(),
                     const QVariantMap &parameters = QVariantMap(),
                     const QString &username = QString(),
                     const QString &password = QString(),
                     QObject *parent = 0);

    // property accessors.
    QString method() const;
    QString mechanism() const;
    QVariantMap parameters() const;
    QString username() const;
    QString password() const;

    // invokable api.
    void setParameter(const QString &parameterName, const QVariant &parameterValue);
    void setParameter(const QString &parameterName, const QString &parameterValue);
    void setParameter(const QString &parameterName, const QStringList &parameterValue);
    void setParameter(const QString &parameterName, const QUrl &parameterValue);
    void setParameter(const QString &parameterName, int parameterValue);
    void setParameter(const QString &parameterName, bool parameterValue);
    void removeParameter(const QString &parameterName);

Q_SIGNALS:
    void parametersChanged();

private:
    QString m_method;
    QString m_mechanism;
    QVariantMap m_parameters;
    QString m_username;
    QString m_password;
};

#endif
