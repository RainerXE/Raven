#!/bin/sh
# ---------------------------------------------------------------------------
#  Raven - dist.sh
#  Builds what is given to people who use Raven without building it: the
#  static and the shared library (Release), the headers, the designer, the
#  bindings and the documentation, in one folder and one archive.
#
#    ./dist.sh [--no-test] [-- extra cmake arguments]
#
#    dist/raven-<version>-linux-<arch>/
#        include/                 Raven.h, raven/*.h
#        lib/                     libraven.so, cmake/raven (find_package)
#        lib/static/              libraven.a, cmake/raven
#        bin/raven_designer
#        share/raven/             README.md, docs/, bindings/
#    dist/raven-<version>-linux-<arch>.tar.gz
#
#  A program finds the shared library with
#      cmake -DCMAKE_PREFIX_PATH=<folder> ...        find_package(raven)
#  and the static one with -Draven_DIR=<folder>/lib/static/cmake/raven.
# ---------------------------------------------------------------------------
set -e

ROOT=$(cd "$(dirname "$0")" && pwd)
TEST=1
while [ $# -gt 0 ]; do
	case "$1" in
		--no-test) TEST=0; shift ;;
		--) shift; break ;;
		*) echo "dist.sh: unknown option $1" >&2; exit 2 ;;
	esac
done

VERSION=$(sed -n 's/^project(raven VERSION \([0-9.]*\).*/\1/p' "$ROOT/CMakeLists.txt")
NAME="raven-$VERSION-linux-$(uname -m)"
OUT="$ROOT/dist/$NAME"
JOBS=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)

rm -rf "$OUT" "$ROOT/dist/$NAME.tar.gz"

# the static library last: the designer that stays in bin/ is the one that needs nothing beside it
for KIND in shared static; do
	BUILD="$ROOT/build/dist-$KIND"
	if [ "$KIND" = shared ]; then SHARED=ON; LIBDIR=lib; else SHARED=OFF; LIBDIR=lib/static; fi
	echo "== $KIND library"
	cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release -DRAVEN_BUILD_SHARED=$SHARED -DRAVEN_BUILD_EXAMPLES=OFF \
		-DCMAKE_INSTALL_PREFIX="$OUT" -DCMAKE_INSTALL_LIBDIR=$LIBDIR "$@"
	cmake --build "$BUILD" -j "$JOBS"
	if [ $TEST = 1 ]; then (cd "$BUILD" && ctest --output-on-failure); fi
	cmake --install "$BUILD"
done

(cd "$ROOT/dist" && tar czf "$NAME.tar.gz" "$NAME")
echo
echo "dist/$NAME.tar.gz"
