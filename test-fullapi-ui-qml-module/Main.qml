import QtQuick

// QML-ONLY full_api UI plugin — NO C++ backend. It consumes test_fullapi_cpp
// entirely from QML/JS through the `logos` bridge injected by the view-module
// runtime:
//   * logos.callModule(module, method, [args]) -> JSON string (sync)
//   * logos.onModuleEvent(module, event)        -> subscribe
//   * Connections { onModuleEventReceived(m, e, data) } -> typed event payloads
//
// "Run All Methods" drives every method type and publishes a single
// `ALL_OK_QML:<target>` token (or `FAIL:<method> ...`); "Fire Events"
// subscribes to + triggers every typed event and publishes
// `ALL_EVENTS_OK_QML:<target>`. A headless UI doctest clicks the buttons and
// asserts on the two exact-text labels.
//
// bstr is intentionally excluded: QML/JS has no native byte literal, so
// echoBytes / bytesEvent aren't expressible here (the C++-backend variant
// test_fullapi_ui covers them).
Item {
    id: root
    objectName: "fullApiUiQmlRoot"
    width: 800
    height: 480

    readonly property string target: "test_fullapi_cpp"
    property string status: ""
    property string eventStatus: ""

    // event-name -> payload received via onModuleEventReceived
    property var received: ({})

    // ── Method matrix (method, args, expected) — mirrors FULLAPI_METHOD_CASES,
    //    minus bstr and container floats kept dyadic for exact compare ──
    readonly property var methodCases: [
        ["whoAmI",        [],                       "test_fullapi_cpp"],
        ["echoString",    ["round-trip"],           "round-trip"],
        ["echoInt",       [42],                     42],
        ["echoInt",       [-7],                     -7],
        ["echoUint",      [7],                      7],
        ["echoDouble",    [2.5],                    2.5],
        ["echoBool",      [true],                   true],
        ["echoBool",      [false],                  false],
        ["echoAny",       ["hi"],                   "hi"],
        ["echoAny",       [42],                     42],
        ["echoAny",       [{ "k": "v" }],           { "k": "v" }],
        ["echoStringList",[["a", "b", "c"]],        ["a", "b", "c"]],
        ["echoIntList",   [[1, 2, 3]],              [1, 2, 3]],
        ["echoUintList",  [[4, 5, 6]],              [4, 5, 6]],
        ["echoDoubleList",[[1.5, 2.5]],             [1.5, 2.5]],
        ["echoBoolList",  [[true, false, true]],    [true, false, true]],
        ["echoList",      [[1, "two", { "k": 1 }]], [1, "two", { "k": 1 }]],
        ["echoMap",       [{ "s": "v", "n": 42, "b": true }], { "s": "v", "n": 42, "b": true }],
        ["makeResult",    [true],                   { "success": true, "value": { "ok": true, "provider": "test_fullapi_cpp" }, "error": null }]
    ]

    // ── Event matrix (event, fireMethod, fireArg, expectedPayload) ──
    readonly property var eventCases: [
        ["stringEvent",     "fireStringEvent",     "hello",              "hello"],
        ["intEvent",        "fireIntEvent",        123,                  123],
        ["uintEvent",       "fireUintEvent",       7,                    7],
        ["doubleEvent",     "fireDoubleEvent",     2.5,                  2.5],
        ["boolEvent",       "fireBoolEvent",       true,                 true],
        ["anyEvent",        "fireAnyEvent",        { "k": "v" },         { "k": "v" }],
        ["stringListEvent", "fireStringListEvent", ["p", "q"],           ["p", "q"]],
        ["intListEvent",    "fireIntListEvent",    [1, 2, 3],            [1, 2, 3]],
        ["uintListEvent",   "fireUintListEvent",   [4, 5],               [4, 5]],
        ["doubleListEvent", "fireDoubleListEvent", [1.5, 2.5],           [1.5, 2.5]],
        ["boolListEvent",   "fireBoolListEvent",   [true, false],        [true, false]],
        ["listEvent",       "fireListEvent",       [1, "two"],           [1, "two"]],
        ["mapEvent",        "fireMapEvent",        { "mk": "mv" },       { "mk": "mv" }]
    ]

    // ── order-independent deep compare ──
    function deepEqual(a, b) {
        if (a === b) return true;
        if (typeof a !== typeof b) return false;
        if (a === null || b === null) return a === b;
        if (Array.isArray(a) || Array.isArray(b)) {
            if (!Array.isArray(a) || !Array.isArray(b) || a.length !== b.length) return false;
            for (var i = 0; i < a.length; i++) if (!deepEqual(a[i], b[i])) return false;
            return true;
        }
        if (typeof a === "object") {
            var ka = Object.keys(a), kb = Object.keys(b);
            if (ka.length !== kb.length) return false;
            for (var j = 0; j < ka.length; j++) {
                if (!b.hasOwnProperty(ka[j])) return false;
                if (!deepEqual(a[ka[j]], b[ka[j]])) return false;
            }
            return true;
        }
        return false;
    }

    function runAllMethods() {
        root.status = "";
        if (typeof logos === "undefined" || !logos.callModule) {
            root.status = "FAIL: logos bridge unavailable";
            return;
        }
        for (var i = 0; i < methodCases.length; i++) {
            var name = methodCases[i][0], args = methodCases[i][1], expected = methodCases[i][2];
            var raw = logos.callModule(root.target, name, args);
            var got;
            try {
                got = JSON.parse(raw);
            } catch (e) {
                root.status = "FAIL:" + name + " unparseable=" + raw;
                return;
            }
            if (!deepEqual(got, expected)) {
                root.status = "FAIL:" + name + " got=" + raw;
                return;
            }
        }
        root.status = "ALL_OK_QML:" + root.target;
    }

    function subscribeEvents() {
        if (typeof logos === "undefined" || !logos.onModuleEvent) return;
        for (var i = 0; i < eventCases.length; i++)
            logos.onModuleEvent(root.target, eventCases[i][0]);
    }

    function fireAllEvents() {
        root.eventStatus = "";
        root.received = ({});
        if (typeof logos === "undefined" || !logos.callModule) {
            root.eventStatus = "FAIL: logos bridge unavailable";
            return;
        }
        for (var i = 0; i < eventCases.length; i++)
            logos.callModule(root.target, eventCases[i][1], [eventCases[i][2]]);
    }

    function checkEventsComplete() {
        for (var i = 0; i < eventCases.length; i++) {
            var ev = eventCases[i][0], expected = eventCases[i][3];
            if (!root.received.hasOwnProperty(ev)) return; // not all in yet
            if (!deepEqual(root.received[ev], expected)) {
                root.eventStatus = "FAIL:" + ev + " got=" + JSON.stringify(root.received[ev]);
                return;
            }
        }
        root.eventStatus = "ALL_EVENTS_OK_QML:" + root.target;
    }

    Component.onCompleted: subscribeEvents()

    Connections {
        target: typeof logos !== "undefined" ? logos : null
        function onModuleEventReceived(moduleName, eventName, data) {
            if (moduleName !== root.target) return;
            var m = root.received;
            m[eventName] = (data && data.length > 0) ? data[0] : null;
            root.received = m;
            root.checkEventsComplete();
        }
    }

    component Btn: Rectangle {
        property string label: ""
        signal clicked()
        width: 220
        height: 40
        radius: 6
        color: tap.pressed ? "#3a6ea5" : "#2d2d3a"
        border.color: "#5a5a6a"
        Text { anchors.centerIn: parent; text: parent.label; color: "#ffffff" }
        MouseArea { id: tap; anchors.fill: parent; onClicked: parent.clicked() }
    }

    Column {
        anchors.centerIn: parent
        spacing: 10

        Text {
            text: "Full API — QML-only (no backend), target: " + root.target
            color: "#8b949e"
        }
        Row {
            spacing: 10
            Btn { label: "Run All Methods"; onClicked: root.runAllMethods() }
            Btn { label: "Fire Events"; onClicked: root.fireAllEvents() }
        }
        // Exact-text labels the headless doctest matches on.
        Text {
            objectName: "statusText"
            color: "#ffffff"
            wrapMode: Text.WordWrap
            width: 760
            text: root.status
        }
        Text {
            objectName: "eventStatusText"
            color: "#7ab8ff"
            wrapMode: Text.WordWrap
            width: 760
            text: root.eventStatus
        }
    }
}
