# LKSCOIN Core 5.18.2.1 — Porting Status (Step 2: rebase to Dash 18.2.2)

**State:** Phase 1 (mechanical port) complete — NOT compiled yet, NOT release-ready.
**Base:** clean `dashpay/dash` tag `v18.2.2`, rebranded + LKSCOIN delta re-applied.
**Previous step:** 4.17.3.2 (Dash 0.17.0.3), in production since August 2026.

## Method

The delta was extracted automatically this time: the same rebrand script was
applied to a clean `v0.17.0.3`, diffed against the production 4.17 tree, and the
resulting patch (6,475 lines over ~50 files) applied to a rebranded `v18.2.2`.
57 hunks were rejected because of upstream restructuring and were re-applied by
hand. The rebrand script now **excludes binary assets** — the first port
corrupted the bundled Montserrat fonts by renaming the `endash`/`emdash` glyph
names inside the OTF files.

## Applied and verified

| Item | Notes |
|---|---|
| Rebrand | 0 residual `dash` strings in `src/` (external `bls-dash`, univalue, leveldb, secp256k1 untouched); fonts byte-identical to upstream |
| Block subsidy (`randomizer.cpp`) | byte-identical to the 4.17 file in production |
| `GetMasternodePayment` flat 80% | applied |
| `MAX_MONEY` 4,081,632,600 | applied |
| Masternode collateral 100,000 LKS | `evo/deterministicmns.cpp` (2 checks) and `rpc/evo.cpp` (upstream moved it from `rpcevo.cpp`) |
| Chain parameters | 42 mainnet / 35 testnet / 16 devnet / 10 regtest values ported into the 18.2.2 structure, plus genesis, message start, ports, base58 prefixes, spork address, checkpoints, chainTxData |
| Fixed seeds | LKS nodes on port 9400 (mainnet); testnet list empty (`vFixedSeeds.clear()`, upstream's `std::begin` does not compile on a zero-length array) |
| CoinJoin hard-disabled | client (`wallet/init.cpp`), GUI options + tab, mixing **server** on masternodes (`net_processing.cpp`, `init.cpp`), RPC |
| LKSCOIN artwork | icons, splash, toolbar logos, pixmaps copied (binary, not carried by the patch) |
| `LLMQ_LKS_10_60` | new quorum type (10/7/6) in `llmq/params.h`, `available_llmqs` resized to 13, registered on main/test/devnet, gated by height in `llmq/utils.cpp` |
| ChainLocks quorum | moved from `LLMQ_400_60` to `LLMQ_LKS_10_60` |
| Masternode purge | `PurgeInactiveMNs()` + call site + header declaration; error API adapted to 18.2.2 (`state.Invalid(ValidationInvalidReason::CONSENSUS, …)`) |
| Golden-vector test | `test/lks_subsidy_tests.cpp` present and wired into `Makefile.test.include` |
| Version | 5.18.2.1, `IS_RELEASE=false` |

Syntax-checked with g++ (C++17) against the real 18.2.2 headers:
`chainparams.cpp`, `consensus/params.h`, `llmq/params.h`, `amount.h`,
`randomizer.cpp` — 0 errors.

## Dormant by design

Inherited from upstream but deliberately **not** scheduled. None of these
activates without an explicit decision and a coordinated fork:

- `DEPLOYMENT_DIP0024` (quorum rotation): start pushed to 2030, timeout infinite,
  on all four networks.
- `BRRHeight` (block reward reallocation): `std::numeric_limits<int>::max()` —
  LKSCOIN keeps the flat 80% masternode payment.
- `nMNPurgeStartHeight`, `nMNPurgeHeight`, `nLKSSmallQuorumHeight`: all 0, so
  the masternode purge and the small quorum type are inert.

## Open items before any release

1. **First build.** This tree has never been compiled. Expect fixes — the 4.17
   port needed several rounds. Build on Ubuntu 18.04 for distributable binaries
   (glibc 2.27); check whether 18.2.2's `depends` still needs the GCC ≤ 10
   constraint and the Boost glibc-2.34 patch that 0.17 required (its Boost is
   newer, so it may not).
2. ~~**Version scheme.**~~ **RESOLVED:** the four-part LKSCOIN scheme is kept —
   `<LKS major>.<Dash major>.<Dash minor>.<LKS patch>`, so this release is
   **5.18.2.1**. Dash 18 had dropped the fourth component and changed the
   numeric formula (`10000*MAJOR + 100*MINOR + BUILD`), which would have made
   the integer version *decrease* from 4170302 to 51802 and confused any tool
   comparing versions. The 0.17-era formula was restored
   (`1000000*MAJOR + 10000*MINOR + 100*BUILD + LKS_PATCH` = 5180201), together
   with a new `CLIENT_VERSION_LKS_PATCH` macro wired through `configure.ac`,
   `clientversion.h/.cpp` and the five Windows resource files. This is a
   deliberate local divergence from upstream: expect a small conflict on these
   files at every future Dash merge.
3. **Consensus parity test.** Full sync from genesis with `-assumevalid=0`,
   comparing the tip hash against a 4.17 node — the test that caught the two
   real defects last time.
4. **Protocol version.** 18.2.2 raises `PROTOCOL_VERSION` and
   `MIN_PEER_PROTO_VERSION`: verify that 4.17 peers (70219) are still accepted,
   otherwise the network splits into two non-communicating groups during rollout.
5. **Devnet.** Exercise the full LLMQ path with the new quorum type before
   considering any activation height.
6. **Remaining GUI/RPC review.** Minor cosmetic deltas from 3.3.0.0 were never
   ported and remain to be reviewed side by side.
