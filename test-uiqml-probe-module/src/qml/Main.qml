import QtQuick

// View for the transport-choice spike.
//
// Three things are kept separate on purpose:
//
//   dispatch  — every slot on the .rep, named exactly once.
//   cases     — pure data: a slot name, a literal, and a group.
//   the pump  — advances on the backend's sequence number, never on a guessed
//               settling delay.
//
// So a case is one line, driving any literal (a JS value or JSON text) into
// either transport, and the label records `typeof arg` — a JS number reaching a
// QString slot, or a JS object reaching a native one, is visible in the output
// instead of inferred.
//
// GROUPS exist because one case can kill the process. `LOGOS_UIQML_CASES` (read
// by the backend, exposed as `caseGroup`) selects which groups run, so a
// crashing case is isolated in its own run and the rest of the table still gets
// measured.
Item {
    id: root
    objectName: "uiqmlProbeRoot"
    width: 900
    height: 620

    // Assigned imperatively rather than bound: `logos.module()` is a plain call
    // with nothing reactive to depend on, so a binding that first evaluated to
    // null would cache that null forever.
    property var backend: null

    // Written as explicit closures rather than `backend[name](arg)` so a typo is
    // a QML error at the call, not a silent no-op.
    readonly property var dispatch: ({
        // native transport
        "doEchoInt":          function (b, a) { b.doEchoInt(a); },
        "doEchoUint":         function (b, a) { b.doEchoUint(a); },
        "doEchoUintList":     function (b, a) { b.doEchoUintList(a); },
        "doEchoMap":          function (b, a) { b.doEchoMap(a); },
        "doEchoAny":          function (b, a) { b.doEchoAny(a); },
        "doEchoBytesNative":  function (b, a) { b.doEchoBytesNative(a); },
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
        "doEchoIntList":      function (b, a) { b.doEchoIntList(a); },
        "doEchoList":         function (b, a) { b.doEchoList(a); }
    })

    // Every byte value 0x00..0xFF, base64url-unpadded, as one bstr. A constant
    // rather than something computed here: `Qt.btoa` encodes a JS string, and a
    // JS string cannot hold 0x80..0xFF as single bytes, so computing it in QML
    // would be measuring the harness. The analysis decodes what the PROVIDER
    // reports with an independent base64 implementation and checks it against
    // 0..255, which is what closes the loop.
    readonly property string allBytesB64:
        "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4" +
        "OTo7PD0-P0BBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWltcXV5fYGFiY2RlZmdoaWprbG1ub3Bx" +
        "cnN0dXZ3eHl6e3x9fn-AgYKDhIWGh4iJiouMjY6PkJGSk5SVlpeYmZqbnJ2en6ChoqOkpaanqKmq" +
        "q6ytrq-wsbKztLW2t7i5uru8vb6_wMHCw8TFxsfIycrLzM3Oz9DR0tPU1dbX2Nna29zd3t_g4eLj" +
        "5OXm5-jp6uvs7e7v8PHy8_T19vf4-fr7_P3-_w"

    // ── the case table ──────────────────────────────────────────────────────
    //
    // Twins sit adjacent so the two transports line up pairwise. 2^64-1 and
    // 2^53+1 appear as bare JS literals on the native side deliberately: that
    // IS how a QML author writes them, and what JS did to the literal before
    // any slot was entered is part of the measurement, not a flaw in the case.
    property var cases: [
        // ── int ─────────────────────────────────────────────────────────────
        { g: "int",  slot: "doEchoInt",          arg: 184 },
        { g: "int",  slot: "doEchoIntJson",      arg: "184" },
        { g: "int",  slot: "doEchoInt",          arg: 3.7 },
        { g: "int",  slot: "doEchoIntJson",      arg: "3.7" },
        { g: "int",  slot: "doEchoInt",          arg: 9007199254740993 },
        { g: "int",  slot: "doEchoIntJson",      arg: "9007199254740993" },
        { g: "int",  slot: "doEchoInt",          arg: -9007199254740993 },
        { g: "int",  slot: "doEchoIntJson",      arg: "-9007199254740993" },

        // ── uint ────────────────────────────────────────────────────────────
        { g: "uint", slot: "doEchoUint",         arg: 184 },
        { g: "uint", slot: "doEchoUintJson",     arg: "184" },
        { g: "uint", slot: "doEchoUint",         arg: 18446744073709551615 },
        { g: "uint", slot: "doEchoUintJson",     arg: "18446744073709551615" },
        // 2^64-1 arrives intact natively, but only because saturating a double
        // that overshoots the top of the range lands on the top of the range.
        // These two are the same magnitude WITHOUT that coincidence, and are
        // the honest measure of the native path's precision.
        { g: "uint", slot: "doEchoUint",         arg: 12345678901234567890 },
        { g: "uint", slot: "doEchoUintJson",     arg: "12345678901234567890" },
        { g: "uint", slot: "doEchoUint",         arg: 9007199254740993 },
        { g: "uint", slot: "doEchoUintJson",     arg: "9007199254740993" },
        { g: "uint", slot: "doEchoUint",         arg: -1 },
        { g: "uint", slot: "doEchoUintJson",     arg: "-1" },

        // ── [uint] ──────────────────────────────────────────────────────────
        { g: "list", slot: "doEchoUintList",     arg: [1, 2, 3] },
        { g: "list", slot: "doEchoUintListJson", arg: "[1,2,3]" },
        { g: "list", slot: "doEchoUintList",     arg: [1, 18446744073709551615, 3] },
        { g: "list", slot: "doEchoUintListJson", arg: "[1,18446744073709551615,3]" },
        { g: "list", slot: "doEchoUintList",     arg: [1, -1, 3] },
        { g: "list", slot: "doEchoUintListJson", arg: "[1,-1,3]" },

        // ── {tstr:any} ──────────────────────────────────────────────────────
        { g: "map",  slot: "doEchoMap",          arg: { "n": 1 } },
        { g: "map",  slot: "doEchoMapJson",      arg: '{"n":1}' },
        { g: "map",  slot: "doEchoMap",          arg: { "n": 18446744073709551615 } },
        { g: "map",  slot: "doEchoMapJson",      arg: '{"n":18446744073709551615}' },
        // Registry entry M3, reached through a map slot: a legitimate one-key
        // map that happens to be named `_bytes` is indistinguishable from
        // tagged bytes.
        { g: "map",  slot: "doEchoMap",          arg: { "_bytes": "aGk" } },
        { g: "map",  slot: "doEchoMapJson",      arg: '{"_bytes":"aGk"}' },
        { g: "map",  slot: "doEchoMap",          arg: 5 },
        { g: "map",  slot: "doEchoMapJson",      arg: "5" },

        // ── any (QString transport only — the native twin is isolated) ──────
        { g: "any",  slot: "doEchoAnyJson",      arg: '"hello"' },
        { g: "any",  slot: "doEchoAnyJson",      arg: "18446744073709551615" },
        { g: "any",  slot: "doEchoAnyJson",      arg: '{"n":18446744073709551615}' },
        { g: "any",  slot: "doEchoAnyJson",      arg: '{"_bytes":"aGk"}' },
        { g: "any",  slot: "doEchoAnyJson",      arg: '[1,18446744073709551615,3]' },

        // ── bstr ────────────────────────────────────────────────────────────
        { g: "bstr", slot: "doEchoBytesJson", arg: '{"_bytes":""}',
          name: "bstr empty" },
        { g: "bstr", slot: "doEchoBytesJson", arg: '{"_bytes":"aGk"}',
          name: "bstr 'hi'" },
        { g: "bstr", slot: "doEchoBytesJson", arg: '{"_bytes":"AAECgP8"}',
          name: "bstr 00 01 02 80 ff" },
        { g: "bstr", slot: "doEchoBytesJson", arg: '{"_bytes":"' + root.allBytesB64 + '"}',
          name: "bstr ALL 256 byte values" },
        // The empty cell of the design table, measured rather than asserted.
        // QML has no byte-array literal, so a string and an array of numbers
        // are the only two things an author could plausibly hand a QByteArray
        // slot.
        { g: "bstr", slot: "doEchoBytesNative", arg: "hi",
          name: "native bstr <- js string 'hi'" },
        { g: "bstr", slot: "doEchoBytesNative", arg: String.fromCharCode(0x80, 0xFF),
          name: "native bstr <- js string U+0080 U+00FF" },
        { g: "bstr", slot: "doEchoBytesNative", arg: [0, 1, 2, 128, 255],
          name: "native bstr <- js array [0,1,2,128,255]" },
        // If the array form turns out to be exact for in-range bytes, then the
        // question the design table's empty cell really turns on is what it
        // does with elements that are NOT bytes. An author who can write
        // [0,1,255] will eventually write one of these.
        { g: "bstr", slot: "doEchoBytesNative", arg: [256, 257, 511],
          name: "native bstr <- js array [256,257,511] (out of byte range)" },
        { g: "bstr", slot: "doEchoBytesNative", arg: [-1, -2],
          name: "native bstr <- js array [-1,-2] (negative)" },
        { g: "bstr", slot: "doEchoBytesNative", arg: [1.5, 2.7],
          name: "native bstr <- js array [1.5,2.7] (fractional)" },
        { g: "bstr", slot: "doEchoBytesNative", arg: ["a", "b"],
          name: "native bstr <- js array of strings" },

        // ── carried over from the original probe ────────────────────────────
        { g: "extra", slot: "doEchoBool",        arg: 1 },
        { g: "extra", slot: "doEchoStringList",  arg: ["a", 1] },
        { g: "extra", slot: "doEchoIntList",     arg: [1, 3.7, 3] },
        { g: "extra", slot: "doEchoList",        arg: "notalist" },

        // ── any, NATIVE ─────────────────────────────────────────────────────
        //
        // This slot kills the process. The scalars share a group because they
        // all survive; every COMPOSITE gets a group to itself, so each one is
        // measured directly instead of being inferred from "the case before it
        // died". A case that never ran is not evidence about that case.
        { g: "anynative",          slot: "doEchoAny", arg: "hello" },
        { g: "anynative",          slot: "doEchoAny", arg: true },
        { g: "anynative",          slot: "doEchoAny", arg: 42 },
        { g: "anynative",          slot: "doEchoAny", arg: 18446744073709551615 },
        { g: "anynative_arr",      slot: "doEchoAny", arg: [1, 2, 3] },
        { g: "anynative_obj",      slot: "doEchoAny", arg: { "n": 1 } },
        { g: "anynative_bignum",   slot: "doEchoAny", arg: { "n": 18446744073709551615 } },
        { g: "anynative_bytes",    slot: "doEchoAny", arg: { "_bytes": "aGk" } }
    ]

    // 256 one-byte bstr cases, so each byte value is exercised alone and not
    // only as a position inside one 256-byte blob. base64url of a single byte
    // is two characters and nothing else — small enough to read and check.
    function singleByteCases() {
        var alpha = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
        var out = [];
        for (var b = 0; b < 256; ++b) {
            var b64 = alpha.charAt(b >> 2) + alpha.charAt((b & 0x03) << 4);
            out.push({ g: "bstr256", slot: "doEchoBytesJson",
                       arg: '{"_bytes":"' + b64 + '"}',
                       name: "byte " + b });
        }
        return out;
    }

    property var active: []

    function label(c) {
        return (c.name ? c.name + " " : "") + c.slot
             + "(" + JSON.stringify(c.arg) + " : js " + (typeof c.arg) + ")";
    }

    // ── the pump ────────────────────────────────────────────────────────────
    //
    // Advances on the backend's sequence number rather than on elapsed time. A
    // case whose invocation threw never reached the backend, so `lastResult`
    // still holds the PREVIOUS answer; without the sequence check the next tick
    // would report that stale answer under this case's name — a fabricated
    // result, and the most dangerous kind of harness bug in a study about
    // values that silently change.
    property int idx: -1
    property int lastSeq: -1
    property bool awaiting: false
    property int stall: 0
    property string table: ""

    function seqOf(s) { var i = s.indexOf("|"); return i < 0 ? -1 : parseInt(s.substring(0, i)); }
    function bodyOf(s) { var i = s.indexOf("|"); return i < 0 ? s : s.substring(i + 1); }

    function record(line) {
        table += line + "\n";
        console.log("UIQML_SPIKE_CASE " + line);
    }

    function step() {
        if (awaiting) {
            var lr = backend.lastResult;
            if (seqOf(lr) > lastSeq) {
                record(label(active[idx]) + " -> " + bodyOf(lr));
                awaiting = false;
                stall = 0;
            } else if (++stall >= 250) {          // 25 s, past the 20 s call timeout
                record(label(active[idx]) + " -> NO ANSWER (25s)");
                awaiting = false;
                stall = 0;
            } else {
                return;
            }
        }

        idx += 1;
        if (idx >= active.length) {
            table += "DONE\n";
            console.log("UIQML_SPIKE_DONE " + active.length + " cases");
            pump.running = false;
            return;
        }

        var c = active[idx];
        console.log("UIQML_SPIKE_INVOKE " + label(c));
        lastSeq = seqOf(backend.lastResult);   // snapshot before the invoke
        try {
            root.dispatch[c.slot](backend, c.arg);
            awaiting = true;
            stall = 0;
        } catch (e) {
            record(label(c) + " -> THREW: " + e);
            awaiting = false;
        }
    }

    function start() {
        var groups = (backend.caseGroup ? backend.caseGroup : "all").split(",");
        var all = cases.concat(singleByteCases());
        active = all.filter(function (c) {
            return groups.indexOf("all") >= 0 || groups.indexOf(c.g) >= 0;
        });
        console.log("UIQML_SPIKE_GROUPS " + groups.join(",")
                    + " -> " + active.length + " cases");
        pump.running = true;
    }

    Timer {
        id: pump
        interval: 100
        repeat: true
        running: false
        onTriggered: root.step()
    }

    // The backend arrives asynchronously; re-query until it does, then start
    // exactly once.
    //
    // The gate is the READY answer, not merely a non-null replica. A replica
    // exists before it has initialised, and reading `caseGroup` too early
    // yields "" — which would silently widen the run to every group, including
    // the one isolated for crashing. Waiting for the backend's own first
    // sequenced answer proves the property values have arrived.
    Timer {
        id: starter
        interval: 100
        repeat: true
        running: true
        onTriggered: {
            if (!root.backend)
                root.backend = logos.module("test_uiqml_probe");
            if (root.backend && root.seqOf(root.backend.lastResult) >= 1) {
                starter.running = false;
                root.start();
            }
        }
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
