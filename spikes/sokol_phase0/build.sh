#!/bin/sh
# usage: build.sh <raven build dir containing libraven.a> <out dir>
set -e
RAVEN_BUILD=${1:?raven build dir}
OUT=${2:?output dir}
HERE=$(cd "$(dirname "$0")" && pwd)
mkdir -p "$OUT"
cc -std=c11 -O1 -g -Wall -Wno-unused-function -I"$HERE/../../include" -I"$HERE" \
  "$HERE/scenes.c" "$HERE/sg_spike_backend.c" -o "$OUT/scenes" \
  "$RAVEN_BUILD/libraven.a" -lEGL -lGLESv2 -lm -ldl -lpthread -lX11
