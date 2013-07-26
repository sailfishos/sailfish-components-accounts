/*
 * Copyright (C) 2013 Jolla Ltd.
 * Contact: Chris Adams <chris.adams@jollamobile.com>
 *
 * License: Proprietary
 */

#include "globalaccountmanager_p.h"
#include <QThreadStorage>

QThreadStorage<Accounts::Manager *> g_accountManagers;
Accounts::Manager *globalAccountManager()
{
    if (!g_accountManagers.hasLocalData()) {
        g_accountManagers.setLocalData(new Accounts::Manager);
    }

    return g_accountManagers.localData();
}

