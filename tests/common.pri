include($$PWD/../src/src.pro)
SRCDIR = $$PWD/../src/
INCLUDEPATH += $$SRCDIR
DEPENDPATH = $$INCLUDEPATH
QT += testlib
TEMPLATE = app
CONFIG -= app_bundle

CONFIG += link_pkgconfig

PKGCONFIG += libsignon-qt5 accounts-qt5 Qt5Qml Qt5Gui
target.path = /opt/tests/Sailfish/Accounts/qt5

INSTALLS += target
