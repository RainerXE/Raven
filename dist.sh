#!/bin/sh
# ---------------------------------------------------------------------------
#  Raven - dist.sh
#  Builds what is given to people who use Raven: the static and the shared
#  library (Release), the headers, the designer, the bindings and the
#  documentation, in one folder and one archive, for the free edition and
#  for the paid one.
#
#    ./dist.sh [free | paid | both] [--no-test] [-- extra cmake arguments]
#
#    free      the free edition only (every window shows a watermark)
#    paid      the paid edition only (no watermark)
#    both      the two editions, one package each (default)
#
#    dist/raven-<version>-linux-<arch>/          the free edition
#        include/                 Raven.h, raven/*.h, raven_web.h
#        lib/                     libraven.so (web pages included), cmake/raven
#        lib/static/              libraven.a, cmake/raven
#        bin/raven_designer, bin/ravenc
#        share/raven/             README.md, docs/, bindings/
#    dist/raven-<version>-linux-<arch>.tar.gz
#    dist/raven-<version>-linux-<arch>-paid/     the paid edition, the same files
#    dist/raven-<version>-linux-<arch>-paid.tar.gz
# ---------------------------------------------------------------------------
set -e

ROOT=$(cd "$(dirname "$0")" && pwd)
TEST=1
EDITIONS=both
while [ $# -gt 0 ]; do
	case "$1" in
		free|paid|both) EDITIONS="$1"; shift ;;
		--no-test) TEST=0; shift ;;
		--) shift; break ;;
		*) echo "dist.sh: unknown option $1" >&2; exit 2 ;;
	esac
done

VERSION=$(sed -n 's/^project(raven VERSION \([0-9.]*\).*/\1/p' "$ROOT/CMakeLists.txt")
BASE="raven-$VERSION-linux-$(uname -m)"
JOBS=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)

# package <ON|OFF> <name suffix> [extra cmake arguments]
package() {
	PAID=$1
	NAME="$BASE$2"
	shift 2
	OUT="$ROOT/dist/$NAME"
	if [ "$PAID" = ON ]; then EDITION=paid; else EDITION=free; fi
	rm -rf "$OUT" "$ROOT/dist/$NAME.tar.gz"

	# the static library last: the designer that stays in bin/ is the one that needs nothing beside it
	for KIND in shared static; do
		if [ "$KIND" = shared ]; then SHARED=ON; LIBDIR=lib; else SHARED=OFF; LIBDIR=lib/static; fi
		echo "== $KIND library, $EDITION edition"
		BUILD="$ROOT/build/dist-$KIND-$EDITION"
		cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release -DRAVEN_BUILD_SHARED=$SHARED -DRAVEN_PAID=$PAID \
			-DRAVEN_BUILD_EXAMPLES=OFF -DCMAKE_INSTALL_PREFIX="$OUT" -DCMAKE_INSTALL_LIBDIR=$LIBDIR "$@"
		cmake --build "$BUILD" -j "$JOBS"
		if [ $TEST = 1 ]; then (cd "$BUILD" && ctest --output-on-failure); fi
		cmake --install "$BUILD"
	done

	(cd "$ROOT/dist" && tar czf "$NAME.tar.gz" "$NAME")
	echo
	echo "dist/$NAME.tar.gz"
}

if [ "$EDITIONS" != paid ]; then package OFF "" "$@"; fi
if [ "$EDITIONS" != free ]; then package ON "-paid" "$@"; fi
