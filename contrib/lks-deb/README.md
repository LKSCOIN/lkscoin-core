# Debian/Ubuntu package for LKSCOIN Core

`build-deb.sh` produces a `.deb` with the same layout as the original
`LKSCoinCore_3300.deb`, so masternode operators can upgrade in place with
`dpkg -i`.

## Building

Build on the **oldest** distribution you want to support. LKSCOIN production
nodes run Ubuntu 18.04 (glibc 2.27), and glibc is not forward compatible: a
package built on Ubuntu 22.04 will fail on 18.04 with
`version 'GLIBC_2.34' not found`.

```bash
cd ~/lkscoin-core
./configure --prefix=$PWD/depends/x86_64-pc-linux-gnu --without-gui
make -j$(nproc)
./contrib/lks-deb/build-deb.sh
```

Result: `LKSCoinCore_<version>.deb` in the current directory.

## Installing / upgrading a node

```bash
lks-cli stop            # or: systemctl stop <your-lksd-service>
sleep 15
cp ~/.lkscore/wallet.dat ~/wallet.dat.backup-$(date +%F)   # always keep a backup

sudo dpkg -i LKSCoinCore_<version>.deb

lksd --version          # confirm the new version
lksd -daemon            # or: systemctl start <your-lksd-service>
lks-cli getnetworkinfo | grep subversion   # confirm the RUNNING daemon
```

Notes:

- **No reindex is needed** when upgrading from 3.2.0.1 / 3.3.0.0 to 4.17.x:
  the existing data directory is read as-is.
- Masternode configuration is untouched: same `lks.conf`, same BLS operator
  key, same ProTx. No re-registration.
- Restart promptly to avoid a PoSe ban for prolonged inactivity.
- If a supervisor (systemd/cron) manages `lksd`, it may restart the daemon
  automatically after `lks-cli stop` — check with
  `lks-cli getnetworkinfo | grep subversion` that the *running* daemon is the
  new one, not just the installed binary (a running process keeps executing
  the old binary image even after the file has been replaced).
- Sentinel is still required with this release and keeps working unchanged.
