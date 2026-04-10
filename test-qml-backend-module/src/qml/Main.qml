import QtQuick

Item {
    id: root
    readonly property var backend: logos.module("test_qml_backend")

    Text {
        anchors.centerIn: parent
        text: backend ? "Backend status: " + backend.status : "Connecting..."
        color: "#ffffff"
    }
}
