SRCDIR = $$PWD/../src/lib
INCLUDEPATH += $$SRCDIR
DEPENDPATH = $$INCLUDEPATH
QT += testlib
TEMPLATE = app
CONFIG -= app_bundle

CONFIG += link_pkgconfig

PKGCONFIG += libsignon-qt5 accounts-qt5 Qt5Qml Qt5Gui
target.path = /opt/tests/Sailfish/Accounts/qt5

LIBS += -L$$SRCDIR -lsailfishaccounts

INSTALLS += target
