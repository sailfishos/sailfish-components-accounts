TEMPLATE = lib
TARGET  = jollacomponentsaccountsplugin
TARGET = $$qtLibraryTarget($$TARGET)

MODULENAME = com/jolla/components/accounts
TARGETPATH = $$[QT_INSTALL_IMPORTS]/$$MODULENAME

QT += declarative
CONFIG += plugin

SOURCES += plugin.cpp
OTHER_FILES += *.qml settings/*qml *.png *.service *.provider settings/*.json

TS_FILE = $$OUT_PWD/components_accounts.ts
EE_QM = $$OUT_PWD/components_accounts_eng_en.qm

translations.commands += lupdate $$PWD -ts $$TS_FILE
translations.depends = $$PWD/*.qml
translations.CONFIG += no_check_exist no_link
translations.output = $$TS_FILE
translations.input = .

translations_install.files = $$TS_FILE
translations_install.path = /usr/share/translations/source
translations_install.CONFIG += no_check_exist


# should add -markuntranslated "-" when proper translations are in place (or for testing)
engineering_english.commands += lrelease -idbased $$TS_FILE -qm $$EE_QM
engineering_english.CONFIG += no_check_exist no_link
engineering_english.depends = translations
engineering_english.input = $$TS_FILE
engineering_english.output = $$EE_QM

engineering_english_install.path = /usr/share/translations
engineering_english_install.files = $$EE_QM
engineering_english_install.CONFIG += no_check_exist

import.files = *.qml icon-l-google.png qmldir images
import.path = $$TARGETPATH
target.path = $$TARGETPATH

providers.files = jolla-google.provider
providers.path = /usr/share/accounts/providers/

services.files = jolla-google-talk.service
services.path = /usr/share/accounts/services/

settings_entry.files = settings/accounts.json
settings_entry.path = /usr/share/jolla-settings/entries/
settings_page.files = settings/mainpage.qml
settings_page.path = /usr/share/jolla-settings/pages/accounts/

QMAKE_EXTRA_TARGETS += translations engineering_english
PRE_TARGETDEPS += translations engineering_english

INSTALLS += target import providers services settings_entry settings_page translations_install engineering_english_install
