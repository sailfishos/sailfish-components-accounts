/****************************************************************************************
** Copyright (c) 2013 - 2023 Jolla Ltd.
**
** All rights reserved.
**
** This file is part of Sailfish Accounts components package.
**
** You may use this file under the terms of BSD license as follows:
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
** 1. Redistributions of source code must retain the above copyright notice, this
**    list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above copyright notice,
**    this list of conditions and the following disclaimer in the documentation
**    and/or other materials provided with the distribution.
**
** 3. Neither the name of the copyright holder nor the names of its
**    contributors may be used to endorse or promote products derived from
**    this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
** SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
** CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
** OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
****************************************************************************************/
#include <QObject>
#include <QtTest>

#include "signinparameters.h"

class tst_SignInParameters : public QObject
{
    Q_OBJECT

private slots:
    //properties
    void properties();
};

void tst_SignInParameters::properties()
{
    QVariantMap params;
    params.insert("testKey", "testValue");
    QPointer<SignInParameters> si(new SignInParameters("test-service", "test-method", "test-mechanism", params, "user", "pass", this));
    QCOMPARE(si->serviceName(), QLatin1String("test-service"));
    QCOMPARE(si->method(), QLatin1String("test-method"));
    QCOMPARE(si->mechanism(), QLatin1String("test-mechanism"));
    QCOMPARE(si->parameters(), params);
    QCOMPARE(si->username(), QLatin1String("user"));
    QCOMPARE(si->password(), QLatin1String("pass"));

    QSignalSpy spy(si.data(), SIGNAL(parametersChanged()));
    int count = spy.count();
    params.insert("testKey2", "testValue2");
    si->setParameters(params);
    QTRY_COMPARE(spy.count(), count+1);
    QCOMPARE(si->parameters(), params);
}

#include "tst_signinparameters.moc"
QTEST_MAIN(tst_SignInParameters)
