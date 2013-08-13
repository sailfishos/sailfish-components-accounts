equals(QT_MAJOR_VERSION, 4): QDOC = qdoc3
equals(QT_MAJOR_VERSION, 5): QDOC = qdoc
QHELPGENERATOR = qhelpgenerator
QDOCCONF = config/sailfishaccounts.qdocconf
QHELPFILE = html/sailfishaccounts.qhp
QCHFILE = html/sailfishaccounts.qch

docs.commands = ($$QDOC $$PWD/$$QDOCCONF) && \
                ($$QHELPGENERATOR $$PWD/$$QHELPFILE -o $$PWD/$$QCHFILE)

QMAKE_EXTRA_TARGETS += docs
