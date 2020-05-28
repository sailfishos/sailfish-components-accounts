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
        $$PWD/*.qml

import.files = $$PWD/qmldir \
                $$PWD/AccountIcon.qml \
                $$PWD/AccountProviderPicker.qml \
                $$PWD/AccountProviderPickerDelegate.qml \
                $$PWD/AccountsListView.qml \
                $$PWD/AccountsListDelegate.qml \
                $$PWD/AccountsFlowView.qml
import.path = $$TARGETPATH
target.path = $$TARGETPATH

INSTALLS += target import
