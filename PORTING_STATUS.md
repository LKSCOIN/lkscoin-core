# LKSCOIN Core 4.17.3.0 — Porting Status (Step 1: rebase to Dash 0.17.0.3)

**State:** Phase 1 (mechanical port) — first working draft. NOT buildable-verified yet, NOT release-ready.
**Base:** clean `dashpay/dash` tag `v0.17.0.3`, rebranded + LKSCOIN consensus delta re-applied.

## Done in this draft

| Item | Status |
|---|---|
| Full tree rebrand (file renames + string mapping `Dash Core→Lksc Core`, `Dash→Lks`, `dash→lks`, `DASH→LKS`) | ✅ 0 residual `dash` strings in `src/` (external libs `bls-dash`, univalue, leveldb, secp256k1 untouched) |
| Version | ✅ 4.17.3.0, `IS_RELEASE=false`, protocol 70219 (inherited from 0.17), `MIN_PEER_PROTO_VERSION=70213` → 3.3.0.0 peers (70218) still accepted |
| `src/randomizer.cpp` (subsidy schedule + jackpots) | ✅ copied verbatim (CRLF-normalized), compiles standalone, golden vectors verified at all schedule boundaries |
| `GetBlockSubsidy` / `GetMasternodePayment` (flat 80%) | ✅ re-applied on 0.17 `validation.cpp` |
| `MAX_MONEY = 4,081,632,600 * COIN` | ✅ `src/amount.h` |
| Masternode collateral 100,000 LKS | ✅ both checks in `src/evo/providertx.cpp` |
| `chainparams.cpp` | ✅ rebuilt: LKS values (genesis, port 9400, prefixes, BIP heights, DIP0003 deployment, spork address, checkpoints, LLMQ set, chainTxData) + 0.17 structure (DIP0008Height field, DIP0020/REALLOC deployments, llmq_test_v17/llmq100_67 defs, fHelpOnly factory, new Update* methods). **Passes g++ syntax check against real 0.17 headers.** |
| `spork.h` | ✅ 0.17 stock (LKS 3.3.0.0 spork set was identical to Dash 0.16.1.1's; 0.17 drops SPORK_22, adds SPORK_23_QUORUM_POSE) |
| Unit test `src/test/lks_subsidy_tests.cpp` | ✅ golden vectors from 3.3.0.0 (subsidy boundaries, jackpots, 80% MN payment, realloc-immunity), wired into Makefile |
| `.gitattributes` | ✅ added (line-ending normalization — the old repo had CRLF everywhere) |

## Open TODOs — must be resolved before ANY release

1. ~~**`consensus.DIP0008Height` mainnet placeholder.**~~ **RESOLVED:** set to
   329600, read from `bip9_softforks.dip0008.since` on a synced 3.3.0.0
   production node (chain height 993303, 2026-07-30). Reference activation
   heights from the same node: bip147=276000, dip0001=307200, dip0003=307200,
   dip0008=329600, csv=334656. Testnet DIP0008Height is still a placeholder
   (testnet to be re-bootstrapped anyway).
2. **DIP0020 activation window (mainnet) is a placeholder** (Oct 2026 → Oct 2027, 80%/800). Community decision needed; it's a hard-fork deployment (new opcodes).
3. **`chainparamsseeds.h` contains DASH mainnet IPs on port 9999** (inherited bug from 3.3.0.0 — the fixed seeds were never regenerated!). Regenerate from real LKS nodes with `contrib/seeds/generate-seeds.py`.
4. **Non-consensus delta not yet ported** (~25 files): GUI (`qt/overviewpage.cpp` 116 lines, `bitcoingui`, `optionsdialog`, `bitcoinunits`), RPC output tweaks (`rpc/misc|blockchain|net|mining|rpcevo`, `wallet/rpcwallet`), wallet defaults, LKS test-value adjustments in ~7 upstream test files. Old repo remains the reference for these.
5. **First full build** via `depends/` (needs a real build machine, see below) + fix whatever the compiler finds — a mechanical port of this size never compiles clean on the first try.
6. **Then Phase 2:** run unit tests, full mainnet sync from genesis with `-assumevalid=0`, mixed-version devnet (per UPGRADE_STEP1_PLAN.md).

## Build machine

Yes — an Ubuntu build machine is needed (sandbox here has no root, can't build the
`depends/` toolchain). Recommended: Ubuntu 22.04, 4+ cores, 16 GB RAM, 40 GB disk:

```bash
sudo apt install build-essential libtool autotools-dev automake pkg-config \
  python3 bsdmainutils cmake curl git ccache
git clone <new-repo-url> && cd lkscoin-core
cd depends && make -j$(nproc)          # builds boost, bdb4.8, bls-dash, qt, etc.
cd .. && ./autogen.sh
./configure --prefix=$(pwd)/depends/x86_64-pc-linux-gnu
make -j$(nproc)
make check                              # runs unit tests incl. lks_subsidy_tests
```

The same machine can later host the gitian/deterministic release builds and a
CI runner.

## Repository recommendation

Create a **new repository** (e.g. `LKSCOIN/lkscoin-core`) instead of pushing this
over the old one:

- The old repo has a single squashed commit — no upstream history to build on anyway.
- A new repo can be initialized from `dashpay/dash` history at `v0.17.0.3` with the
  LKS delta applied as reviewable commits on top: every future upstream merge
  (0.18, v19 BLS...) becomes a real `git merge` instead of another manual diff hunt.
- The old repo stays untouched as the reference/archive for 3.3.0.0 (external
  parties compiled from it; it documents exactly what runs in production today).

Suggested commit structure for the new repo:
1. Import of `dashpay/dash` `v0.17.0.3` (or full upstream history up to that tag)
2. `rebrand: dash -> lks (mechanical, no behavior change)`
3. `consensus: LKSCOIN subsidy schedule (randomizer.cpp) + flat 80% MN payment`
4. `consensus: MAX_MONEY, 100k MN collateral`
5. `chainparams: LKS mainnet/testnet/devnet/regtest parameters`
6. `tests: golden-vector subsidy tests from 3.3.0.0`
7. …then the §2.2 GUI/RPC ports, one commit per area
