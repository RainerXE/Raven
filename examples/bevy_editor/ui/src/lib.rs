//! The interface of the Raven Bevy Editor.
//!
//! The window, its panels and everything they do are in `src/editor.xmlrs`: Raven XML with
//! the Rust that goes with it, turned into Rust by ravenc when the crate builds. What draws
//! the scene is given by the program: Bevy (`raven-bevy-editor`) or [`SoftwareRenderer`].

pub mod render;

/// The editor (generated from src/editor.xmlrs).
#[allow(clippy::all)]
pub mod editor {
    include!(concat!(env!("OUT_DIR"), "/editor.xmlrs.rs"));
}

pub use editor::{run, run_loop, start, stop, with, Editor};
pub use render::{Frame, RenderInput, SceneRenderer, SoftwareRenderer};
