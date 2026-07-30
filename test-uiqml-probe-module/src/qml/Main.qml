import QtQuick

// View for the transport-choice spike.
//
// Structure matters here: the SLOT TABLE (`dispatch`) and the CASE TABLE
// (`cases`) are separate. `dispatch` names every slot exactly once, so a case is
// pure data — a slot name and a literal — and phase 2 can drive any combination
// (JS literal or string literal, into either transport) by editing `cases`
// alone. The label records `typeof arg`, so a JS number reaching a QString slot
// (or vice versa) is visible in the output rather than inferred.
Item {
    id: root
    objectName: "uiqmlProbeRoot"
    width: 900
    height: 620

    readonly property var backend: logos.module("test_uiqml_probe")

    // Every slot on the .rep, invoked by name. Written as explicit closures
    // rather than `backend[name](arg)` so a typo is a QML error at the call,
    // not a silent no-op.
    readonly property var dispatch: ({
        // native transport
        "doEchoInt":          function (b, a) { b.doEchoInt(a); },
        "doEchoUint":         function (b, a) { b.doEchoUint(a); },
        "doEchoUintList":     function (b, a) { b.doEchoUintList(a); },
        "doEchoMap":          function (b, a) { b.doEchoMap(a); },
        "doEchoAny":          function (b, a) { b.doEchoAny(a); },
        // QString (canonical JSON text) transport
        "doEchoIntJson":      function (b, a) { b.doEchoIntJson(a); },
        "doEchoUintJson":     function (b, a) { b.doEchoUintJson(a); },
        "doEchoBytesJson":    function (b, a) { b.doEchoBytesJson(a); },
        "doEchoUintListJson": function (b, a) { b.doEchoUintListJson(a); },
        "doEchoMapJson":      function (b, a) { b.doEchoMapJson(a); },
        "doEchoAnyJson":      function (b, a) { b.doEchoAnyJson(a); },
        // carried over from the original probe (native only)
        "doEchoBool":         function (b, a) { b.doEchoBool(a); },
        "doEchoStringList":   function (b, a) { b.doEchoStringList(a); },
        "doEchoIntList":      function (b, a) { b.doEchoIntList(a); }
    })

    // Seed set: each covered type driven through BOTH transports with the same
    // hostile value, so the twins line up pairwise in the output. Phase 2
    // extends this table; nothing else needs to change.
    property var cases: [
        { slot: "doEchoInt",          arg: 3.7 },
        { slot: "doEchoIntJson",      arg: "3.7" },
        { slot: "doEchoInt",          arg: 9007199254740993 },
        { slot: "doEchoIntJson",      arg: "9007199254740993" },

        { slot: "doEchoUint",         arg: -1 },
        { slot: "doEchoUintJson",     arg: "-1" },
        { slot: "doEchoUint",         arg: 18446744073709551615 },
        { slot: "doEchoUintJson",     arg: "18446744073709551615" },

        { slot: "doEchoBytesJson",    arg: '{"_bytes":"AAECgP8"}' },

        { slot: "doEchoUintList",     arg: [1, -1, 3] },
        { slot: "doEchoUintListJson", arg: "[1,-1,3]" },
        { slot: "doEchoUintList",     arg: [1, 2, 3] },
        { slot: "doEchoUintListJson", arg: "[1,2,3]" },

        { slot: "doEchoMap",          arg: 5 },
        { slot: "doEchoMapJson",      arg: "5" },
        { slot: "doEchoMap",          arg: { "a": 1 } },
        { slot: "doEchoMapJson",      arg: '{"a":1}' },

        // QString twin FIRST: the native `any` slot crashed the host in the
        // first run, taking every case after it with it. Ordering the exact
        // transport ahead of the lossy one keeps its answer on the record
        // regardless of what the native one does.
        { slot: "doEchoAnyJson",      arg: '{"a":1}' },
        { slot: "doEchoAny",          arg: { "a": 1 } }
    ]

    function label(c) {
        return c.slot + "(" + JSON.stringify(c.arg) + " : js " + (typeof c.arg) + ")";
    }

    property int idx: -1
    property string table: ""
    // A case whose invocation threw never reached the backend, so lastResult
    // still holds the PREVIOUS case's answer. Without this flag the next tick
    // reports that stale answer under this case's name — a fabricated result,
    // and the most dangerous kind of harness bug in a study about values that
    // silently change.
    property bool pending: false

    function record(line) {
        table += line + "\n";
        console.log("UIQML_SPIKE_CASE " + line);
    }

    function step() {
        if (pending && idx >= 0 && idx < cases.length)
            record(label(cases[idx]) + " -> " + (backend ? backend.lastResult : "<no backend>"));
        pending = false;

        idx += 1;
        if (idx >= cases.length) {
            table += "DONE\n";
            console.log("UIQML_SPIKE_DONE");
            pump.running = false;
            return;
        }
        var c = cases[idx];
        console.log("UIQML_SPIKE_INVOKE " + label(c));
        try {
            root.dispatch[c.slot](backend, c.arg);
            pending = true;
        } catch (e) {
            record(label(c) + " -> THREW: " + e);
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
