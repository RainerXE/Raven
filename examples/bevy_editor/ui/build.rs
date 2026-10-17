fn main() {
    // src/editor.xmlrs -> $OUT_DIR/editor.xmlrs.rs, included by src/lib.rs
    raven_build::compile("src/editor.xmlrs");
}
