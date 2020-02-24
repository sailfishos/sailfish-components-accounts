TEMPLATE = lib
TARGET  = sailfishaccounts
TARGET = $$qtLibraryTarget($$TARGET)
TARGETPATH = $$[QT_INSTALL_LIBS]

QT += qml dbus
CONFIG += qt hide_symbols create_pc create_prl no_install_prl link_pkgconfig
PKGCONFIG += libsailfishkeyprovider accounts-qt5 libsignon-qt5 buteosyncfw5 mlite5
LIBS += -lssu

SOURCES += \
    $$PWD/account.cpp \
    $$PWD/accountmanager.cpp \
    $$PWD/accountmodel.cpp \
    $$PWD/globalaccountmanager_p.cpp \
    $$PWD/globaltranslatorcache_p.cpp \
    $$PWD/provider.cpp \
    $$PWD/providermodel.cpp \
    $$PWD/service.cpp \
    $$PWD/servicetype.cpp \
    $$PWD/servicemodel.cpp \
    $$PWD/signinparameters.cpp \
    $$PWD/accountsyncmanager.cpp \
    $$PWD/accountsyncoptions.cpp

HEADERS += \
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
    $$PWD/globaltranslatorcache_p.h \
    $$PWD/provider.h \
    $$PWD/providermodel.h \
    $$PWD/service.h \
    $$PWD/servicetype.h \
    $$PWD/servicemodel.h \
    $$PWD/signinparameters.h \
    $$PWD/accountsyncmanager.h \
    $$PWD/accountsyncoptions.h \
    $$PWD/accountsyncoptions_p.h

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

develheaders.path = /usr/include/libsailfishaccounts
develheaders.files = \
    $$PWD/account.h \
    $$PWD/accountmanager.h \
    $$PWD/provider.h \
    $$PWD/service.h \
    $$PWD/servicetype.h \
    $$PWD/signinparameters.h \
    $$PWD/accountsyncmanager.h \
    $$PWD/accountsyncoptions.h

target.path = $$[QT_INSTALL_LIBS]
pkgconfig.files = $$PWD/pkgconfig/sailfishaccounts.pc
pkgconfig.path = $$target.path/pkgconfig

QMAKE_PKGCONFIG_NAME = lib$$TARGET
QMAKE_PKGCONFIG_DESCRIPTION = Application-segregated encrypted account credentials development files
QMAKE_PKGCONFIG_LIBDIR = $$target.path
QMAKE_PKGCONFIG_INCDIR = $$develheaders.path
QMAKE_PKGCONFIG_DESTDIR = pkgconfig
QMAKE_PKGCONFIG_REQUIRES = Qt5Qml Qt5Xml libsailfishkeyprovider accounts-qt5 libsignon-qt5
QMAKE_PKGCONFIG_VERSION = $$VERSION

INSTALLS += target develheaders pkgconfig
