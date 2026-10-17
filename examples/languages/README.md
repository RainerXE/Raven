# One program, eight languages

The same small program — a title, a button, a label that counts the clicks in
the language of the system (`strings.csv`, English and French) — written once
per language, each as **one file holding the interface and its code**. These
are exactly the projects that the IDE writes with *Project > New project*.

| Folder | File | Build and run |
|---|---|---|
| `hello_c` | `main.xmlc` | `cmake -S . -B build -DCMAKE_PREFIX_PATH=<Raven package> && cmake --build build` |
| `hello_cpp` | `main.xmlc++` | the same |
| `hello_rust` | `src/main.xmlrs` | `RAVENC=<package>/bin/ravenc RAVEN_LIB_DIR=<package>/lib cargo run` |
| `hello_python` | `main.xmlpy` | `RAVENC=... PYTHONPATH=<bindings>/python python -m raven_xmlpy main.xmlpy` |
| `hello_go` | `main.xmlgo` | `go generate && go run .` (ravenc on the `PATH`, cgo finds the library) |
| `hello_csharp` | `main.xmlcs` | `dotnet run` (ravenc on the `PATH`) |
| `hello_zig` | `main.xmlzig` | `RAVEN_LIB_DIR=<package>/lib zig build run` |
| `hello_lua` | `main.xmllua` | `luajit -e "require('raven_xmllua').run('main.xmllua')"` |

The easiest way: open the folder in the IDE (`raven_designer hello_rust`) and
press F5. The IDE sets `RAVENC`, `RAVEN_LIB_DIR` and the rest from the Raven
package it runs from.

The Rust, Go, C# projects find the bindings in `../../../bindings` (in a
package: `share/raven/bindings`). `hello_zig/raven/` holds copies of
`bindings/zig` because a `build.zig` only imports files of its own project.

In every language, `OnClicked="{on_hello}"` names a function of the scripts
and the compiler of the language checks it; a mistake in a script is
reported at its line in the `main.xml…` file.
