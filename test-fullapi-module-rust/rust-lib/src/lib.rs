//! Rust cdylib provider mirroring test_fullapi_cpp's `full_api` surface — every
//! supported method parameter/return type and event parameter type.
//!
//! Contract-first (no build.rs): the module-impl C ABI scaffold, the typed
//! `emit_*` event emitters, and the `TestFullapiRustModule` trait are generated
//! by the builder from `test_fullapi_rust.lidl`. The author writes only the
//! trait impl below. Every echo returns its input unchanged so the C++ and Rust
//! providers answer identically (the cross-language parity check); `who_am_i`
//! is the one intentional difference.

use serde_json::Value;

include!(concat!(env!("CARGO_MANIFEST_DIR"), "/generated/provider_gen.rs"));

#[derive(Default)]
struct FullapiImpl;

impl TestFullapiRustModule for FullapiImpl {
    fn who_am_i(&mut self) -> String {
        "test_fullapi_rust".to_string()
    }

    // ── Scalar echoes ────────────────────────────────────────────────────────
    fn echo_string(&mut self, v: String) -> String { v }
    fn echo_bytes(&mut self, v: Vec<u8>) -> Vec<u8> { v }
    fn echo_int(&mut self, v: i64) -> i64 { v }
    fn echo_uint(&mut self, v: u64) -> u64 { v }
    fn echo_double(&mut self, v: f64) -> f64 { v }
    fn echo_bool(&mut self, v: bool) -> bool { v }
    fn echo_any(&mut self, v: Value) -> Value { v }

    // ── Container echoes (composites arrive as serde_json::Value) ─────────────
    fn echo_string_list(&mut self, v: Value) -> Value { v }
    fn echo_int_list(&mut self, v: Value) -> Value { v }
    fn echo_uint_list(&mut self, v: Value) -> Value { v }
    fn echo_double_list(&mut self, v: Value) -> Value { v }
    fn echo_bool_list(&mut self, v: Value) -> Value { v }
    fn echo_list(&mut self, v: Value) -> Value { v }
    fn echo_map(&mut self, v: Value) -> Value { v }

    // ── Arity ────────────────────────────────────────────────────────────────
    // `i=<decimal>|s=<utf8>|b=<lowercase hex>`, byte for byte the same digest
    // the C++ provider builds — so the two answers are comparable and argument
    // ORDER is pinned by a single comparison.
    fn echo_triple(&mut self, i: i64, s: String, b: Vec<u8>) -> String {
        let hex: String = b.iter().map(|byte| format!("{:02x}", byte)).collect();
        format!("i={}|s={}|b={}", i, s, hex)
    }

    // ── Return-only types ────────────────────────────────────────────────────
    fn do_void(&mut self) -> Value { Value::Null }

    fn make_result(&mut self, ok: bool) -> Result<Value, String> {
        if ok {
            Ok(serde_json::json!({ "provider": "test_fullapi_rust", "ok": true }))
        } else {
            Err("deliberate error for testing".to_string())
        }
    }

    // ── Event trigger drivers ────────────────────────────────────────────────
    fn fire_string_event(&mut self, v: String) -> bool { emit_string_event(&v); true }
    fn fire_bytes_event(&mut self, v: Vec<u8>) -> bool { emit_bytes_event(&v); true }
    fn fire_int_event(&mut self, v: i64) -> bool { emit_int_event(v); true }
    fn fire_uint_event(&mut self, v: u64) -> bool { emit_uint_event(v); true }
    fn fire_double_event(&mut self, v: f64) -> bool { emit_double_event(v); true }
    fn fire_bool_event(&mut self, v: bool) -> bool { emit_bool_event(v); true }
    fn fire_any_event(&mut self, v: Value) -> bool { emit_any_event(&v); true }
    fn fire_string_list_event(&mut self, v: Value) -> bool { emit_string_list_event(&v); true }
    fn fire_int_list_event(&mut self, v: Value) -> bool { emit_int_list_event(&v); true }
    fn fire_uint_list_event(&mut self, v: Value) -> bool { emit_uint_list_event(&v); true }
    fn fire_double_list_event(&mut self, v: Value) -> bool { emit_double_list_event(&v); true }
    fn fire_bool_list_event(&mut self, v: Value) -> bool { emit_bool_list_event(&v); true }
    fn fire_list_event(&mut self, v: Value) -> bool { emit_list_event(&v); true }
    fn fire_map_event(&mut self, v: Value) -> bool { emit_map_event(&v); true }
    fn fire_triple_event(&mut self, i: i64, s: String, b: Vec<u8>) -> bool {
        emit_triple_event(i, &s, &b);
        true
    }
}

#[no_mangle]
pub extern "Rust" fn logos_module_install() {
    install::<FullapiImpl>();
}
