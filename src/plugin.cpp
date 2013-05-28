#include <QQmlExtensionPlugin>
#include <QQmlEngine>
#include <QtQml>

#include <QTranslator>
#include <QGuiApplication>
#include <QLocale>

#include "encodedkeyprovider_p.h"
#include "accountfactory_p.h"

// using custom translator so it gets properly removed from qApp when engine is deleted
class AppTranslator: public QTranslator
{
    Q_OBJECT
public:
    AppTranslator(QObject *parent)
        : QTranslator(parent)
    {
        qApp->installTranslator(this);
    }

    virtual ~AppTranslator()
    {
        qApp->removeTranslator(this);
    }
};


class SailfishAccountsPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.sailfish.components.accounts")

public:

    void initializeEngine(QQmlEngine *engine, const char *uri)
    {
        Q_UNUSED(uri)
        Q_ASSERT(QLatin1String(uri) == QLatin1String("Sailfish.Accounts"));

        AppTranslator *engineeringEnglish = new AppTranslator(engine);
        AppTranslator *translator = new AppTranslator(engine);
        engineeringEnglish->load("sailfish_components_accounts_qt5_eng_en", "/usr/share/translations");
        translator->load(QLocale(), "sailfish_components_accounts_qt5", "-", "/usr/share/translations");
    }

    virtual void registerTypes(const char *uri)
    {
        Q_UNUSED(uri)
        Q_ASSERT(QLatin1String(uri) == QLatin1String("Sailfish.Accounts"));
        // private types.
        qmlRegisterType<EncodedKeyProvider>("Sailfish.Accounts.private", 1, 0, "EncodedKeyProvider");
        qmlRegisterType<AccountFactory>("Sailfish.Accounts.private", 1, 0, "AccountFactory");
    }
};

#include "plugin.moc"

