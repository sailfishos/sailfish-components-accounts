.pragma library

function serviceName(serviceType, serviceDisplayName) {
    switch (serviceType) {
    case "e-mail":
        return qsTrId("components_accounts-la-service_name_email")
    case "IM":
        return qsTrId("components_accounts-la-service_name_im")
    case "microblogging":
        return qsTrId("components_accounts-la-service_name_microblogging")
    case "sharing":
        return qsTrId("components_accounts-la-service_name_sharing")
    case "sync":
        return qsTrId("components_accounts-la-service_name_sync")
    default:
        return serviceDisplayName
    }
}

function serviceDescription(serviceType, providerDisplayName) {
    switch (serviceType) {
    case "e-mail":
        return qsTrId("components_accounts-la-service_description_email").arg(providerDisplayName)
    case "IM":
        return qsTrId("components_accounts-la-service_description_im").arg(providerDisplayName)
    case "microblogging":
        return qsTrId("components_accounts-la-service_description_microblogging").arg(providerDisplayName)
    case "sharing":
        return qsTrId("components_accounts-la-service_description_sharing").arg(providerDisplayName)
    case "sync":
        return qsTrId("components_accounts-la-service_description_sync").arg(providerDisplayName)
    default:
        return ""
    }
}
