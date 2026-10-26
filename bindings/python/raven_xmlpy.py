"""Imports .xmlpy files: an interface in Raven XML with its Python code.

    import raven_xmlpy          # once: .xmlpy files can then be imported
    import main                 # main.xmlpy, next to the program or on sys.path
    main.main_create()

    python -m raven_xmlpy main.xmlpy [arguments]    # runs one as the program

ravenc turns the file into Python (the interface becomes a table: no XML is
read when it runs). The result is kept in __pycache__ and made again when the
.xmlpy changes. ravenc is looked for in RAVENC (the program itself), next to
RAVEN_LIBRARY (bin/ of a Raven package), then in the PATH.
"""

import importlib.abc
import importlib.util
import os
import shutil
import subprocess
import sys

__all__ = ["compile_file", "load", "run"]


def _ravenc():
    explicit = os.environ.get("RAVENC")
    if explicit:
        return explicit
    exe = "ravenc.exe" if os.name == "nt" else "ravenc"
    library = os.environ.get("RAVEN_LIBRARY")
    if library:
        folder = os.path.dirname(os.path.abspath(library))
        for candidate in (os.path.join(folder, exe), os.path.join(folder, "..", "bin", exe)):
            if os.path.isfile(candidate):
                return candidate
    # the ravenc shipped in the pip package of Raven (raven/bin/)
    try:
        import raven as _raven_module
        bundled = os.path.join(os.path.dirname(os.path.abspath(_raven_module.__file__)), "bin", exe)
        if os.path.isfile(bundled):
            return bundled
    except (ImportError, OSError):
        pass
    found = shutil.which("ravenc")
    if found:
        return found
    raise RuntimeError("ravenc was not found: set RAVENC to the ravenc program of Raven")


def compile_file(path):
    """The Python file made from an .xmlpy (compiled again when the .xmlpy is newer)."""
    path = os.path.abspath(path)
    folder = os.path.join(os.path.dirname(path), "__pycache__")
    os.makedirs(folder, exist_ok=True)
    output = os.path.join(folder, os.path.basename(path) + ".py")
    if not os.path.exists(output) or os.path.getmtime(output) < os.path.getmtime(path):
        result = subprocess.run([_ravenc(), path, "-o", output, "--lang", "python"], capture_output=True, text=True)
        if result.returncode != 0:
            raise SyntaxError(result.stderr.strip() or "ravenc failed on " + path)
    return output


class _Loader(importlib.abc.SourceLoader):
    def __init__(self, path):
        self.path = path

    def get_filename(self, fullname):
        return compile_file(self.path)

    def get_data(self, filename):
        with open(filename, "rb") as handle:
            return handle.read()


class _Finder(importlib.abc.MetaPathFinder):
    def find_spec(self, fullname, path, target=None):
        name = fullname.rpartition(".")[2]
        for folder in (path or sys.path):
            candidate = os.path.join(folder or ".", name + ".xmlpy")
            if os.path.isfile(candidate):
                return importlib.util.spec_from_loader(fullname, _Loader(candidate), origin=candidate)
        return None


def load(path, name=None):
    """Imports one .xmlpy file by its path and returns the module."""
    name = name or os.path.splitext(os.path.basename(path))[0]
    spec = importlib.util.spec_from_loader(name, _Loader(os.path.abspath(path)), origin=path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def run(path, argv=None):
    """Runs an .xmlpy as the program (__name__ == "__main__")."""
    sys.argv = [path] + list(argv or [])
    sys.path.insert(0, os.path.dirname(os.path.abspath(path)))
    return load(path, "__main__")


if not any(isinstance(finder, _Finder) for finder in sys.meta_path):
    sys.meta_path.append(_Finder())

if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.stderr.write("usage: python -m raven_xmlpy file.xmlpy [arguments]\n")
        sys.exit(2)
    run(sys.argv[1], sys.argv[2:])
