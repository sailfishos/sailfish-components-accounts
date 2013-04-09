#include "encodedkeyprovider_p.h"
#include "accountvalueencoding_p.h"

#include <QtDebug>

EncodedKeyProvider::EncodedKeyProvider(QObject *parent)
    : QObject(parent)
{
}

QVariantMap EncodedKeyProvider::encodedKeys(const QString &providerName) const
{
    // all keys can be decoded via Account.decodeConfigurationValue(value, "xor", "jolla")
    QVariantMap retn;
    if (providerName == QLatin1String("facebook")) {
        retn.insert(QLatin1String("ClientId"), QLatin1String("JzslFS4uKl4hGz8XIQYoECIoJVU="));
    } else if (providerName == QLatin1String("google")) {
        retn.insert(QLatin1String("ClientId"), QLatin1String("JBULXSwQCF4jNT8YIQYsXiMBKhYJJyEZO1hWGjZTEgMINC8GDAEiFwgBPgADBD4ZNVNTGw=="));
        retn.insert(QLatin1String("ClientSecret"), QLatin1String("OzoIXjYsAwo5DSQ5P1xQLyJdCDs4KTYfBVgpBTVSBig="));
    } else if (providerName == QLatin1String("twitter")) {
        retn.insert(QLatin1String("ConsumerKey"), QLatin1String("OAEEOwIQLhsONSQAOSQ7AT4WPlEJKBQELDIDJT0wV1I="));
        retn.insert(QLatin1String("ConsumerSecret"), QLatin1String("DzcEAQVbPTohNQ44OysvEwsqADAJOQgpLj4+FDUNLDwPXicjCwEILTAoHAAvBiUfDzRTFyEoBV8+PVFc"));
    } else if (providerName == QLatin1String("flickr")) {
        retn.insert(QLatin1String("ConsumerKey"), QLatin1String("JDg6BC8tOhQhJSdeISgrBiIrIVIwKzleLAc6GyE1PAQjOy8HIRYEDSQCNVE="));
        retn.insert(QLatin1String("ConsumerSecret"), QLatin1String("MzsEBSwANl0jJS9fIjgzBzU4BAwkLlFR"));
    } else {
        qWarning() << Q_FUNC_INFO << "no keys known for provider:" << providerName;
    }

    return retn;
}

QString EncodedKeyProvider::decodeKey(const QString &encodedKey, const QString &scheme, const QString &decodingKey) const
{
    return decodeValue(encodedKey, scheme, decodingKey);
}
