# LKSCOIN Core 5.18.2.1 Release Notes

LKSCOIN Core 5.18.2.1 rebases the codebase onto **Dash Core 18.2.2**, two
upstream lines ahead of 4.17.3.2 (Dash 0.17.0.3), which has been in production
since August 2026. It carries the accumulated upstream work — networking, wallet,
RPC, performance and the associated Bitcoin Core backports — and adds the
groundwork for the network revival described in
[`network-revival-plan.md`](network-revival-plan.md).

**This release changes no consensus rules and activates no hard fork.**
It is a drop-in replacement for 4.17.3.x: new and old nodes interoperate, so
operators can upgrade at their own pace.

Report issues at <https://github.com/LKSCOIN/lkscoin-core/issues>.

## Upgrading

Stop the daemon, install the new binaries, start it again. **No reindex is
required** and the existing data directory is used as-is.

```bash
lks-cli stop && sleep 15
sudo dpkg -i LKSCoinCore_5.18.2.1.deb
lksd -daemon
```

With systemd (typical masternode setup):

```bash
systemctl stop lksd
dpkg -i LKSCoinCore_5.18.2.1.deb
systemctl start lksd
```

Then confirm the **running** daemon, not just the installed file — a running
process keeps executing the old binary image after the file has been replaced,
and some setups restart `lksd` automatically:

```bash
lks-cli getnetworkinfo | grep subversion     # /Lksc Core:5.18.2.1/
```

### Masternodes

No re-registration and no configuration change: same `lks.conf`, same BLS
operator key, same ProTx, same collateral. After restarting, verify:

```bash
lks-cli masternode status                    # state: READY, PoSePenalty: 0
lks-cli mnsync status | grep AssetName       # MASTERNODE_SYNC_FINISHED
```

**Please upgrade.** Masternodes are compatible with 4.17 peers today because no
quorum is forming, but 18.x requires protocol 70221 or later for DKG
participation, and 4.17 speaks 70219. When the quorums are switched back on, a
masternode left on 4.17 would be treated as a non-participant and would collect
PoSe penalties. Every masternode must be on 5.18.x before that happens.

### Downgrading

Supported: the data directory format is unchanged and no irreversible database
upgrade is performed. Reinstall the previous package and restart.

## Notable changes

**Rebase onto Dash Core 18.2.2.** Protocol version is now 70224 (was 70219);
`MIN_PEER_PROTO_VERSION` is 70215, so 4.17 peers are still accepted.

**Groundwork for the network revival, dormant by default.** Two mechanisms are
present but switched off, and neither activates without an explicit decision and
a coordinated fork:

- `LLMQ_LKS_10_60`, a quorum type (10 members, minimum 7, threshold 6) sized for
  the surviving LKSCOIN network, gated by `nLKSSmallQuorumHeight`. ChainLocks are
  assigned to it instead of `LLMQ_400_60`, whose DKG would need 300 participants.
- A masternode purge: at `nMNPurgeHeight`, entries that published no provider
  special transaction during an announced re-registration window are removed from
  the deterministic list. **Collateral is never touched** and removed operators
  can register again at any time.

Both are inert in this release (`nMNPurgeStartHeight`, `nMNPurgeHeight` and
`nLKSSmallQuorumHeight` are all 0). Rationale and the measurements behind them
are in [`network-revival-plan.md`](network-revival-plan.md).

**Upstream features deliberately not scheduled.** DIP0024 (quorum rotation) has
its activation window pushed out and `BRRHeight` is set to `INT_MAX`: LKSCOIN
keeps its flat 80% masternode payment and does not adopt Dash's block reward
reallocation.

**CoinJoin remains disabled** at every level: mixing client, GUI tab and
checkbox, the masternode-side mixing server, and the RPC.

**Testing infrastructure fixed.** The functional test framework could not run any
masternode test on LKSCOIN: it used Dash's 1,000 collateral, Dash's genesis
timestamp and Dash's regtest spork address, and the regtest chain had inherited
mainnet difficulty bits (~20 s per block). All corrected, and a new functional
test, `feature_lks_mn_purge.py`, exercises the purge end to end.

## Verification

The release was validated the same way as 4.17.3.2:

- **Consensus parity.** A 5.18.2.1 node re-validated the entire chain from
  genesis with `-assumevalid=0` — every signature and special transaction
  re-checked — reaching height 994969. At height 994959 its block hash was
  `000000001fecea56f4c776d40a4c883cdd0b43919a9f6ab06baddd99c491d9c3`,
  **identical** to the production 4.17.3.2 node.
- **Network state.** The deterministic masternode list was rebuilt identically
  (702 registered, 503 enabled) and the LLMQ sets match.
- **Interoperability.** The syncing node held 34 peers, a mix of 4.17 and 5.18.
- **Unit and functional tests** pass, including the golden-vector subsidy test
  and the new masternode purge test.

## Known issues

- The LKSCOIN network currently produces no quorums, so ChainLocks and
  InstantSend are unavailable. This predates the release and is the subject of
  the revival plan.
- `getblockchaininfo` reports the legacy `realloc` deployment as active since
  height 420800. This is cosmetic: in 18.x that deployment drives nothing, and
  the masternode payment is a flat 80% regardless.
- The LKSCOIN testnet is not operating; its parameters are carried over but its
  fixed seed list is empty.
- The GUI wallet is provided for Windows only.
- The activation path of `LLMQ_LKS_10_60` has not yet been rehearsed end to end
  (the purge has). It stays dormant in this release.

## Credits

Thanks to everyone running a node, a masternode, a miner or an explorer for the
LKSCOIN network.
