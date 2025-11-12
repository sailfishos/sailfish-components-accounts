# SPDX-FileCopyrightText: 2013 - 2014 Jolla Ltd.
# SPDX-FileCopyrightText: 2025 Jolla Mobile Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

QDOC = qdoc
QHELPGENERATOR = qhelpgenerator
QDOCCONF = config/sailfishaccounts.qdocconf
QHELPFILE = html/sailfishaccounts.qhp
QCHFILE = html/sailfishaccounts.qch

docs.commands = ($$QDOC $$PWD/$$QDOCCONF) && \
                ($$QHELPGENERATOR $$PWD/$$QHELPFILE -o $$PWD/$$QCHFILE)

QMAKE_EXTRA_TARGETS += docs
