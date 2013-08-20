import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Accounts 1.0

Dialog {
    id: root

    property int providerCount: view.count

    property int _selectionCount
    property alias _viewHeader: view.header
    property alias _accountManager: view._accountManager

    signal providerSelected(int index, string providerName)
    signal providerDeselected(int index, string providerName)

    canAccept: _selectionCount > 0

    AccountProviderPicker {
        id: view
        anchors.fill: parent

        header: DialogHeader {
            //: Number of selected accounts
            //% "%n selected"
            acceptText: qsTrId("components_accounts-he-selected_accounts", root._selectionCount)
            dialog: root
        }

        onProviderSelected: {
            _selectionCount++
            root.providerSelected(index, providerName)
        }

        onProviderDeselected: {
            _selectionCount--
            root.providerDeselected(index, providerName)
        }
    }
}
