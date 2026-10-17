#!/bin/sh
# ---------------------------------------------------------------------------
#  Raven - dist.sh
#  Builds what is given to people who use Raven: the static and the shared
#  library (Release), the headers, the designer, the bindings and the
#  documentation, in one folder and one archive.
#
#    ./dist.sh [--no-test] [--keys <folder>] [-- extra cmake arguments]
#
#  Each library is there twice: the license-free edition (a watermark in
#  every window), and the Release edition encrypted beside it (<file>.rvnenc).
#  A program configured with -DRAVEN_LICENSE_FILE=<license> gets the Release
#  edition: find_package(raven) checks the license with bin/raven_license_check
#  and decrypts it. Encrypting needs the unlock key: --keys, or
#  $RAVEN_LICENSE_KEYS, or ~/.raven-license (raven_license keygen).
#
#    dist/raven-<version>-linux-<arch>/
#        include/                 Raven.h, raven/*.h, raven_web.h
#        lib/                     libraven.so (+ .rvnenc), libraven_web.so, cmake/raven
#        lib/static/              libraven.a (+ .rvnenc), libraven_web.a, cmake/raven
#        bin/raven_designer, bin/raven_license_check
#        share/raven/             README.md, docs/, bindings/
#    dist/raven-<version>-linux-<arch>.tar.gz
# ---------------------------------------------------------------------------
set -e

ROOT=$(cd "$(dirname "$0")" && pwd)
TEST=1
KEYS=""
while [ $# -gt 0 ]; do
	case "$1" in
		--no-test) TEST=0; shift ;;
		--keys) KEYS="$2"; shift 2 ;;
		--) shift; break ;;
		*) echo "dist.sh: unknown option $1" >&2; exit 2 ;;
	esac
done
[ -n "$KEYS" ] || KEYS="${RAVEN_LICENSE_KEYS:-$HOME/.raven-license}"
if [ ! -f "$KEYS/unlock.key" ]; then
	echo "dist.sh: no unlock key in $KEYS: run raven_license keygen (once), or give --keys" >&2
	exit 1
fi
if [ ! -f "$ROOT/tools/license/public.key" ]; then
	echo "dist.sh: tools/license/public.key is missing: copy it from $KEYS" >&2
	exit 1
fi
if ! cmp -s "$ROOT/tools/license/public.key" "$KEYS/public.key"; then
	echo "dist.sh: tools/license/public.key is not the public key of $KEYS: the licenses would not be accepted" >&2
	exit 1
fi

VERSION=$(sed -n 's/^project(raven VERSION \([0-9.]*\).*/\1/p' "$ROOT/CMakeLists.txt")
NAME="raven-$VERSION-linux-$(uname -m)"
OUT="$ROOT/dist/$NAME"
JOBS=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)

rm -rf "$OUT" "$ROOT/dist/$NAME.tar.gz"

# the static library last: the designer that stays in bin/ is the one that needs nothing beside it
for KIND in shared static; do
	if [ "$KIND" = shared ]; then SHARED=ON; LIBDIR=lib; else SHARED=OFF; LIBDIR=lib/static; fi

	echo "== $KIND library, license-free edition"
	BUILD="$ROOT/build/dist-$KIND"
	cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release -DRAVEN_BUILD_SHARED=$SHARED -DRAVEN_EDITION=LicenseFree \
		-DRAVEN_BUILD_EXAMPLES=OFF -DCMAKE_INSTALL_PREFIX="$OUT" -DCMAKE_INSTALL_LIBDIR=$LIBDIR "$@"
	cmake --build "$BUILD" -j "$JOBS"
	if [ $TEST = 1 ]; then (cd "$BUILD" && ctest --output-on-failure); fi
	cmake --install "$BUILD"

	echo "== $KIND library, Release edition (encrypted)"
	RELEASE="$ROOT/build/dist-$KIND-release"
	cmake -S "$ROOT" -B "$RELEASE" -DCMAKE_BUILD_TYPE=Release -DRAVEN_BUILD_SHARED=$SHARED -DRAVEN_EDITION=Release \
		-DRAVEN_BUILD_EXAMPLES=OFF -DRAVEN_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX="$RELEASE/stage" -DCMAKE_INSTALL_LIBDIR=$LIBDIR "$@"
	cmake --build "$RELEASE" -j "$JOBS"
	rm -rf "$RELEASE/stage"
	cmake --install "$RELEASE"
	cmake -DTOOL="$RELEASE/bin/raven_license" -DKEYS="$KEYS" -DSTAGE="$RELEASE/stage" -DOUT="$OUT" \
		-P "$ROOT/tools/license/encrypt_release.cmake"
done

if [ ! -x "$OUT/bin/raven_license_check" ]; then
	echo "dist.sh: bin/raven_license_check is missing from the package" >&2
	exit 1
fi
(cd "$ROOT/dist" && tar czf "$NAME.tar.gz" "$NAME")
echo
echo "dist/$NAME.tar.gz"
