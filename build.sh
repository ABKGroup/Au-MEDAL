#!/usr/bin/env bash
# Configures and builds the router into build/flow.
# Usage: ./build.sh [--with-z3] [-DZ3_ROOT=<dir with include/ and lib/>] [extra cmake args]
# --with-z3     downloads z3 (tag pinned below) and builds it into build/z3 first.
set -euo pipefail
cd "$(dirname "$0")"

Z3_TAG="z3-4.15.4"
WITH_Z3=0
while [ $# -gt 0 ]; do
  case "$1" in
    --with-z3) WITH_Z3=1; shift ;;
    *) break ;;
  esac
done

if [ "$WITH_Z3" = 1 ]; then
  Z3_PREFIX="$PWD/build/z3"
  if [ ! -f "$Z3_PREFIX/lib/libz3.so" ]; then
    [ -d build/z3-src ] || git clone --depth 1 --branch "$Z3_TAG" https://github.com/Z3Prover/z3 build/z3-src
    cmake -S build/z3-src -B build/z3-src/build -DCMAKE_BUILD_TYPE=Release \
      -DZ3_BUILD_LIBZ3_SHARED=TRUE -DCMAKE_INSTALL_PREFIX="$Z3_PREFIX" \
      -DCMAKE_INSTALL_LIBDIR=lib \
      ${CXX:+-DCMAKE_CXX_COMPILER="$CXX" -DCMAKE_SHARED_LINKER_FLAGS="-static-libstdc++ -static-libgcc"}
    cmake --build build/z3-src/build -j"$(nproc)"
    cmake --install build/z3-src/build
  fi
  set -- "-DZ3_ROOT=$Z3_PREFIX" "$@"
fi

cmake -S . -B build "$@"
cmake --build build -j"$(nproc)"
