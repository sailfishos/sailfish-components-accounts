TEMPLATE=lib
TARGET = sailfishaccountsplugin
MODULENAME = Sailfish/Accounts
TARGETPATH = $$[QT_INSTALL_QML]/$$MODULENAME

QT += qml
CONFIG += qt plugin hide_symbols link_pkgconfig Qt5Qml
PKGCONFIG += libsailfishkeyprovider accounts-qt5 libsignon-qt5

INCLUDEPATH += $$PWD/../lib
LIBS += -L$$PWD/../lib -lsailfishaccounts

SOURCES += $$PWD/plugin.cpp

OTHER_FILES += \
        $$PWD/qmldir \
        $$PWD/AccountIcon.qml \
        $$PWD/AccountProviderPicker.qml \
        $$PWD/AccountProviderPickerDialog.qml \
        $$PWD/AccountsListView.qml

TS_FILE = $$OUT_PWD/sailfish_components_accounts_qt5.ts
EE_QM = $$OUT_PWD/sailfish_components_accounts_qt5_eng_en.qm

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

import.files = $$PWD/qmldir $$PWD/AccountIcon.qml $$PWD/AccountProviderPicker.qml $$PWD/AccountProviderPickerDialog.qml $$PWD/AccountsListView.qml
import.path = $$TARGETPATH
target.path = $$TARGETPATH

QMAKE_EXTRA_TARGETS += translations engineering_english
PRE_TARGETDEPS += translations engineering_english

INSTALLS += target import translations_install engineering_english_install
