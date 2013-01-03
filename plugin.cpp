#include <QDeclarativeExtensionPlugin>
#include <QTranslator>
#include <QApplication>
#include <QDeclarativeEngine>
#include <QLocale>

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


class ComponentsAccountsPlugin : public QDeclarativeExtensionPlugin
{
    Q_OBJECT

public:

    void initializeEngine(QDeclarativeEngine *engine, const char *uri)
    {
        Q_UNUSED(uri)
        Q_ASSERT(QLatin1String(uri) == QLatin1String("com.jolla.components.accounts"));

        AppTranslator *engineeringEnglish = new AppTranslator(engine);
        AppTranslator *translator = new AppTranslator(engine);
        engineeringEnglish->load("components_accounts_eng_en", "/usr/share/translations");
        translator->load(QLocale(), "components_accounts", "-", "/usr/share/translations");
    }

    virtual void registerTypes(const char *uri)
    {
        Q_UNUSED(uri)
        Q_ASSERT(QLatin1String(uri) == QLatin1String("com.jolla.components.accounts"));
    }
};

#include "plugin.moc"

Q_EXPORT_PLUGIN2(componentsaccountsplugin, ComponentsAccountsPlugin);

