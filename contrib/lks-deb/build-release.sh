#!/usr/bin/env bash
# Build the distributable LKSCOIN Core artefacts.
#
#   ./contrib/lks-deb/build-release.sh linux     # daemon + cli + .deb
#   ./contrib/lks-deb/build-release.sh windows   # cross-compiled .exe (incl. GUI)
#
# Everything is collected under ~/releases/<version>-<platform>/.
#
# IMPORTANT: run the Linux build on the OLDEST distribution you intend to
# support. LKSCOIN production nodes run Ubuntu 18.04 (glibc 2.27) and glibc is
# not forward compatible: binaries built on 22.04 fail on 18.04 with
# "version 'GLIBC_2.34' not found". The Windows cross-build has no such
# constraint and can be produced anywhere.

set -e
TARGET="${1:-}"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

case "$TARGET" in
linux)
    HOST=x86_64-pc-linux-gnu
    CONFIGURE_EXTRA="--without-gui"
    ;;
windows)
    HOST=x86_64-w64-mingw32
    CONFIGURE_EXTRA=""      # keep the Qt GUI: it is what most users run
    ;;
*)
    echo "usage: $0 linux|windows" >&2
    exit 1
    ;;
esac

echo ">>> Building depends for $HOST (first run takes about an hour)"
if [ "$TARGET" = "linux" ]; then
    make -C depends -j"$(nproc)" NO_QT=1
else
    make -C depends -j"$(nproc)" HOST="$HOST"
fi

echo ">>> Configuring"
./autogen.sh
if [ "$TARGET" = "linux" ]; then
    ./configure --prefix="$ROOT/depends/$HOST" $CONFIGURE_EXTRA
else
    CONFIG_SITE="$ROOT/depends/$HOST/share/config.site" ./configure --prefix=/ $CONFIGURE_EXTRA
fi

echo ">>> Compiling"
make -j"$(nproc)"

VERSION=$(awk -F'[, )]+' '
    /^define\(_CLIENT_VERSION_MAJOR/{a=$2}
    /^define\(_CLIENT_VERSION_MINOR/{b=$2}
    /^define\(_CLIENT_VERSION_BUILD/{c=$2}
    /^define\(_CLIENT_VERSION_LKS_PATCH/{d=$2}
    END{print a"."b"."c"."d}' configure.ac)
OUT="$HOME/releases/$VERSION-$TARGET"
mkdir -p "$OUT"

if [ "$TARGET" = "linux" ]; then
    cp src/lksd src/lks-cli src/lks-tx "$OUT/"
    strip "$OUT"/lks* 2>/dev/null || true
    echo ">>> Building the .deb package"
    ./contrib/lks-deb/build-deb.sh
    mv "LKSCoinCore_$VERSION.deb" "$OUT/" 2>/dev/null || true
else
    cp src/*.exe "$OUT/" 2>/dev/null || true
    cp src/qt/lks-qt.exe "$OUT/" 2>/dev/null || true
    x86_64-w64-mingw32-strip "$OUT"/*.exe 2>/dev/null || true
fi

echo ">>> Checksums"
( cd "$OUT" && sha256sum * > SHA256SUMS && cat SHA256SUMS )

echo
echo "Artefacts in $OUT"
[ "$TARGET" = "linux" ] && "$OUT/lksd" --version | head -1
