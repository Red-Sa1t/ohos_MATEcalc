#!/usr/bin/env bash
# Cross-compile GMP / MPFR / MPC as static libraries for OpenHarmony / HarmonyOS.
# Run with MSYS2 bash:  C:/msys64/usr/bin/bash.exe third_party/build-mp.sh [arm64-v8a|x86_64]...
# Uses the NDK bundled with DevEco Studio through the space-free junction third_party/ndk.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd -W)"
NDK="$ROOT/ndk"
SRC="$ROOT/src"
JOBS="${JOBS:-8}"
mkdir -p "$ROOT/tmp"
export TMP="$ROOT/tmp" TEMP="$ROOT/tmp"

GMP=gmp-6.3.0
MPFR=mpfr-4.2.1
MPC=mpc-1.3.1

# Download and unpack the sources when missing
fetch() {
    local name="$1" url="$2"
    if [ ! -d "$SRC/$name" ]; then
        mkdir -p "$SRC"
        (cd "$SRC" && curl -sSfLO "$url" && tar xf "$(basename "$url")")
    fi
}
fetch "$GMP" "https://ftp.gnu.org/gnu/gmp/$GMP.tar.xz"
fetch "$MPFR" "https://ftp.gnu.org/gnu/mpfr/$MPFR.tar.xz"
fetch "$MPC" "https://ftp.gnu.org/gnu/mpc/$MPC.tar.gz"

build_abi() {
    local abi="$1" triple host
    case "$abi" in
        arm64-v8a) triple=aarch64-linux-ohos; host=aarch64-linux-gnu ;;
        x86_64)    triple=x86_64-linux-ohos;  host=x86_64-linux-gnu ;;
        *) echo "unknown abi $abi" >&2; exit 1 ;;
    esac

    local prefix="$ROOT/prebuilt/$abi"
    local work="$ROOT/build/$abi"
    mkdir -p "$prefix" "$work"

    export CC="$NDK/llvm/bin/clang.exe --target=$triple --sysroot=$NDK/sysroot"
    export CFLAGS="-O2 -fPIC"
    export AR="$NDK/llvm/bin/llvm-ar.exe"
    export RANLIB="$NDK/llvm/bin/llvm-ranlib.exe"
    export NM="$NDK/llvm/bin/llvm-nm.exe"
    export STRIP="$NDK/llvm/bin/llvm-strip.exe"
    export CC_FOR_BUILD="/d/Tools/mingw64/bin/gcc.exe"

    local common=(--host="$host" --prefix="$prefix" --enable-static --disable-shared)

    if [ ! -f "$prefix/lib/libgmp.a" ]; then
        rm -rf "$work/gmp" && mkdir -p "$work/gmp" && cd "$work/gmp"
        ../../../src/$GMP/configure "${common[@]}" --disable-assembly
        make -j"$JOBS" && make install
    fi

    if [ ! -f "$prefix/lib/libmpfr.a" ]; then
        rm -rf "$work/mpfr" && mkdir -p "$work/mpfr" && cd "$work/mpfr"
        ../../../src/$MPFR/configure "${common[@]}" --with-gmp="$prefix"
        make -j"$JOBS" && make install
    fi

    if [ ! -f "$prefix/lib/libmpc.a" ]; then
        rm -rf "$work/mpc" && mkdir -p "$work/mpc" && cd "$work/mpc"
        ../../../src/$MPC/configure "${common[@]}" --with-gmp="$prefix" --with-mpfr="$prefix"
        make -j"$JOBS" && make install
    fi

    echo "== $abi done: $prefix"
}

if [ $# -eq 0 ]; then set -- arm64-v8a x86_64; fi
for abi in "$@"; do build_abi "$abi"; done
