//! Compiles `.xmlrs` files (an interface in Raven XML with its Rust code) from `build.rs`.
//!
//! ```toml
//! [dependencies]
//! raven = { path = ".../bindings/rust" }
//! [build-dependencies]
//! raven-build = { path = ".../bindings/rust-build" }
//! ```
//!
//! ```no_run
//! // build.rs
//! fn main() {
//!     raven_build::compile("src/main.xmlrs");
//! }
//! ```
//!
//! ```ignore
//! // src/main.rs: the file is the crate (its <script> blocks hold `fn main`)
//! include!(concat!(env!("OUT_DIR"), "/main.xmlrs.rs"));
//! ```
//!
//! `ravenc` is looked for in `RAVENC` (the program itself), then in the `bin/`
//! folder next to `RAVEN_LIB_DIR` (a Raven package), then in the `PATH`.
//! The scripts keep their lines in the generated file: what rustc reports is
//! at the line of the `.xmlrs`.

use std::env;
use std::path::{Path, PathBuf};
use std::process::Command;

/// Where ravenc is.
pub fn ravenc() -> PathBuf {
    println!("cargo:rerun-if-env-changed=RAVENC");
    println!("cargo:rerun-if-env-changed=RAVEN_LIB_DIR");
    let exe = if cfg!(windows) { "ravenc.exe" } else { "ravenc" };
    if let Ok(path) = env::var("RAVENC") {
        return PathBuf::from(path);
    }
    if let Ok(dir) = env::var("RAVEN_LIB_DIR") {
        for candidate in [Path::new(&dir).join("../bin").join(exe), Path::new(&dir).join(exe)] {
            if candidate.exists() {
                return candidate;
            }
        }
    }
    PathBuf::from(exe)
}

/// Compiles one `.xmlrs` (relative to the crate) into `$OUT_DIR/<file name>.rs`, and returns that path.
pub fn compile(path: &str) -> PathBuf {
    let manifest = env::var("CARGO_MANIFEST_DIR").expect("raven_build::compile runs from build.rs");
    let out_dir = env::var("OUT_DIR").expect("raven_build::compile runs from build.rs");
    let source = Path::new(&manifest).join(path);
    let name = source.file_name().expect("a file").to_string_lossy().into_owned();
    let output = Path::new(&out_dir).join(format!("{}.rs", name));
    println!("cargo:rerun-if-changed={}", source.display());
    let ravenc = ravenc();
    let status = Command::new(&ravenc)
        .arg(&source)
        .arg("-o")
        .arg(&output)
        .arg("--lang")
        .arg("rust")
        .status()
        .unwrap_or_else(|error| panic!("cannot start {} ({}): set RAVENC to the ravenc program", ravenc.display(), error));
    if !status.success() {
        panic!("ravenc failed on {}", source.display());
    }
    output
}

/// Compiles every `.xmlrs` of a folder of the crate (not its subfolders).
pub fn compile_all(folder: &str) -> Vec<PathBuf> {
    let manifest = env::var("CARGO_MANIFEST_DIR").expect("raven_build::compile_all runs from build.rs");
    let mut outputs = Vec::new();
    if let Ok(entries) = std::fs::read_dir(Path::new(&manifest).join(folder)) {
        for entry in entries.flatten() {
            let path = entry.path();
            if path.extension().map(|e| e == "xmlrs").unwrap_or(false) {
                outputs.push(compile(&format!("{}/{}", folder, path.file_name().unwrap().to_string_lossy())));
            }
        }
    }
    println!("cargo:rerun-if-changed={}", Path::new(&manifest).join(folder).display());
    outputs
}
