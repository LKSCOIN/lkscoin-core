#!/usr/bin/env bash
# Build a .deb package for LKSCOIN Core, matching the layout of the
# original LKSCoinCore_3300.deb (files installed under /bin, /lib,
# /include and /share/man, no maintainer scripts).
#
# Run this from the root of a tree that has already been built, e.g.:
#
#   cd ~/lkscoin-core
#   ./configure --prefix=$PWD/depends/x86_64-pc-linux-gnu --without-gui
#   make -j$(nproc)
#   ./contrib/lks-deb/build-deb.sh
#
# The resulting package is written to the current directory.
#
# NOTE: build on the OLDEST distribution you intend to support
# (LKSCOIN production nodes run Ubuntu 18.04 / glibc 2.27). A package
# built on a newer distro will NOT run on older ones.

set -e

if [ ! -f src/lksd ]; then
    echo "ERROR: src/lksd not found - build the tree first (make -j\$(nproc))." >&2
    exit 1
fi

VERSION=$(src/lksd --version | head -1 | sed -E 's/.*version v([0-9.]+).*/\1/')
if [ -z "$VERSION" ]; then
    echo "ERROR: could not determine version from src/lksd --version" >&2
    exit 1
fi

PKGDIR=$(mktemp -d)
OUTFILE="LKSCoinCore_${VERSION}.deb"

echo "Packaging LKSCOIN Core ${VERSION} ..."

# Install the built tree into the staging directory with prefix "/",
# exactly like the 3.3.0.0 package did.
make install DESTDIR="${PKGDIR}" prefix=/ >/dev/null

# Drop test/benchmark binaries: they were shipped in the old package by
# accident and are of no use to node operators (they add ~170 MB).
rm -f "${PKGDIR}/bin/test_lks" "${PKGDIR}/bin/test_lks-qt" "${PKGDIR}/bin/bench_lks"

# The staging directory is created by mktemp with mode 0700 and is owned by
# the building user. Both would end up recorded in the package: dpkg would
# then apply 0700 to "/" and install user-owned binaries into /bin. Normalise
# directory permissions here and force root:root ownership at build time.
find "${PKGDIR}" -type d -exec chmod 755 {} +

mkdir -p "${PKGDIR}/DEBIAN"
cat > "${PKGDIR}/DEBIAN/control" <<EOF
Package: lkscoincore
Version: ${VERSION}
Section: base
Priority: optional
Architecture: amd64
Maintainer: LKSCoin Foundation <admin@lksfoundation.org>
Description: LKSCoin Core version ${VERSION}
EOF

dpkg-deb --root-owner-group --build "${PKGDIR}" "${OUTFILE}" >/dev/null
rm -rf "${PKGDIR}"

echo "Done: ${OUTFILE}"
dpkg-deb -I "${OUTFILE}" | head -12
echo
echo "Contents:"
dpkg-deb -c "${OUTFILE}" | awk '{print $1, $3, $6}'
