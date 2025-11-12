# SPDX-FileCopyrightText: 2013 - 2023 Jolla Ltd.
# SPDX-FileCopyrightText: 2025 Jolla Mobile Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

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
        $$PWD/plugins.qmltypes \
        $$PWD/*.qml

import.files = $$PWD/qmldir \
                $$PWD/plugins.qmltypes \
                $$PWD/AccountIcon.qml \
                $$PWD/AccountProviderPicker.qml \
                $$PWD/AccountProviderPickerDelegate.qml \
                $$PWD/AccountsListView.qml \
                $$PWD/AccountsListDelegate.qml \
                $$PWD/AccountsFlowView.qml
import.path = $$TARGETPATH
target.path = $$TARGETPATH

INSTALLS += target import

# Invoke directly to deal with circular dependency with silica submodules - keep
# just the Sailfish.Silica.private dependency to break the cycle.
qtPrepareTool(QMLIMPORTSCANNER, qmlimportscanner)
qmltypes.commands = \
    echo -e $$shell_quote('import Sailfish.Accounts 1.0\nQtObject{}\n') \
        |$$QMLIMPORTSCANNER -qmlFiles - -importPath $$[QT_INSTALL_QML] \
        |sed -e $$shell_quote('/"Sailfish.Silica"/,/{/d') \
        |sed -e $$shell_quote('/"Sailfish.Silica.Background"/,/{/d') > dependencies.json && \
    qmlplugindump -nonrelocatable -dependencies dependencies.json \
         Sailfish.Accounts 1.0 > $$PWD/plugins.qmltypes
QMAKE_EXTRA_TARGETS += qmltypes
