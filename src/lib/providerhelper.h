/*
 * Copyright (c) 2020 Open Mobile Platform LLC.
 *
 * License: Proprietary
 */

#ifndef SAILFISH_ACCOUNTS__PROVIDERHELPER_H
#define SAILFISH_ACCOUNTS__PROVIDERHELPER_H

#include <QSet>
#include <QString>

bool allowedProvider(const QSet<QString> &tags);

#endif
