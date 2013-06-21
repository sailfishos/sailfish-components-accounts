TEMPLATE = subdirs
SUBDIRS = src tests src/plugin
src/plugin.depends = src

OTHER_FILES += rpm/*.spec
