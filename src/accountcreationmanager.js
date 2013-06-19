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
    var provider = accountModel.provider(providerName)
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
        // Once this page becomes visible, we load its settings page
        if (obj.status === PageStatus.Active) {
            var settingsProperties = {
                "accountProvider": obj.accountProvider,
                "isNewAccount": true
            }
            // notify creation page that the settings page is ready
            obj.settingsPage = _cachedSettingsPage(providerName, settingsProperties)
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
    obj.statusChanged.connect(function(){
        // If this is for a brand new account, once this settings page becomes visible, we load
        // its destination (i.e. the account creation page for the next provider in the queue)
        if (obj.isNewAccount && obj.status === PageStatus.Active) {
            if (!obj.acceptDestination) {
                obj.acceptDestination = _nextAccountCreationPage()
                if (obj.acceptDestination === null)
                    obj.acceptDestination = accountCreationManager.finalPageDestination
            }
        }
    })
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

function _cachedSettingsPage(providerName, props) {
    var page = null
    for (var i=0; i<_accountCreationQueue.length; i++) {
        var data = _accountCreationQueue[i]
        if (data.providerName === providerName) {
            page = data.settingsPage
            if (page === undefined) {
                page = createSettingsPage(data.providerName, props)
                _accountCreationQueue[i].settingsPage = page
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
