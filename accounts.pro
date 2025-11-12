# SPDX-FileCopyrightText: 2012 - 2014 Jolla Ltd.
# SPDX-FileCopyrightText: 2024 - 2025 Jolla Mobile Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

TEMPLATE = subdirs
SUBDIRS = src tests tools
OTHER_FILES += rpm/*.spec
include(doc/doc.pri)
tools.depends = src
tests.depends = src
