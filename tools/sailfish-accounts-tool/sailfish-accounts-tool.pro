VERSION = 0.0.1
PROJECT_NAME = sailfish-accounts-tool
TEMPLATE = app
CONFIG += hide_symbols

CONFIG += link_pkgconfig
PKGCONFIG += accounts-qt5 libsignon-qt5 buteosyncfw5 libsailfishkeyprovider

QMAKE_CXXFLAGS += -fPIC -fvisibility=hidden -fvisibility-inlines-hidden

LIBS += -L../../src/lib -lsailfishaccounts -pie -rdynamic
INCLUDEPATH += . ../../src/lib

SOURCES += accountmodifier.cpp accountbackuprestorer.cpp main.cpp
HEADERS += accountmodifier_p.h accountbackuprestorer_p.h

TARGET = $$PROJECT_NAME
target.path = $$INSTALL_ROOT/usr/bin/
INSTALLS += target
