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

#ifndef SAILFISH_ACCOUNTS__ACCOUNTVALUEENCRYPTION_P_H
#define SAILFISH_ACCOUNTS__ACCOUNTVALUEENCRYPTION_P_H

#include <QByteArray>

#ifdef USE_OSSLEVP_ENCRYPTION
#include "accountvalueencryption_osslevp_p.h"
#elif USE_QCA_ENCRYPTION
#include "accountvalueencryption_qca_p.h"
#else
#include "accountvalueencoding_p.h"
#endif

QByteArray aes_encrypt_plaintext(const QByteArray &plaintext, const QByteArray &key)
{
#ifdef USE_OSSLEVP_ENCRYPTION
    QByteArray encryptedData;
    unsigned char *encrypted = NULL;
    int size = osslevp_aes_encrypt_plaintext((const unsigned char *)key.constData(),
                                             key.size(),
                                             (const unsigned char *)plaintext.constData(),
                                             plaintext.size(),
                                             &encrypted);
    if (size <= 0) {
        return encryptedData;
    }

    encryptedData = QByteArray::fromRawData((char *)encrypted, size);
    free(encrypted);
    return encryptedData;
#elif USE_QCA_ENCRYPTION
    return qca_aes_encrypt_plaintext(plaintext, key);
#else
    return encodeValue(QString::fromUtf8(plaintext), QLatin1String("xor"), QString::fromUtf8(key)).toLatin1();
#endif
}

QByteArray aes_decrypt_ciphertext(const QByteArray &ciphertext, const QByteArray &key)
{
#ifdef USE_OSSLEVP_ENCRYPTION
    QByteArray decryptedData;
    unsigned char *decrypted = NULL;
    int size = osslevp_aes_decrypt_ciphertext((const unsigned char *)key.constData(),
                                              key.size(),
                                              (const unsigned char *)ciphertext.constData(),
                                              ciphertext.size(),
                                              &decrypted);
    if (size <= 0) {
        return decryptedData;
    }

    decryptedData = QByteArray::fromRawData((char *)decrypted, size);
    free(decrypted);
    return decryptedData;
#elif USE_QCA_ENCRYPTION
    return qca_aes_decrypt_ciphertext(ciphertext, key);
#else
    return decodeValue(QString::fromUtf8(ciphertext), QLatin1String("xor"), QString::fromUtf8(key)).toLatin1();
#endif
}

#endif
