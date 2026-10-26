# raven-gui-build

Compiles `.xmlrs` files (an interface in [Raven](https://github.com/oscar-soirey/Raven)
XML with its Rust code) with `ravenc`, from `build.rs`.

```toml
[dependencies]
raven-gui = "0.1"

[build-dependencies]
raven-gui-build = "0.1"
```

```rust
// build.rs
fn main() {
    raven_build::compile("src/main.xmlrs");
}
```

```rust
// src/main.rs
include!(concat!(env!("OUT_DIR"), "/main.xmlrs.rs"));
```

`ravenc` is looked for in `RAVENC`, then in the `bin/` folder of the Raven
package next to `RAVEN_LIB_DIR`, then in the `PATH`. License: Apache 2.0.
