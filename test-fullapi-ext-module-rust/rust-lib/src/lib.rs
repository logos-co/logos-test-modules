//! Rust cdylib provider for the `full_api_ext` surface — the composite tail of
//! the conformance matrix that `full_api` cannot carry.
//!
//! `full_api` is implemented by BOTH providers, and the C++ one is header-first:
//! its contract is derived from an impl header, and the cdylib backend's
//! `typeSupported()` gate rejects records and `[bstr]` by name. Adding those
//! types to `full_api` would not add a test, it would break `test_fullapi_cpp`'s
//! build. So they live here, in a separate contract, Rust-first — and a C++ ext
//! provider joins once the generator can express them.
//!
//! Every method is an echo, so the matrix compares a value against itself: any
//! difference is the wire, the codegen or the dispatch, never this file.
//!
//! What is typed and what is not is itself part of what the matrix records.
//! Records reach the author as real structs (`Blob`, `Wrapper`) — including a
//! record nested in a record, and a `bstr` field which must ride the canonical
//! `{"_bytes": ...}` tag at every depth. Other composites are still
//! `serde_json::Value`: an echo of a Value preserves whatever arrived, so those
//! cells report what the wire actually did rather than what a typed decode
//! would paper over.

use serde_json::Value;

include!(concat!(env!("CARGO_MANIFEST_DIR"), "/generated/provider_gen.rs"));

#[derive(Default)]
struct ExtImpl;

impl TestFullapiExtRustModule for ExtImpl {
    fn who_am_i(&mut self) -> String {
        "test_fullapi_ext_rust".to_string()
    }

    // ── Records ──────────────────────────────────────────────────────────────
    // Typed all the way down: `Wrapper` holds a `Blob`, and `Blob` holds a
    // `bstr`. Returning the struct re-encodes it through the generated
    // `to_json`, so a byte field that survives here survived BOTH directions.
    fn echo_blob(&mut self, v: Blob) -> Blob { v }
    fn echo_wrapper(&mut self, v: Wrapper) -> Wrapper { v }
    fn echo_blob_list(&mut self, v: Vec<Blob>) -> Vec<Blob> { v }

    // ── Composites the codegen does not type yet ─────────────────────────────
    // Echoed as raw JSON on purpose: whatever the wire delivered is what goes
    // back, so the matrix sees the wire's behaviour and not a decode that
    // silently repairs it. (`{tstr: Blob}` is here rather than above because
    // consumer/provider codegen types `[Record]` but not `{tstr: Record}`.)
    fn echo_blob_map(&mut self, v: Value) -> Value { v }
    fn echo_bytes_list(&mut self, v: Value) -> Value { v }
    fn echo_bytes_map(&mut self, v: Value) -> Value { v }
    fn echo_int_map(&mut self, v: Value) -> Value { v }
    fn echo_string_map(&mut self, v: Value) -> Value { v }
    fn echo_nested_ints(&mut self, v: Value) -> Value { v }
    fn echo_map_of_bytes_lists(&mut self, v: Value) -> Value { v }

    // ── Optionality ──────────────────────────────────────────────────────────
    // `?T` is two-state: a value of T, or empty. In Rust that is `Option<T>` and
    // nothing else — one LIDL type, one type per language — so a `?tstr` slot is
    // `Option<String>` here and an optional record field is `Option<...>` inside
    // the generated `Opt`.
    //
    // AUTHORED AT THE TARGET, before any generator read optionality. `lidl-gen`'s
    // `rust_param_type` had no `TypeKind::Optional` arm, so `?tstr` fell to the
    // catch-all and the emitted trait said `fn echo_optional(&mut self, v:
    // serde_json::Value) -> serde_json::Value` — against which this signature
    // failed to compile with `expected serde_json::Value, found Option<String>`.
    // That error WAS the missing feature, stated once, at the place that declares
    // the mapping. The arm landed and this crate builds.
    // (`echo_opt`/`echo_opt_list` are pure echoes of a generated struct, so they
    // compiled under both shapes; only the bare `?tstr` slot names the type,
    // which is why it is the one that had to break.)
    fn echo_opt(&mut self, v: Opt) -> Opt { v }
    fn echo_opt_list(&mut self, v: Vec<Opt>) -> Vec<Opt> { v }
    // `-> result`, not `-> ?tstr`: see the contract. Success with a null value
    // is "the argument was empty"; a failure would be success = false. Echoing
    // through Value keeps the two-state argument observable on the wire.
    fn echo_optional(&mut self, v: Option<String>) -> Result<Value, String> {
        Ok(v.map(Value::from).unwrap_or(Value::Null))
    }

    // ── Record in an event payload ───────────────────────────────────────────
    // The emitter is TYPED for a record parameter, so the Blob goes in as a
    // Blob. It is still encoded through the record's own to_json — the same
    // encoder the return path uses, which is what keeps the bstr field tagged —
    // only now the generator calls it instead of the author, so the payload
    // cannot drift from the one a record RETURN produces.
    fn fire_blob_event(&mut self, v: Blob) -> bool {
        emit_blob_event(&v);
        true
    }
}

#[no_mangle]
pub extern "Rust" fn logos_module_install() {
    install::<ExtImpl>();
}
