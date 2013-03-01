#ifndef ENCODEDKEYPROVIDER_P_H
#define ENCODEDKEYPROVIDER_P_H

#include <QtCore/QObject>
#include <QtCore/QVariantMap>
#include <QtCore/QString>

class EncodedKeyProvider : public QObject
{
    Q_OBJECT

public:
    EncodedKeyProvider(QObject *parent = 0);
    Q_INVOKABLE QVariantMap encodedKeys(const QString &providerName) const;
};

#endif // ENCODEDKEYPROVIDER_P_H
