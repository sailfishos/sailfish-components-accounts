/****************************************************************************************
** Copyright (c) 2014 - 2023 Jolla Ltd.
**
** All rights reserved.
**
** This file is part of Sailfish Accounts components package.
**
** You may use this file under the terms of BSD license as follows:
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
** 1. Redistributions of source code must retain the above copyright notice, this
**    list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above copyright notice,
**    this list of conditions and the following disclaimer in the documentation
**    and/or other materials provided with the distribution.
**
** 3. Neither the name of the copyright holder nor the names of its
**    contributors may be used to endorse or promote products derived from
**    this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
** SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
** CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
** OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
****************************************************************************************/

#include "globaltranslatorcache_p.h"
#include <QThreadStorage>
#include <QTranslator>
#include <QHash>
#include <QString>
#include <QByteArray>
#include <QFileInfo>
#include <QLocale>
#include <QCoreApplication>
#include <QtDebug>

#include <Accounts/Provider>
#include <Accounts/Service>
#include <Accounts/ServiceType>

namespace SailfishAccounts {

static QTranslator* libEngEngTranslator = nullptr;
static QTranslator* libTranslator = nullptr;

void initLibTranslator()
{
    if (!libEngEngTranslator) {
        libEngEngTranslator = new QTranslator(qApp);
        libEngEngTranslator->load(QString::fromLatin1("sailfishaccounts_eng_en"),
                                  QString::fromLatin1("/usr/share/translations"));
        qApp->installTranslator(libEngEngTranslator);

        libTranslator = new QTranslator(qApp);
        libTranslator->load(QLocale(), QString::fromLatin1("sailfishaccounts"), QString::fromLatin1("-"),
                            QString::fromLatin1("/usr/share/translations"));
        qApp->installTranslator(libTranslator);
    }
}


class TranslatorManager
{
public:
    mutable QHash<QString, QTranslator*> translators;

    QTranslator *translator(const QString &trCatalog, bool engineeringEnglish)
    {
        QTranslator *translator = 0;
        QString catalogName = engineeringEnglish ? (trCatalog + QLatin1String("_eng_en")) : trCatalog;

        if (!translators.contains(catalogName)) {
            QFileInfo fi(catalogName);
            translator = new QTranslator;
            if (fi.isAbsolute()) {
                if (fi.exists()) {
                    // fully specified path to file
                    // engineering version just skipped, combination doesn't make sense
                    if (!engineeringEnglish) {
                        translator->load(catalogName);
                    }
                } else {
                    // partially specified path to file
                    QString trPath = fi.path();
                    QString trFile = fi.fileName();
                    if (engineeringEnglish) {
                        translator->load(trFile, trPath);
                    } else {
                        translator->load(QLocale(), trFile, "-", trPath);
                    }
                }
            } else {
                if (engineeringEnglish) {
                    translator->load(catalogName, "/usr/share/translations");
                } else {
                    translator->load(QLocale(), catalogName, "-", "/usr/share/translations");
                }
            }
            translators.insert(catalogName, translator);
        } else {
            translator = translators.value(catalogName);
        }
        return translator;
    }

    ~TranslatorManager()
    {
        qDeleteAll(translators.values());
    }
};

static QThreadStorage<TranslatorManager*> g_translationManager;

static QTranslator *cachedTranslator(const QString &trCatalog, bool engineeringEnglish)
{
    if (!g_translationManager.hasLocalData()) {
        g_translationManager.setLocalData(new TranslatorManager);
    }

    TranslatorManager *manager = g_translationManager.localData();
    return manager ? manager->translator(trCatalog, engineeringEnglish) : 0;
}

#define RETURN_TRANSLATED_DISPLAYNAME(instance)                                                         \
    do {                                                                                                \
        if (instance.trCatalog().isEmpty()) return instance.displayName();                              \
        QByteArray translationId = instance.displayName().toLatin1();                                   \
        QTranslator *translator = cachedTranslator(instance.trCatalog(), false);                        \
        QString retn = translator ? translator->translate("", translationId.constData()) : QString();   \
        if (!retn.isEmpty() && retn != translationId.constData()) return retn;                          \
        translator = cachedTranslator(instance.trCatalog(), true);                                      \
        retn = translator ? translator->translate("", translationId.constData()) : QString();           \
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
