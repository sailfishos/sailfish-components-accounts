//.pragma library // if pragma library, cannot access Component.

// providerName OR accountId must be given
function openAccountCreationPage(page, replace, accountsModel, providerName, accountId) {
    var provider = null
    if (accountId == 0) {
        // creating a new account.
        provider = accountsModel.provider(providerName)
        if (!provider) {
            throw new Error("Unable to obtain provider with name: " + providerName)
        }
    } else {
        // editing an existing account.
        provider = accountsModel.provider(accountId)
        if (!provider) {
            throw new Error("Unable to ascertain provider for account with id: " + accountId)
        }
    }

    // load the per-provider account creation page
    var componentName = "../extensions/" + provider.name + ".qml"
    var comp = Qt.createComponent(componentName)
    if (comp.status !== Component.Ready) {
        throw new Error("Error creating provider-specific account creation page for provider \'" + provider.name + "\': " + comp.errorString())
    }
    var obj = comp.createObject(page, { "accountsModel": accountsModel, "provider": provider, "accountId": accountId })

    // either replace or push
    if (replace) {
        pageStack.replace(obj)
    } else {
        pageStack.push(obj)
    }
}

// existingAccountId MUST be non-zero
function openAccountSettingsPage(page, replace, accountsModel, accountId) {
    var componentName = "AccountSettingsPage.qml"
    var comp = Qt.createComponent(componentName)
    if (comp.status !== Component.Ready) {
        throw new Error("Error creating account settings page for account \'" + accountId + "\': " + comp.errorString())
    }
    var obj = comp.createObject(page, { "accountsModel": accountsModel, "accountId": accountId })

    // either replace or push
    if (replace) {
        pageStack.replace(obj)
    } else {
        pageStack.push(obj)
    }
}
