# SPDX-FileCopyrightText: 2013 - 2014 Jolla Ltd.
# SPDX-FileCopyrightText: 2025 Jolla Mobile Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

TEMPLATE = subdirs
SUBDIRS = \
    tst_account \
    tst_accountmanager \
    tst_accountmodel \
    tst_provider \
    tst_service \
    tst_servicetype \
    tst_signinparameters

tests_xml.target = tests.xml
tests_xml.files = tests.xml
tests_xml.path = /opt/tests/Sailfish/Accounts/qt5
INSTALLS += tests_xml

tests_provider.target = test-provider.provider
tests_provider.files = test-provider.provider
tests_provider.path = /usr/share/accounts/providers
INSTALLS += tests_provider

tests_service.files = test-service2.service test-service-oauth.service
tests_service.path = /usr/share/accounts/services
INSTALLS += tests_service

tests_service_type.files = test-service-type2.service-type
tests_service_type.path = /usr/share/accounts/service_types
INSTALLS += tests_service_type
