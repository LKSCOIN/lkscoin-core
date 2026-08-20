#!/usr/bin/env bash
# Copyright (c) 2018-2020 The Lksc Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
# use testnet settings,  if you need mainnet,  use ~/.lkscore/lksd.pid file instead
export LC_ALL=C

lks_pid=$(<~/.lkscore/testnet3/lksd.pid)
sudo gdb -batch -ex "source debug.gdb" lksd ${lks_pid}
