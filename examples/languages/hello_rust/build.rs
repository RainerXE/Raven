// ravenc turns src/main.xmlrs into Rust ($OUT_DIR/main.xmlrs.rs)
fn main() {
    raven_build::compile("src/main.xmlrs");
}
