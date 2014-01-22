/*
 * Copyright (C) 2014 Jolla Ltd.
 * Contact: Chris Adams <chris.adams@jollamobile.com>
 *
 * License: Proprietary
 */

#include "globaltranslatorcache_p.h"
#include <QThreadStorage>
#include <QTranslator>
#include <QHash>
#include <QString>
#include <QByteArray>

#include <Accounts/Provider>
#include <Accounts/Service>
#include <Accounts/ServiceType>

namespace SailfishAccounts {

class TranslatorManager
{
public:
    mutable QHash<QString, QTranslator*> translators;

    QTranslator *translator(const QString &trCatalog)
    {
        QTranslator *translator = 0;
        if (!translators.contains(trCatalog)) {
            translator = new QTranslator;
            translator->load(trCatalog);
            translators.insert(trCatalog, translator);
        } else {
            translator = translators.value(trCatalog);
        }
        return translator;
    }

    ~TranslatorManager()
    {
        qDeleteAll(translators.values());
    }
};

QThreadStorage<TranslatorManager*> g_translationManager;
QTranslator *cachedTranslator(const QString &trCatalog)
{
    if (!g_translationManager.hasLocalData()) {
        g_translationManager.setLocalData(new TranslatorManager);
    }

    TranslatorManager *manager = g_translationManager.localData();
    return manager ? manager->translator(trCatalog) : 0;
}

#define RETURN_TRANSLATED_DISPLAYNAME(instance)                                                         \
    do {                                                                                                \
        QByteArray translationId = instance.displayName().toLatin1();                                   \
        QTranslator *translator = cachedTranslator(instance.trCatalog());                               \
        QString retn = translator ? translator->translate("", translationId.constData()) : QString();   \
        return retn.isEmpty() ? instance.displayName() : retn;                                          \
    } while (0)                                                                                         \

QString translatedDisplayName(const Accounts::Provider &provider)
{
    RETURN_TRANSLATED_DISPLAYNAME(provider);
}

QString translatedDisplayName(const Accounts::Service &service)
{
    RETURN_TRANSLATED_DISPLAYNAME(service);
}

QString translatedDisplayName(const Accounts::ServiceType &serviceType)
{
    RETURN_TRANSLATED_DISPLAYNAME(serviceType);
}

#undef RETURN_TRANSLATED_DISPLAYNAME

}
