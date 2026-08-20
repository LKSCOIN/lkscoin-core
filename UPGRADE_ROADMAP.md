# LKSCOIN Core Modernization Roadmap

**Status:** Proposal / Discussion draft
**Author:** LKSCOIN maintainers
**Last updated:** 2026-07-02

## 1. Purpose of this document

This document is a public, technical starting point for modernizing the LKSCOIN Core codebase. LKSCOIN is a fork of [Dash Core](https://github.com/dashpay/dash), and the fork has not tracked upstream for several years. This proposal lays out, as precisely as we can determine from the current codebase, how far behind we are, what that gap actually costs us (mainly in consensus security and network robustness, not just "missing features"), and a proposed phased approach to closing it.

We are publishing this now, before any code changes land, so the community — masternode operators, exchanges, node runners, and outside contributors — can weigh in on scope and priorities before we commit engineering time to a specific path.

## 2. Where LKSCOIN Core stands today

Based on direct inspection of the current `LKSCOIN` repository:

| Item | Value |
|---|---|
| Reported client version | `3.3.0.0` |
| P2P protocol version | `70218` |
| Deterministic masternodes (DIP0003) | Present |
| ChainLocks / long-living quorums (DIP0008) | Present |
| Spork list | Up to `SPORK_22_PS_MORE_PARTICIPANTS` |
| Quorum rotation (DIP0024) | Not present |
| BLS "basic" signature scheme | Not present (still on legacy BLS scheme) |
| Extraordinary Hard Fork (EHF) activation mechanism | Not present |
| Masternode Reward Reallocation | Not present |
| Sentinel integration into core | Not present (external Sentinel still required) |
| Dash Platform / Evonode support | Not present |
| Repository history | Single squashed commit (April 2021), no linear commit history against upstream |

Taken together, this places the LKSCOIN codebase at roughly the **Dash Core 0.17.x line (mid-2021)**. Current upstream Dash Core, as of this writing, is in the **23.x series**, running P2P protocol `70235`. That is on the order of **five years and more than a dozen upstream releases** of consensus, security, and infrastructure work that LKSCOIN has not received.

This is not a criticism of past work — freezing a fork at a stable point is a legitimate choice — but it does mean the project is currently missing several years of security hardening in code that is directly responsible for moving and securing funds.

## 3. Why this matters

A few points to separate "nice to have" from "actually important":

**Security of the LLMQ signing system.** Upstream Dash migrated from a "legacy" BLS signature scheme to a "basic" scheme (finalized as a hard fork in the 19.x series) specifically to close weaknesses in how quorum signatures were generated and verified. LKSCOIN still runs the legacy scheme. This is the single highest-priority item in this proposal — it affects the integrity of ChainLocks and InstantSend, which is to say, the safety of every confirmed transaction on the network.

**Years of Bitcoin Core backports.** Dash Core is itself built on top of Bitcoin Core, and every major Dash release pulls forward a batch of upstream Bitcoin fixes — networking robustness, wallet correctness, P2P hardening, performance. LKSCOIN has not received any of this since ~2021.

**Network resilience.** Newer protocol versions include things like enforced multi-network outbound connections (to resist partition/eclipse attacks) and larger header-batch sizes for faster initial sync. These are operational, not cosmetic.

**Governance/activation mechanics.** The Extraordinary Hard Fork (EHF) mechanism that upstream now uses lets the network coordinate mandatory upgrades without relying purely on spork flags voted by a small set of signers. Adopting an equivalent mechanism would make *future* upgrades (including the rest of this roadmap) considerably safer to roll out.

**Ecosystem compatibility.** Explorers, wallets, and exchange integrations built against modern Dash-family protocol versions may assume behavior LKSCOIN's stack doesn't yet implement, which becomes an increasing source of friction the longer the gap grows.

## 4. Gap analysis by upstream milestone

This table summarizes what changed in each relevant upstream Dash Core line, and our current read on relevance to LKSCOIN. Priority is our initial assessment for discussion, not a final decision.

| Upstream version | Approx. date | Key changes | Relevance to LKSCOIN | Proposed priority |
|---|---|---|---|---|
| 0.18 | 2022 | DIP0024: quorum rotation (only ~1/4 of quorum members change at a time), quorum count increased, `quorum rotationinfo` RPC | Improves InstantSend quorum robustness and reduces double-signing risk | High |
| 19.x | 2023 | BLS legacy → basic scheme hard fork; mainnet spork hardening; high-performance masternode groundwork | Closes a known signature-scheme weakness; foundational for everything after it | **Highest** |
| 20.x | 2024 | Masternode Reward Reallocation groundwork; Sentinel merged into core binary; Asset Lock (credit-pool bridge toward Platform) | Reward mechanics only relevant if we adopt Dash's tokenomics changes; Sentinel merge simplifies masternode operations regardless | Medium (Sentinel merge), Low/decision-dependent (reward reallocation) |
| 21.x | 2024 | Masternode Reward Reallocation activation; full spork hardening; large batch of Bitcoin 0.20–26 backports | Backports are broadly valuable; reward reallocation is a tokenomics decision, not just a technical one | Medium–High (backports), decision-dependent (reward split) |
| 22.x | 2024 | Asset Unlock improvements, BIP324 encrypted P2P (opt-in), protocol 70235, larger header batches (up to 8,000) | Network-hardening and sync performance, low risk to adopt | Medium |
| 23.x | 2025–2026 | Stability, security, and performance patches on top of the above | Standard maintenance track once we're this far forward | Ongoing |
| Dash Platform (Drive, Tenderdash, Evonodes) | 2023–present | A second, largely independent Layer-2 chain/storage system for decentralized apps and identities, run by a separate "Evonode" class with much higher collateral (4,000 vs 1,000) | Out of scope unless LKSCOIN wants to become an app platform, not just a payments coin | **Needs explicit community decision — recommend deferring** |

## 5. Proposed approach

**5.1 Don't attempt a single big-bang merge.** LKSCOIN's repository has a single squashed commit with no shared history against `dashpay/dash`, and years of LKSCOIN-specific branding, chain parameters, and possibly custom logic have diverged from upstream. Trying to jump straight to `dashpay/dash` HEAD in one pass is high-risk and very hard to review. Instead:

1. Add `dashpay/dash` as a git remote and diff LKSCOIN against the closest matching historical tag (likely `0.17.0.3`) to cleanly separate "LKSCOIN-specific" changes (genesis parameters, branding, reward schedule, network magic, seed nodes, etc.) from "upstream Dash" code.
2. Walk forward version-by-version (0.18 → 19.x → 20.x → 21.x → 22.x → 23.x), re-applying LKSCOIN-specific deltas at each step, rather than rebasing once across the whole gap.
3. Stand up a dedicated devnet (and later a public testnet) to validate each consensus-affecting step before it goes anywhere near mainnet.

**5.2 Sequence by risk, not by version number.** Recommended order:

1. BLS legacy → basic scheme migration (security-critical, foundational for later quorum work).
2. DIP0024 quorum rotation.
3. EHF-equivalent activation mechanism, so subsequent hard forks can be coordinated safely.
4. Sentinel-into-core merge (operational simplification for masternode operators).
5. General Bitcoin Core backports (networking, wallet, sync performance) — can happen in parallel with the above since it's largely non-consensus-critical.
6. Community decision on Masternode Reward Reallocation and any tokenomics changes — this is a governance question as much as a technical one and should not be bundled silently into a "just catching up" release.
7. Community decision on Dash Platform — recommend treating this as a separate, later initiative rather than part of this catch-up effort.

**5.3 Coordinate the rollout like a real hard fork, because it is one.** Every consensus-affecting change in this plan needs: a public testnet period, advance notice to masternode operators and exchanges, and a clear activation mechanism (spork-based initially, migrating to EHF-style once available) so the network doesn't split.

## 6. Risks and open questions

- **This is consensus-critical C++ work.** Bugs here can split the chain or, worse, create a path to fund loss. This should not be treated as routine maintenance — it needs experienced review, and ideally an external audit before mainnet activation of the BLS scheme migration and quorum rotation changes.
- **Chain continuity.** We need to confirm that existing masternode collateral, wallet balances, and UTXO history remain fully intact through every step — this is a forward upgrade of a live chain, not a relaunch.
- **Resourcing.** This is realistically months of focused work, not a weekend patch. We're publishing this proposal partly to gauge community interest in contributing engineering time, review, or testnet participation.
- **Scope creep via Dash Platform.** It would be easy to let "catch up with Dash" quietly turn into "become Dash Platform," which is a much larger commitment (new node class, new collateral tier, new storage/consensus layer). We think this deserves its own separate community discussion rather than being folded into this roadmap by default.
- **Tokenomics changes.** Masternode Reward Reallocation changes the split between miners, masternodes, and treasury. Whether LKSCOIN wants to mirror Dash's split is a policy decision for the community/token holders, not something to inherit automatically by "updating."

## 7. How to get involved

This is a proposal, not a finished plan. We're looking for:

- Review and correction of the technical assessment above (open an issue if you think something here is inaccurate or outdated).
- C++ / consensus-code contributors, particularly anyone with prior experience on Dash Core, Bitcoin Core, or a similar UTXO-chain codebase.
- Masternode operators willing to run devnet/testnet nodes once a first milestone (BLS scheme migration) is ready to test.
- Community input on the two explicit decision points above: Masternode Reward Reallocation and Dash Platform scope.

Please open an issue on this repository to discuss any part of this plan, or comment directly on the tracking issue linked from the README once it is published.

## 8. References

- [Dash Core GitHub releases](https://github.com/dashpay/dash/releases)
- [Dash Core release notes (master)](https://github.com/dashpay/dash/blob/master/doc/release-notes.md)
- [DIP-0024 — Long-Living Masternode Quorum Distribution and Rotation](https://github.com/dashpay/dips/blob/master/dip-0024.md)
- [DIP-0028 — Evolution Masternodes](https://github.com/dashpay/dips/blob/master/dip-0028.md)
- [DashCore v18.0 Product Brief](https://www.dash.org/blog/dashcore-v18-0-product-brief/)
- [DashCore v19.0 Product Brief](https://www.dash.org/blog/dashcore-v19-0-product-brief/)
- [DashCore v20.0 Product Brief](https://www.dash.org/blog/dashcore-v20-0-product-brief/)
- [DashCore v21.0 Release Announcement](https://www.dash.org/news/dashcore-v21-0-release-announcement/)
- [Dash Core v22.0.0 release notes](https://github.com/dashpay/dash/blob/v22.0.0/doc/release-notes.md)
- [Dash Core v23.0.0 release tag](https://github.com/dashpay/dash/releases/tag/v23.0.0)
- [Dash Protocol Versions reference](https://docs.dash.org/projects/core/en/stable/docs/reference/p2p-network-protocol-versions.html)
- [Dash Evonode setup documentation](https://docs.dash.org/en/stable/docs/user/masternodes/setup-evonode.html)
