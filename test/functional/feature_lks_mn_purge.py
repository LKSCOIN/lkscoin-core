#!/usr/bin/env python3
# Copyright (c) 2026 The Lksc Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Exercise the LKSCOIN masternode purge.

Background: the LKSCOIN LLMQ subsystem has produced no quorum since 2021.
Quorum members are drawn from the whole deterministic masternode list, and the
overwhelming majority of the registered entries are long offline, so no DKG can
reach its minimum size. Because PoSe penalties are applied by quorums, the
protocol cannot evict those entries by itself.

CDeterministicMNManager::PurgeInactiveMNs() breaks that deadlock: a public
re-registration window is announced between two block heights, and at the purge
height every masternode that published no provider special transaction inside
the window is removed from the list. Collateral is never touched.

This test drives the whole cycle on regtest:

  1. set up masternodes and note the height,
  2. restart every node with a purge window starting a few blocks ahead,
  3. have half of them prove liveness with `protx update_service`,
  4. mine past the purge height,
  5. assert that exactly the silent ones are gone, that every node agrees,
     that the collateral of a removed masternode is untouched, and that its
     operator can register again.
"""

from test_framework.test_framework import LksTestFramework
from test_framework.util import assert_equal, force_finish_mnsync, p2p_port


class MasternodePurgeTest(LksTestFramework):
    def set_test_params(self):
        # 2 plain nodes + 4 masternodes: two will prove liveness, two will not.
        self.set_lks_test_params(6, 4, fast_dip3_enforcement=True)

    def restart_all_with(self, extra):
        """Restart every node adding `extra`, preserving each node's own args
        (masternodes need their BLS operator key back)."""
        mn_by_idx = {mn.nodeIdx: mn for mn in self.mninfo}
        for i in range(len(self.nodes)):
            args = list(self.extra_args[i]) + extra
            if i in mn_by_idx:
                args = ['-masternodeblsprivkey=%s' % mn_by_idx[i].keyOperator] + args
            self.restart_node(i, args)
        for i in range(1, len(self.nodes)):
            self.connect_nodes(i, 0)
        for mn in self.mninfo:
            mn.node = self.nodes[mn.nodeIdx]
            force_finish_mnsync(mn.node)
        self.sync_all()

    def run_test(self):
        node = self.nodes[0]
        force_finish_mnsync(node)

        all_protx = [mn.proTxHash for mn in self.mninfo]
        assert_equal(len(all_protx), 4)
        self.log.info("Masternodes registered: %s" % ", ".join(h[:8] for h in all_protx))
        assert_equal(len(node.protx('list', 'valid')), 4)

        # --- 1. schedule the window a few blocks ahead --------------------
        height = node.getblockcount()
        window_start = height + 5
        purge_height = height + 20
        self.log.info("Height %d: window %d..%d, purge at %d"
                      % (height, window_start, purge_height, purge_height))
        self.restart_all_with(["-mnpurgeparams=%d:%d" % (window_start, purge_height)])

        # Anything published before the window must not count as proof of life.
        node.generate(window_start - node.getblockcount())
        self.sync_all()
        assert_equal(node.getblockcount(), window_start)

        # --- 2. only the first two prove liveness -------------------------
        survivors = all_protx[:2]
        doomed = all_protx[2:]

        for mn in self.mninfo[:2]:
            node.protx('update_service', mn.proTxHash,
                       '127.0.0.1:%d' % p2p_port(mn.nodeIdx), mn.keyOperator,
                       "", node.getnewaddress())
        node.generate(1)
        self.sync_all()
        self.log.info("Proof of life published by %s" % ", ".join(h[:8] for h in survivors))
        assert_equal(len(node.protx('list', 'valid')), 4)

        # --- 3. cross the purge height ------------------------------------
        node.generate(purge_height - node.getblockcount() - 1)
        self.sync_all()
        assert_equal(node.getblockcount(), purge_height - 1)
        assert_equal(len(node.protx('list', 'valid')), 4)
        self.log.info("One block before the purge the list is still complete")

        node.generate(1)
        self.sync_all()
        assert_equal(node.getblockcount(), purge_height)

        # --- 4. exactly the silent ones are gone --------------------------
        remaining = set(node.protx('list', 'valid'))
        for protx in survivors:
            assert protx in remaining, "masternode %s proved liveness but was removed" % protx
        for protx in doomed:
            assert protx not in remaining, "masternode %s was silent but survived" % protx
        assert_equal(len(remaining), len(survivors))
        self.log.info("Purge removed %d silent masternodes and kept %d"
                      % (len(doomed), len(survivors)))

        # The purge is a consensus rule: every node must reach the same list.
        for n in self.nodes[1:]:
            assert_equal(set(n.protx('list', 'valid')), remaining)
        self.log.info("All %d nodes agree on the purged list" % len(self.nodes))

        # --- 5. the collateral of a removed masternode is untouched -------
        # mninfo[3] was registered with an external collateral (odd index).
        removed = self.mninfo[3]
        coin = node.gettxout(removed.collateral_txid, removed.collateral_vout)
        assert coin is not None, "collateral of a removed masternode was destroyed"
        assert_equal(coin['value'], 100000)
        self.log.info("Collateral of the removed masternode is still unspent")

        # --- 6. a removed operator can register again ---------------------
        node.protx('register', removed.collateral_txid, removed.collateral_vout,
                   '127.0.0.1:%d' % p2p_port(removed.nodeIdx), removed.ownerAddr,
                   removed.pubKeyOperator, removed.votingAddr, 0,
                   removed.collateral_address, node.getnewaddress())
        node.generate(1)
        self.sync_all()
        assert_equal(len(node.protx('list', 'valid')), len(survivors) + 1)
        self.log.info("A removed operator was able to register again")


if __name__ == '__main__':
    MasternodePurgeTest().main()
