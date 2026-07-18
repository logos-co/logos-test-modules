import QtQuick

// View for the full_api UI plugin. Clickable buttons (distinct labels) drive the
// backend; Text labels bind the backend's synced PROPs. A headless UI doctest
// clicks the buttons and asserts on the status/event text. The call* functions
// are also exposed for call_method-style driving.
Item {
    id: root
    objectName: "fullApiUiRoot"
    width: 800
    height: 480

    readonly property var backend: logos.module("test_fullapi_ui")

    function callBindTo(name) { if (backend) backend.bindTo(name); }
    function callRunMethods() { if (backend) backend.runMethods(); }
    function callRunMethodsAsync() { if (backend) backend.runMethodsAsync(); }
    function callFireEvents() { if (backend) backend.fireEvents(); }

    component Btn: Rectangle {
        property string label: ""
        signal clicked()
        width: 200
        height: 40
        radius: 6
        color: tap.pressed ? "#3a6ea5" : "#2d2d3a"
        border.color: "#5a5a6a"
        Text {
            anchors.centerIn: parent
            text: parent.label
            color: "#ffffff"
        }
        MouseArea {
            id: tap
            anchors.fill: parent
            onClicked: parent.clicked()
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 10

        Row {
            spacing: 10
            Btn { label: "Bind CPP Provider"; onClicked: root.callBindTo("test_fullapi_cpp") }
            Btn { label: "Bind Rust Provider"; onClicked: root.callBindTo("test_fullapi_rust") }
            Btn { label: "Bind Proxy"; onClicked: root.callBindTo("test_fullapi_proxy") }
        }
        Row {
            spacing: 10
            Btn { label: "Run All Methods"; onClicked: root.callRunMethods() }
            Btn { label: "Run All (Async)"; onClicked: root.callRunMethodsAsync() }
            Btn { label: "Fire Events"; onClicked: root.callFireEvents() }
        }

        Text {
            objectName: "targetText"
            color: "#ffffff"
            text: backend ? ("target: " + backend.boundTarget) : "Connecting..."
        }
        // These two show the EXACT PROP value (no prefix) so the headless
        // doctest can match them by exact text.
        Text {
            objectName: "statusText"
            color: "#ffffff"
            wrapMode: Text.WordWrap
            width: 760
            text: backend ? backend.status : ""
        }
        Text {
            objectName: "eventText"
            color: "#ffffff"
            text: backend ? backend.lastEvent : ""
        }
    }
}
