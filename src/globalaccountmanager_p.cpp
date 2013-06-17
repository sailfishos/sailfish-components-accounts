/*
 * Copyright (C) 2013 Jolla Ltd.
 * Contact: Chris Adams <chris.adams@jollamobile.com>
 *
 * License: Proprietary
 */

#include "globalaccountmanager_p.h"

static Accounts::Manager *g_accountManager = NULL;

Accounts::Manager *globalAccountManager()
{
    if (g_accountManager == NULL) {
        g_accountManager = new Accounts::Manager;
    }
    return g_accountManager;
}

