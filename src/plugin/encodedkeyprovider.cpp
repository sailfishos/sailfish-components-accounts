#include "encodedkeyprovider_p.h"

//libsailfishkeyprovider
#include <sailfishkeyprovider.h>

#include <QtDebug>

QString encodedKeyValue(const char *provider, const char *service, const char *key)
{
    // step one: grab the appropriate keys from the keyprovider
    // step two: encode them with "xor" / "jolla".

    char *ptv = NULL;
    char *ev = NULL;
    QString retn;

    int success = SailfishKeyProvider_storedKey(provider, service, key, &ptv);
    if (success == 0 && ptv != NULL && strlen(ptv) != 0) {
        success = SailfishKeyProvider_encodeKey(ptv, "xor", "jolla", &ev);
        if (success == 0 && ev != NULL && strlen(ev) != 0) {
            retn = QLatin1String(ev);
        }
    }

    free(ev);
    free(ptv);
    return retn;
}

EncodedKeyProvider::EncodedKeyProvider(QObject *parent)
    : QObject(parent)
{
}

QVariantMap EncodedKeyProvider::encodedKeys(const QString &providerName) const
{
    QVariantMap retn;
    if (providerName == QLatin1String("facebook")) {
        retn.insert(QLatin1String("ClientId"), encodedKeyValue("facebook", "facebook-sync", "client_id"));
    } else if (providerName == QLatin1String("google")) {
        retn.insert(QLatin1String("ClientId"), encodedKeyValue("google", "google-sync", "client_id"));
        retn.insert(QLatin1String("ClientSecret"), encodedKeyValue("google", "google-sync", "client_secret"));
    } else if (providerName == QLatin1String("twitter")) {
        retn.insert(QLatin1String("ConsumerKey"), encodedKeyValue("twitter", "twitter-sync", "consumer_key"));
        retn.insert(QLatin1String("ConsumerSecret"), encodedKeyValue("twitter", "twitter-sync", "consumer_secret"));
    } else if (providerName == QLatin1String("flickr")) {
        retn.insert(QLatin1String("ConsumerKey"), encodedKeyValue("flickr", "flickr-sharing", "consumer_key"));
        retn.insert(QLatin1String("ConsumerSecret"), encodedKeyValue("flickr", "flickr-sharing", "consumer_secret"));
    } else {
        qWarning() << Q_FUNC_INFO << "no keys known for provider:" << providerName;
    }

    return retn;
}

QString EncodedKeyProvider::decodeKey(const QString &encodedKey, const QString &scheme, const QString &decodingKey) const
{
    QString retn;
    QByteArray ekba = encodedKey.toLatin1();
    QByteArray sba = scheme.toLatin1();
    QByteArray dkba = decodingKey.toLatin1();
    const char *ekc = ekba.constData();
    const char *sc = sba.constData();
    const char *dkc = dkba.constData();

    char *dv = NULL;
    int success = SailfishKeyProvider_decodeKey(ekc, sc, dkc, &dv);
    if (success == 0 && dv != NULL && strlen(dv) != 0) {
        retn = QLatin1String(dv);
    }

    free(dv);
    return retn;
}
