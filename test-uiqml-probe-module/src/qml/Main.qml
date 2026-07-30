import QtQuick

// View for the ui_qml probe. Drives the NINE hostile-argument cases through the
// backend's QtRO slots — the only inbound surface a `type: ui_qml` +
// `interface: universal` module has — and accumulates each answer into one Text
// node so a headless run can read the whole table off the view.
Item {
    id: root
    objectName: "uiqmlProbeRoot"
    width: 900
    height: 520

    readonly property var backend: logos.module("test_uiqml_probe")

    property var cases: [
        { name: "echoUint(-1)",            fn: function () { backend.doEchoUint(-1); } },
        { name: "echoInt(3.7)",            fn: function () { backend.doEchoInt(3.7); } },
        { name: "echoBool(1)",             fn: function () { backend.doEchoBool(1); } },
        { name: "echoStringList([a,1])",   fn: function () { backend.doEchoStringList(["a", 1]); } },
        { name: "echoUintList([1,-1,3])",  fn: function () { backend.doEchoUintList([1, -1, 3]); } },
        { name: "echoUintList([1,2.5,3])", fn: function () { backend.doEchoUintList([1, 2.5, 3]); } },
        { name: "echoIntList([1,3.7,3])",  fn: function () { backend.doEchoIntList([1, 3.7, 3]); } },
        { name: "echoList(notalist)",      fn: function () { backend.doEchoList("notalist"); } },
        { name: "echoMap(5)",              fn: function () { backend.doEchoMap(5); } }
    ]

    property int idx: -1
    property string table: ""

    function step() {
        if (idx >= 0 && idx < cases.length) {
            var line = cases[idx].name + " -> " + (backend ? backend.lastResult : "<no backend>");
            table += line + "\n";
            console.log("UIQML_PROBE_CASE " + line);
        }
        idx += 1;
        if (idx >= cases.length) {
            table += "DONE\n";
            console.log("UIQML_PROBE_DONE");
            pump.running = false;
            return;
        }
        try {
            cases[idx].fn();
        } catch (e) {
            table += cases[idx].name + " -> THREW: " + e + "\n";
            console.log("UIQML_PROBE_CASE " + cases[idx].name + " -> THREW: " + e);
        }
    }

    Timer {
        id: pump
        interval: 700
        repeat: true
        running: root.backend !== null && root.backend !== undefined
        onTriggered: root.step()
    }

    Column {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        Text {
            objectName: "probeStatus"
            text: root.backend ? root.backend.lastResult : "NO_BACKEND"
            color: "#ffffff"
        }
        Text {
            objectName: "probeTable"
            text: root.table
            color: "#ffffff"
        }
    }

    Rectangle {
        anchors.fill: parent
        z: -1
        color: "#1a1a22"
    }
}
