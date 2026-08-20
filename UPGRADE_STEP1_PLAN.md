# LKSCOIN Core — Step 1 Upgrade Plan: Rebase to Dash Core 0.17.0.3

**Status:** Engineering plan — companion to [UPGRADE_ROADMAP.md](UPGRADE_ROADMAP.md)
**Scope:** First incremental rebase step only (0.16 line → 0.17 line)
**Last updated:** 2026-07-29

---

## 1. Verified baseline (source-level analysis)

The following was established by direct, file-by-file comparison of the LKSCOIN
source tree against upstream Dash Core release tags (`v0.15.0.0`, `v0.16.0.1`,
`v0.16.1.1`, `v0.17.0.3`), after normalizing branding strings, copyright lines
and line endings:

| Reference tag | Files identical (normalized) | Files different |
|---|---|---|
| Dash v0.15.0.0 | 0 | 679 |
| Dash v0.16.0.1 | 646 | 56 |
| **Dash v0.16.1.1** | **662** | **40** |
| Dash v0.17.0.3 | 255 | 427 |

**Conclusion: LKSCOIN Core 3.3.0.0 is a rebrand of Dash Core `v0.16.1.1`**
(P2P protocol 70218, 715 source files — both match exactly), with real code
changes confined to ~40 files. This makes the rebase strategy tractable: the
LKSCOIN-specific delta is small and can be re-applied on top of each upstream
tag as we walk forward.

## 2. Inventory of LKSCOIN-specific changes (the "delta")

These are the changes that MUST be preserved, byte-for-byte in consensus
behavior, through every rebase step.

### 2.1 Consensus-critical (any deviation forks the chain)

