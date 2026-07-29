#!/usr/bin/env bash
# use testnet settings,  if you need mainnet,  use ~/.lkscore/lksd.pid file instead
export LC_ALL=C

lks_pid=$(<~/.lkscore/testnet3/lksd.pid)
sudo gdb -batch -ex "source debug.gdb" lksd ${lks_pid}
