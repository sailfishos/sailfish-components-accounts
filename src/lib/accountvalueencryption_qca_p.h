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

#ifndef SAILFISH_ACCOUNTS__ACCOUNTVALUEENCRYPTION_QCA_P_H
#define SAILFISH_ACCOUNTS__ACCOUNTVALUEENCRYPTION_QCA_P_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <QtCrypto/QtCrypto>

#include <QtDebug>

QByteArray qca_aes_encrypt_plaintext(const QByteArray &plaintext, const QByteArray &key)
{
    QCA::Initializer init;
    QCA::SecureArray plaintextArray(plaintext);

    if (plaintext.size() == 0 || key.size() == 0) {
        qWarning() << "qca_aes_encrypt_plaintext():"
                   << "invalid arguments, aborting encryption";
        return QByteArray();
    }

    if(!QCA::isSupported("aes128-cbc-pkcs7")) {
        qWarning() << "qca_aes_encrypt_plaintext():"
                   << "failed to initialize encryption context";
        return QByteArray();
    }

    QCA::SymmetricKey symmetricKey(key);
    QCA::InitializationVector initVector(QByteArray("Sailfish.Accounts"));

    QCA::Cipher cipher(QString("aes128"),
                       QCA::Cipher::CBC,
                       QCA::Cipher::DefaultPadding,
                       QCA::Encode,
                       symmetricKey,
                       initVector);

    QCA::SecureArray partial = cipher.update(plaintextArray);
    if (!cipher.ok()) {
        qWarning() << "qca_aes_encrypt_plaintext():"
                   << "encryption failed";
        return QByteArray();
    }

    QCA::SecureArray final = cipher.final();
    if (!cipher.ok()) {
        qWarning() << "qca_aes_encrypt_plaintext():"
                   << "encryption finalization failed";
        return QByteArray();
    }

    QCA::SecureArray ciphertext = partial.append(final);
    return ciphertext.toByteArray();
}

QByteArray qca_aes_decrypt_ciphertext(const QByteArray &ciphertext, const QByteArray &key)
{
    QCA::Initializer init;
    QCA::SecureArray ciphertextArray(ciphertext);

    if (ciphertext.size() == 0 || key.size() == 0) {
        qWarning() << "qca_aes_decrypt_ciphertext():"
                   << "invalid arguments, aborting decryption";
        return QByteArray();
    }

    if(!QCA::isSupported("aes128-cbc-pkcs7")) {
        qWarning() << "qca_aes_decrypt_ciphertext():"
                   << "failed to initialize decryption context";
        return QByteArray();
    }

    QCA::SymmetricKey symmetricKey(key);
    QCA::InitializationVector initVector(QByteArray("Sailfish.Accounts"));

    QCA::Cipher cipher(QString("aes128"),
                       QCA::Cipher::CBC,
                       QCA::Cipher::DefaultPadding,
                       QCA::Decode,
                       symmetricKey,
                       initVector);

    QCA::SecureArray partial = cipher.update(ciphertextArray);
    if (!cipher.ok()) {
        qWarning() << "qca_aes_decrypt_ciphertext():"
                   << "decryption failed";
        return QByteArray();
    }

    QCA::SecureArray final = cipher.final();
    if (!cipher.ok()) {
        qWarning() << "qca_aes_decrypt_ciphertext():"
                   << "decryption finalization failed: key failure";
        return QByteArray();
    }

    QCA::SecureArray plaintext = partial.append(final);
    return plaintext.toByteArray();
}

#endif
