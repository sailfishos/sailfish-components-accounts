import QtQuick 1.1
import Sailfish.Silica 1.0
import org.nemomobile.accounts 1.0

Page {
    id: root

    signal providerSelected(string providerName)

    AccountProviderModel { id: providerModel }
    AccountProvidersListView {
        id: aplv
        anchors.fill: parent
        providerModel: providerModel
        onProviderClicked: root.providerSelected(providerName)
    }
}
