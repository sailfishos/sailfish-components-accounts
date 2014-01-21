TEMPLATE = subdirs
SUBDIRS = src tests tools
OTHER_FILES += rpm/*.spec
include(doc/doc.pri)
