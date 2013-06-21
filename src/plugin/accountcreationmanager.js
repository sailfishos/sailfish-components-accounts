var _accountCreationQueue = []
var _currentCreationIndex = -1
var _firstSelectedProviderIndex = -1


function startAccountCreation() {
    var picker = pageStack.push(Qt.resolvedUrl("AccountProviderPickerDialog.qml"),
                             {"acceptDestinationAction": PageStackAction.Replace})
    if (picker === null) {
        console.log("Cannot load AccountProviderPickerDialog!")
        return
    }
    initAccountCreationQueue(picker.providerCount)
    _currentCreationIndex = 0
    _firstSelectedProviderIndex = -1
    picker.providerSelected.connect(_selectedProviderToCreate)
    picker.providerDeselected.connect(_deselectedProviderToCreate)
    picker.acceptPendingChanged.connect(function() {
        picker.acceptDestination = _firstCachedCreationPage()
    })
    picker.accepted.connect(function() {
        // remove unselected providers from the account creation queue
        for (var i=0; i<_accountCreationQueue.length; i++) {
            var data = _accountCreationQueue[i]
            if (data.providerName === "") {
                // at this point we have only constructed the creationPage; other pages don't need
                // to be cleaned up
                if (data.creationPage !== undefined) {
                    data.creationPage.destroy()
                }
                _accountCreationQueue.splice(i, 1)
                i--
            }
        }
    })
    return picker
}

function createAccountCreationPage(providerName) {
    if (providerName === "") {
        console.log("No account provider name given!")
        return null
    }
    var provider = accountManager.provider(providerName)
    if (!provider) {
        throw new Error("Unable to obtain provider with name: " + providerName)
    }
    var componentFileName = "/usr/share/accounts/ui/" + providerName + ".qml"
    var comp = Qt.createComponent(componentFileName)
    if (comp.status !== Component.Ready) {
        throw new Error("Unable to load account creation page "
                        + componentFileName + ": " + comp.errorString())
    }
    var obj = comp.status === Component.Ready
            ? comp.createObject(accountCreationManager, {"accountProvider": provider})
            : null
    if (obj === null) {
        console.log("Error: cannot load account creation page for " + providerName)
        return null
    }
    obj.statusChanged.connect(function(){
        // Once this page becomes visible, we load its post-creation page and settings page
        if (obj.status === PageStatus.Active) {
            var settingsProperties = {
                "accountProvider": obj.accountProvider,
                "isNewAccount": true
            }
            obj.postCreationDialog = _cachedPostCreationDialog(providerName, settingsProperties)
        }
    })
    return obj
}

function createSettingsPage(providerName, properties) {
    var componentFileName = "/usr/share/accounts/ui/" + providerName + "-settings.qml"
    var comp = Qt.createComponent(componentFileName)
    if (comp.status !== Component.Ready) {
        comp = Qt.createComponent(Qt.resolvedUrl("StandardAccountSettingsDialog.qml"))
    }
    var obj = comp.status === Component.Ready
            ? comp.createObject(accountCreationManager, properties)
            : null
    if (obj === null) {
        console.log("Error: cannot find StandardAccountSettingsDialog.qml!")
        return null
    }
    // If this is for a brand new account, the destination of the settings page is the account
    // creation page for the next provider in the queue)
    if (obj.isNewAccount && !obj.acceptDestination) {
        obj.acceptDestination = _nextAccountCreationPage()
        if (obj.acceptDestination === null)
            obj.acceptDestination = accountCreationManager.finalPageDestination
    }
    obj.rejected.connect(function() {
        if (obj.isNewAccount) {
            accountCreationManager.accountDeletionRequested(obj.accountId)
        }
    })
    return obj
}

function clearAccountCreationQueue() {
   if (_accountCreationQueue.length === 0) {
        return
    }
    for (var i=0; i<_accountCreationQueue.length; i++) {
        var data = _accountCreationQueue[i]
        if (data.creationPage !== undefined) {
            data.creationPage.destroy()
        }
        if (data.postCreationDialog !== undefined) {
            data.postCreationDialog.destroy()
        }
        if (data.settingsPage !== undefined) {
            data.settingsPage.destroy()
        }
    }
    _accountCreationQueue = []
}

function _selectedProviderToCreate(index, providerName) {
    // mark provider as selected
    _accountCreationQueue[index].providerName = providerName

    // Pre-emptively load the first account creation page that is selected in the dialog,
    // otherwise this loading will cause a blocking delay when the picker is accepted.
    if (_firstSelectedProviderIndex < 0 || index < _firstSelectedProviderIndex) {
        _firstSelectedProviderIndex = index
        _cachedCreationPage(index)
    }
}

function _deselectedProviderToCreate(index, providerName) {
    // mark provider as unselected
    _accountCreationQueue[index].providerName = ""
}

function initAccountCreationQueue(initialLength) {
    if (_accountCreationQueue.length > 0) {
        clearAccountCreationQueue()
    }
    for (var i=0; i<initialLength; i++) {
        var data = {
            "providerName": "",
            "creationPage": undefined,
            "postCreationDialog": undefined,
            "settingsPage": undefined
        }
        _accountCreationQueue.push(data)
    }
}

function _nextAccountCreationPage() {
    _currentCreationIndex++
    if (_currentCreationIndex >= _accountCreationQueue.length) {
        return null
    }
    return _cachedCreationPage(_currentCreationIndex)
}

function _cachedCreationPage(index) {
    if (index < 0 || index > _accountCreationQueue.length) {
        return
    }
    var data = _accountCreationQueue[index]
    var page = data.creationPage
    if (page === undefined) {
        page = createAccountCreationPage(data.providerName)
        _accountCreationQueue[index].creationPage = page
    }
    return page
}

function _cachedPostCreationDialog(providerName, settingsProperties) {
    var page = null
    for (var i=0; i<_accountCreationQueue.length; i++) {
        var data = _accountCreationQueue[i]
        if (data.providerName === providerName) {
            page = data.postCreationDialog
            if (page === undefined) {
                var comp = Qt.createComponent(Qt.resolvedUrl("AccountPostCreationDialog.qml"))
                if (comp.status !== Component.Ready) {
                    console.log("Error: cannot load AccountPostCreationDialog.qml!", comp.errorString())
                    return null
                }
                var settingsPage = createSettingsPage(providerName, settingsProperties)
                _accountCreationQueue[i].settingsPage = settingsPage
                page = comp.createObject(accountCreationManager, {"settingsPage": settingsPage})
                _accountCreationQueue[i].postCreationDialog = page
            }
            break
        }
    }
    return page
}

function _firstCachedCreationPage() {
    for (var i=0; i<_accountCreationQueue.length; i++) {
        var data = _accountCreationQueue[i]
        if (data.providerName !== "") {
            return data.creationPage
        }
    }
    return null
}
