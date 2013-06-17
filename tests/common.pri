include($$PWD/../src/src.pro)
SRCDIR = $$PWD/../src/
INCLUDEPATH += $$SRCDIR
DEPENDPATH = $$INCLUDEPATH
QT += testlib
TEMPLATE = app
CONFIG -= app_bundle

CONFIG += link_pkgconfig

equals(QT_MAJOR_VERSION, 4) {
    PKGCONFIG += accounts-qt QtDeclarative QtGui
    target.path = /opt/tests/Sailfish/Accounts
}
equals(QT_MAJOR_VERSION, 5) {
    PKGCONFIG += accounts-qt5 Qt5Qml Qt5Gui
    target.path = /opt/tests/Sailfish/Accounts/qt5
}

INSTALLS += target
