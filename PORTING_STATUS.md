# LKSCOIN Core 5.18.2.1 — Porting Status (Step 2: rebase to Dash 18.2.2)

**State:** Phase 2 COMPLETE — builds and consensus parity proven.

> **Full-sync validation result (2026-08-21)**
> A 5.18.2.1 node re-validated the whole LKSCOIN mainnet from genesis with
> `-assumevalid=0` and reached height **994969**; at height 994959 its block
> hash was `000000001fecea56f4c776d40a4c883cdd0b43919a9f6ab06baddd99c491d9c3`,
> **identical** to the production 4.17.3.2 node. Masternode list rebuilt
> identically (702 registered / 503 enabled), same LLMQ sets, 34 peers -
> proving 5.18 and 4.17 nodes interoperate. `dip0020` and `dip0024` both report
> `defined`, and `llmq_lks_10_60` does not appear in `quorum list`: the dormant
> switches hold. The golden-vector subsidy test passes.
>
> Note: `getblockchaininfo` reports the legacy `realloc` deployment as active
> since height 420800. This is cosmetic: in 18.x that BIP9 deployment no longer
> drives anything (reallocation is keyed off `BRRHeight`, set to INT_MAX here),
> and `GetMasternodePayment` returns a flat 80% regardless.
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

1. ~~**First build.**~~ **DONE.** Builds on Ubuntu 22.04 with the system GCC:
   18.2.2 needs neither the GCC ≤ 10 constraint nor the Boost glibc-2.34 patch
   that 0.17 required. Distributable binaries still have to be built on Ubuntu
   18.04 for glibc 2.27 compatibility.
   Defects found and fixed during the first build: the `randomizer.cpp` include,
   `CValidationState::DoS()` → `Invalid(ValidationInvalidReason::CONSENSUS,…)`,
   `ForEachMN` now passing a dereferenced masternode, the test fixture header
   move, the dead Boost URL, the missing `src/config/.empty`, `dashpay` being
   rebranded to `lkspay` (breaking the bls-signatures download), the LKSCOIN DNS
   seeds never ported, and six `Update*` helpers whose bodies had been
   overwritten by the automated parameter port.
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
3. ~~**Consensus parity test.**~~ **PASSED** — see the box at the top.
4. ~~**Protocol version.**~~ **CHECKED.** 5.18.2.1 speaks protocol 70224 and
   accepts peers from 70215, so 4.17 nodes (70219) remain fully compatible - the
   34 peers observed during the sync confirm it in practice.
   **However `MIN_MASTERNODE_PROTO_VERSION` is 70221**, above 4.17's 70219, and
   it is used in `llmq/dkgsession.cpp`: a masternode still on 4.17 would be
   treated as a non-participant in a DKG run by 5.18 nodes and would collect bad
   votes, leading to PoSe penalties. Harmless today (no DKG completes), but it
   means **every masternode must be upgraded before the quorums are switched
   back on** - the re-registration window has to close before the small-quorum
   activation height, not after.
5. **Regtest/devnet rehearsal — the real remaining gap.** `PurgeInactiveMNs()`
   and `LLMQ_LKS_10_60` compile but **have never been executed**: on every real
   network they are switched off. `-mnpurgeparams=<start>:<purge>` and
   `-lkssmallquorumheight=<height>` were added (regtest-only) so the whole cycle
   can be rehearsed without recompiling.
6. **Remaining GUI/RPC review.** Minor cosmetic deltas from 3.3.0.0 were never
   ported and remain to be reviewed side by side.