| Area | File(s) | LKSCOIN behavior |
|---|---|---|
| Block subsidy | `src/randomizer.cpp` (custom file, `#include`d by `validation.cpp`), `GetBlockSubsidy()` | Fixed step schedule via `coinsforblocks()`: 50,000,000 LKS for blocks 0–39; 10,000 for 40–8,039; 45 for 8,040–44,319; base 500 afterwards; plus hardcoded "jackpot" heights paying 100,000–1,000,000 LKS on specific recurring block numbers. Replaces Dash's difficulty-based subsidy + 7.1%/year decline entirely. |
| Max money | `src/amount.h` | `MAX_MONEY = 4,081,632,600 * COIN` (vs Dash 21M). |
| Masternode payment | `src/validation.cpp` `GetMasternodePayment()` | Flat **80%** of block value (vs Dash's progressive 20%→60% schedule + reallocation logic, which is deleted). |
| Masternode collateral | `src/evo/providertx.cpp` | **100,000 LKS** (vs Dash's 1,000). |
| Chain parameters | `src/chainparams.cpp` (~301 changed lines) | Own genesis (timestamp `1496594048`, nonce `786953`, genesis reward 1,000 LKS); default port **9400**; base58 prefixes PUBKEY=76, SCRIPT=16, SECRET=204; BIP44 coin type 5; own BIP34/65/66 heights; DIP0003 at height 307,200 (BIP9 bit 3, own window/threshold 800/640); DIP0008 (bit 4, threshold 80/800 — note: 10%, much lower than Dash's 80%); own checkpoints; own spork address (`XdBu2...`, 1-of-1 key); LLMQ set = 50_60 / 400_60 / 400_85, ChainLocks on 400_60, InstantSend on 50_60. |
| Sporks | `src/spork.h` | Reduced set, highest is `SPORK_22_PS_MORE_PARTICIPANTS`. |

> ⚠️ Note: `pchMessageStart` (network magic `bf 0c 6b bd`) is **identical to
> Dash mainnet's**. Changing it now would be its own network break, so it stays
> — but it should be documented as a known quirk (LKS and Dash nodes can waste
> each other's connection slots before version handshake rejects them).

### 2.2 Non-consensus (must be carried forward, lower risk)

- GUI adaptations: `qt/overviewpage.cpp` (116 lines), `qt/bitcoingui.cpp`,
  `qt/optionsdialog.cpp`, `qt/bitcoinunits.*`, `qt/guiutil.cpp`
- RPC output tweaks: `rpc/misc.cpp`, `rpc/blockchain.cpp`, `rpc/net.cpp`,
  `rpc/mining.cpp`, `rpc/rpcevo.cpp`, `wallet/rpcwallet.cpp`
- Wallet defaults: `wallet/wallet.cpp`, `wallet/init.cpp`, `wallet/coincontrol.h`
- Cleanups: removal of legacy `CBitcoinAddress` class in `base58.*`
- Test adjustments: ~7 test files with LKS-specific expected values
- File renames: `dashd` → `lksd`, `dash-cli` → `lks-cli`, `dash-tx` → `lks-tx`, etc.

## 3. Step-1 target: Dash Core 0.17.0.3

Why 0.17 and not a bigger jump: ~767 changed paths between 0.16.1.1 and
0.17.0.3, including structural moves the later versions build on
(`privatesend/` → `coinjoin/`, new `key_io.cpp`/`bech32`, new `logging.cpp`,
new `interfaces/` and `node/` layers, `blockfilter` / BIP158). Landing this
step cleanly makes every following step (0.18 → 19.x, where the
security-critical BLS migration lives) a repeat of a proven process instead of
a leap.

v0.17 consensus surface is small (no new hard fork required for us at this
step if we do not schedule one), which makes it the right "practice run"
before v0.18 (DIP0024) and v19 (BLS basic scheme).

## 4. Work plan

### Phase 0 — Repo hygiene & reproducibility (prerequisite)

1. **Normalize line endings.** The working tree currently has CRLF endings
   (Windows checkout), which breaks diffing against upstream. Add
   `.gitattributes` (`* text=auto`, `*.cpp text eol=lf`, etc.) and commit a
   normalization pass *before* any real change, so future diffs are clean.
2. **Branching model.** Create `develop` branch; keep `master` = current
   3.3.0.0 release. All rebase work in `feature/rebase-0.17` off `develop`.
3. **Reproduce the current build.** Build `lksd` 3.3.0.0 from the current tree
   (depends system, Ubuntu 22 or gitian) and verify it syncs against the live
   network. *If the current tree does not build cleanly, fixing that comes
   first — we need a known-good starting point.*
4. **Add upstream remote** (`git remote add upstream
   https://github.com/dashpay/dash.git`) and fetch tags, so all future work
   references upstream commits directly.
5. **CI.** Stand up a minimal GitHub Actions workflow: build (Linux x86_64) +
   unit tests on every PR. Without this, every later phase is flying blind.

### Phase 1 — Mechanical port (branding + delta re-application)

Strategy: **start from clean `v0.17.0.3` and re-apply the LKSCOIN delta**
(not the other way around). The delta is 40 files; the 0.16→0.17 upstream
change is 767 paths. Re-applying the small thing onto the big thing is the
lower-error direction.

1. Scripted rebrand of `v0.17.0.3` (dash→lks names, binaries, icons, strings)
   — reproduce exactly the renames listed in §2.2.
2. Re-apply §2.1 consensus delta file-by-file. Special care:
   - `randomizer.cpp` / `GetBlockSubsidy`: keep as-is initially (bug-for-bug
     compatible). Refactoring the `#include "randomizer.cpp"` pattern into a
     proper header/TU is allowed (link-level change only, no behavior change).
   - `GetMasternodePayment`: re-apply flat 80%, delete Dash's reallocation
     schedule again.
   - `chainparams.cpp`: port parameter-by-parameter onto the 0.17 structure
     (it changed shape upstream; do NOT copy the old file wholesale).
   - `providertx.cpp` collateral check: 100,000 LKS (file moved/changed in
     0.17 — re-locate the check).
3. Re-apply §2.2 non-consensus delta (GUI/RPC/wallet), adapting to 0.17's
   refactored interfaces (`interfaces/`, `key_io`).
4. Port LKS test expectations onto the 0.17 test suite.

### Phase 2 — Consensus-parity verification (the critical phase)

1. **Golden-vector subsidy test.** New unit test: table of
   `(height → expected subsidy, expected MN payment)` covering every schedule
   boundary in `coinsforblocks()` (0, 39, 40, 8039, 8040, 44319, 44320, each
   jackpot height family, halving boundaries) — generated from the *old*
   3.3.0.0 binary, asserted against the *new* one.
2. **Full mainnet sync from genesis** with `-assumevalid=0` and checkpoints
   disabled, on the new binary, against the live network (or a copied datadir
   + `-reindex`). This is the single strongest regression test available: it
   re-validates every historical block under the new code. Target: tip hash
   identical to a 3.3.0.0 node.
3. **Cross-version mixed network on devnet:** old 3.3.0.0 nodes + new nodes
   on the same devnet; verify block relay, mempool relay, MN list sync, DKG,
   ChainLocks, InstantSend locks all function across versions (protocol
   70218 ↔ 70219 interop).
4. Unit + functional test suite green on CI.

### Phase 3 — Testnet deployment

1. Reactivate/verify LKS testnet params in the new tree; spin up ≥4 testnet
   masternodes (minimum for llmq_test DKG) + miner + explorer.
2. Run ≥2 weeks: monitor quorum formation, ChainLocks continuity, InstantSend,
   PoSe scoring, reorg handling.
3. Publish testnet binaries + a public call for community testing.

### Phase 4 — Release engineering & mainnet rollout

1. Version: **4.0.0.0**; protocol bumps to 70219 (as upstream 0.17), keeping
   `MIN_PEER_PROTO_VERSION` at a value that tolerates 3.3.0.0 peers during the
   transition window.
2. Release notes documenting every user-visible change inherited from
   upstream 0.17 (RPC renames, `privatesend`→`coinjoin` options, new logging
   flags, wallet behavior).
3. Deterministic/reproducible builds (gitian or guix per 0.17 docs) for
   Linux/Windows/macOS; signed binaries.
4. Coordinated rollout: announce → masternodes upgrade window (2–4 weeks) →
   monitor adoption via `protx list` / peer versions → only then consider any
   spork-gated behavior change. **No hard fork is scheduled in this step**, so
   rollout risk is limited to P2P/relay compatibility, which Phase 2.3 covers.

### Exit criteria for Step 1 (all must hold)

- New binary fully syncs mainnet from genesis with `assumevalid=0`, tip hash
  matches 3.3.0.0.
- Golden-vector subsidy/payment tests pass.
- Mixed-version devnet: quorums form, ChainLocks + InstantSend work.
- ≥2 weeks stable public testnet.
- Reproducible builds published and verified by ≥2 independent builders.

## 5. Estimated effort

For one experienced C++ developer familiar with the Bitcoin/Dash codebase
(multiply accordingly if ramping up):

| Phase | Estimate |
|---|---|
| Phase 0 (hygiene, build, CI) | 1–2 weeks |
| Phase 1 (mechanical port) | 3–5 weeks |
| Phase 2 (consensus verification) | 2–3 weeks (sync time included) |
| Phase 3 (testnet) | 2+ weeks elapsed (low touch) |
| Phase 4 (release) | 1–2 weeks |
| **Total** | **~2.5–3.5 months** |

Subsequent steps (0.18 with DIP0024, then v19 with the BLS scheme hard fork)
reuse Phases 1–4 as a template; v19 will additionally need hard-fork
activation planning and ideally external review, as noted in the roadmap.

## 6. Open items / needed from maintainers

1. **Does the current tree build and sync?** Needs confirmation on a clean
   machine — this is Phase 0.3 and gates everything.
2. **Live network status:** current chain height, number of reachable nodes,
   number of active masternodes, working DNS seeds. (Fixed seeds in the source
   are from 2021 and may be dead.)
3. **Spork key custody:** who holds the private key for spork address
   `XdBu2FTWQRqeZCShnYtJ37fHVRaZPXLtzw`? Required for any spork-gated rollout.
4. **Any private patches** deployed on production nodes that are not in this
   repository must be disclosed and folded into the delta inventory (§2).
5. **Explorer / infrastructure access** for verifying tip hashes during
   Phase 2.

---

*Analysis methodology: file-level normalized diff (branding strings, copyright
lines and CRLF endings neutralized) between the LKSCOIN tree and upstream Dash
Core tags fetched from `dashpay/dash`. Full command log available on request.*
