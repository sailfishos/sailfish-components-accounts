# SPDX-FileCopyrightText: 2013 - 2023 Jolla Ltd.
# SPDX-FileCopyrightText: 2024 - 2025 Jolla Mobile Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

SRCDIR = $$PWD/../src/lib
INCLUDEPATH += $$SRCDIR
DEPENDPATH = $$INCLUDEPATH
QT += testlib
TEMPLATE = app

CONFIG += link_pkgconfig

PKGCONFIG += libsignon-qt5 accounts-qt5 Qt5Qml Qt5Gui
target.path = /opt/tests/Sailfish/Accounts/qt5

LIBS += -L$$SRCDIR -lsailfishaccounts

INSTALLS += target
