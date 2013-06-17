TEMPLATE = lib
TARGET  = sailfishaccountsplugin
TARGET = $$qtLibraryTarget($$TARGET)

MODULENAME = Sailfish/Accounts
TARGETPATH = $$[QT_INSTALL_QML]/$$MODULENAME

equals(QT_MAJOR_VERSION, 5): QT += qml quick
equals(QT_MAJOR_VERSION, 4): QT += declarative
CONFIG += plugin

CONFIG += link_pkgconfig
PKGCONFIG += accounts-qt5 libsignon-qt5

SOURCES += \
    $$PWD/accountfactory.cpp \
    $$PWD/encodedkeyprovider.cpp \
    $$PWD/account.cpp \
    $$PWD/accountmanager.cpp \
    $$PWD/accountmodel.cpp \
    $$PWD/globalaccountmanager_p.cpp \
    $$PWD/plugin.cpp \
    $$PWD/provider.cpp \
    $$PWD/providermodel.cpp \
    $$PWD/service.cpp \
    $$PWD/servicetype.cpp \
    $$PWD/signinparameters.cpp

HEADERS += \
    $$PWD/accountfactory_p.h \
    $$PWD/encodedkeyprovider_p.h \
    $$PWD/account.h \
    $$PWD/accountmanager.h \
    $$PWD/accountmanager_p.h \
    $$PWD/accountmodel.h \
    $$PWD/account_p.h \
    $$PWD/accountvalueencoding_p.h \
    $$PWD/accountvalueencryption_osslevp_p.h \
    $$PWD/accountvalueencryption_p.h \
    $$PWD/accountvalueencryption_qca_p.h \
    $$PWD/globalaccountmanager_p.h \
    $$PWD/provider.h \
    $$PWD/providermodel.h \
    $$PWD/service.h \
    $$PWD/servicetype.h \
    $$PWD/signinparameters.h

OTHER_FILES += $$PWD/*.qml $$PWD/*.js qmldir

# We can use either QCA or OpenSSL-EVP for AES encryption of credentials
CONFIG(qca_encryption) {
    message("Building Sailfish.Accounts with QCA credentials encryption")
    DEFINES += USE_QCA_ENCRYPTION
    LIBS += -lqca
    CONFIG += crypto
    HEADERS += $$PWD/accountvalueencryption_qca_p.h
    HEADERS -= $$PWD/accountvalueencoding_p.h
}
CONFIG(osslevp_encryption) {
    message("Building Sailfish.Accounts with OpenSSL EVP credentials encryption")
    DEFINES += USE_OSSLEVP_ENCRYPTION
    PKGCONFIG += libcrypto
    HEADERS += $$PWD/accountvalueencryption_osslevp_p.h
    HEADERS -= $$PWD/accountvalueencoding_p.h
}
# if neither of these are specified, we fall back to xor encoding

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

import.files = qmldir
import.path = $$TARGETPATH
target.path = $$TARGETPATH

components.files = $$PWD/*.qml $$PWD/*.js qmldir
components.path = $$TARGETPATH
QMAKE_EXTRA_TARGETS += translations engineering_english
PRE_TARGETDEPS += translations engineering_english

INSTALLS += target import components translations_install engineering_english_install
