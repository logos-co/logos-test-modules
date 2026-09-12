//! The teardown contract, made observable — the RUST-FIRST twin of
//! test_unload_module_cpp.
//!
//! Same three cases, same evidence, same runner. See that module's impl header
//! for why the evidence is a journal FILE and not stderr (the container closes
//! the child's streams before it signals) and why the mode is chosen at RUNTIME
//! from LOGOS_UNLOAD_MODE rather than at build time.
//!
//! What is different, and why this fixture exists at all: a rust-first module's
//! trait is the author's own, so the generated scaffold cannot call a hook that
//! may not be declared on it. Such a module opts in by implementing
//! `logos_rust_sdk::AboutToUnload` and installing with `logos_install!` instead
//! of `install::<T>()` — the two lines at the bottom of this file are the whole
//! contract, and this is the only place they are exercised through a real host.

use std::env;
use std::fs::OpenOptions;
use std::io::Write;
use std::thread;
use std::time::Duration;

use logos_rust_sdk::{unload_finished, Shutdown};

/// Long enough that an unwaiting host would have killed us first, short enough
/// to stay inside the grace period — so `async` finishing is evidence, not luck.
const ASYNC_WORK_MS: u64 = 1500;

/// Longer than any grace period the host might reasonably use: this mode never
/// finishes, so the daemon exiting at all proves the deadline is enforced.
const HANG_MS: u64 = 60000;

pub trait TestUnloadModuleRustModule: Send + 'static {
    /// The teardown mode this module will use, as read from LOGOS_UNLOAD_MODE.
    /// Callable so a test can confirm the module agrees with the environment
    /// before the interesting part happens — an empty journal is otherwise
    /// ambiguous between "the hook never fired" and "the mode was never set".
    fn unload_mode(&mut self) -> String;

    /// Absolute path of the journal, or "" when LOGOS_UNLOAD_JOURNAL is unset.
    fn unload_journal_path(&mut self) -> String;

    fn on_context_ready(&mut self, _ctx: &RustModuleContext) {}
}

include!(concat!(env!("CARGO_MANIFEST_DIR"), "/generated/provider_gen.rs"));

fn env_or(name: &str, fallback: &str) -> String {
    match env::var(name) {
        Ok(v) if !v.is_empty() => v,
        _ => fallback.to_string(),
    }
}

// Opened and closed per write, so a hard kill mid-teardown still leaves
// everything written up to that instant on disk — which is the evidence the
// `hang` case depends on.
fn journal(what: &str) {
    let path = env_or("LOGOS_UNLOAD_JOURNAL", "");
    if path.is_empty() {
        return;
    }
    if let Ok(mut f) = OpenOptions::new().create(true).append(true).open(&path) {
        let _ = writeln!(f, "{what}");
    }
}

#[derive(Default)]
struct UnloadImpl;

impl TestUnloadModuleRustModule for UnloadImpl {
    fn unload_mode(&mut self) -> String {
        env_or("LOGOS_UNLOAD_MODE", "sync")
    }

    fn unload_journal_path(&mut self) -> String {
        env_or("LOGOS_UNLOAD_JOURNAL", "")
    }
}

impl logos_rust_sdk::AboutToUnload for UnloadImpl {
    fn about_to_unload(&self) -> Shutdown {
        journal("ENTERED");
        let mode = env_or("LOGOS_UNLOAD_MODE", "sync");
        if mode == "sync" {
            return Shutdown::Synchronous;
        }

        // Detached on purpose: about_to_unload must RETURN so the host can start
        // waiting. Working inline here would make Asynchronous a slower spelling
        // of Synchronous.
        let work = if mode == "hang" { HANG_MS } else { ASYNC_WORK_MS };
        thread::spawn(move || {
            thread::sleep(Duration::from_millis(work));
            journal("FINISHED");
            unload_finished();
        });
        Shutdown::Asynchronous
    }
}

#[no_mangle]
pub extern "Rust" fn logos_module_install() {
    logos_install!(UnloadImpl);
}
