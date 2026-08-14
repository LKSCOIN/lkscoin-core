#!/usr/bin/env bash
# LKSCOIN masternode health check.
#
# Run on an upgraded masternode (as the user that owns the data directory):
#   bash mn-check.sh
#
# Optional: compare against a legacy (3.x) control node over SSH:
#   LEGACY_NODE=user@1.2.3.4 bash mn-check.sh
#
# Intended to be run periodically (e.g. hourly via cron, appending to a log)
# during the validation window described in the LKSCOIN 2.0 roadmap, Fase A.

CLI=${CLI:-lks-cli}
DATADIR_ARG=${DATADIR:+-datadir=$DATADIR}
Q() { $CLI $DATADIR_ARG "$@" 2>/dev/null; }

ok()   { echo "  [ OK ]  $*"; }
warn() { echo "  [WARN]  $*"; }
bad()  { echo "  [FAIL]  $*"; }

echo "=== LKSCOIN masternode check - $(date -u '+%Y-%m-%d %H:%M:%S UTC') ==="

# --- 1. Client and sync ------------------------------------------------------
SUBVER=$(Q getnetworkinfo | grep -o '"subversion": *"[^"]*"' | cut -d'"' -f4)
BLOCKS=$(Q getblockcount)
HEADERS=$(Q getblockchaininfo | grep -o '"headers": *[0-9]*' | grep -o '[0-9]*')
BESTHASH=$(Q getbestblockhash)
PEERS=$(Q getpeerinfo | grep -c '"addr"')
MNSYNC=$(Q mnsync status | grep -o '"AssetName": *"[^"]*"' | cut -d'"' -f4)

echo
echo "Client:      $SUBVER"
echo "Height:      $BLOCKS (headers: $HEADERS)"
echo "Best hash:   $BESTHASH"
echo "Peers:       $PEERS"
echo "MN sync:     $MNSYNC"
echo

[ -n "$BLOCKS" ] || { bad "node not responding"; exit 1; }
[ "$BLOCKS" = "$HEADERS" ] && ok "chain tip reached" || warn "behind tip by $((HEADERS-BLOCKS)) blocks"
[ "$MNSYNC" = "MASTERNODE_SYNC_FINISHED" ] && ok "masternode sync finished" || warn "mnsync: $MNSYNC"
[ "$PEERS" -ge 3 ] 2>/dev/null && ok "peer count $PEERS" || warn "only $PEERS peers"

# --- 2. Masternode identity and PoSe ----------------------------------------
STATE=$(Q masternode status | grep -o '"state": *"[^"]*"' | cut -d'"' -f4)
PROTX=$(Q masternode status | grep -o '"proTxHash": *"[^"]*"' | cut -d'"' -f4)
POSE=$(Q masternode status | grep -o '"PoSePenalty": *[0-9-]*' | grep -o '[0-9-]*$')
POSEBAN=$(Q masternode status | grep -o '"PoSeBanHeight": *[0-9-]*' | grep -o '[0-9-]*$')
LASTPAID=$(Q masternode status | grep -o '"lastPaidHeight": *[0-9]*' | grep -o '[0-9]*$')

echo
echo "ProTx:       $PROTX"
echo "State:       $STATE"
echo "PoSePenalty: $POSE   PoSeBanHeight: $POSEBAN"
echo "Last paid:   block $LASTPAID  (now: $BLOCKS, delta: $((BLOCKS-LASTPAID)))"
echo

[ "$STATE" = "READY" ] && ok "masternode READY" || bad "masternode state: $STATE"
[ "$POSE" = "0" ] && ok "no PoSe penalty" || bad "PoSe penalty is $POSE"
[ "$POSEBAN" = "-1" ] && ok "never PoSe banned" || bad "PoSe banned at height $POSEBAN"

# Expected payment interval ~= number of enabled masternodes (in blocks).
ENABLED=$(Q masternode count | grep -o '"enabled": *[0-9]*' | grep -o '[0-9]*$')
if [ -n "$ENABLED" ] && [ -n "$LASTPAID" ]; then
  DELTA=$((BLOCKS-LASTPAID))
  echo "  (enabled masternodes: $ENABLED - expected payment interval ~$ENABLED blocks)"
  if [ "$DELTA" -gt $((ENABLED*2)) ]; then
    warn "not paid for ${DELTA} blocks, over twice the expected interval - investigate"
  else
    ok "payment interval within expected range"
  fi
fi

# --- 3. Quorums and ChainLocks ----------------------------------------------
echo
MEMBEROF=$(Q quorum memberof "$PROTX" | grep -c '"quorumHash"')
CL=$(Q getbestchainlock 2>/dev/null | grep -o '"height": *[0-9]*' | grep -o '[0-9]*$')

echo "Quorum memberships: ${MEMBEROF:-0}"
echo "Best ChainLock:     ${CL:-none}"
echo

if [ -n "$CL" ]; then
  LAG=$((BLOCKS-CL))
  [ "$LAG" -le 5 ] && ok "ChainLock current (lag $LAG blocks)" || warn "ChainLock lagging $LAG blocks"
else
  warn "no ChainLock seen - check whether the network is producing them"
fi

# --- 4. Log scan -------------------------------------------------------------
LOG=${LOG:-$HOME/.lkscore/debug.log}
if [ -f "$LOG" ]; then
  echo
  echo "Log scan (last 2000 lines of $LOG):"
  for pat in "Misbehaving" "bad-cb-payee" "ConnectBlock.*failed" "InvalidChainFound" "ERROR"; do
    n=$(tail -2000 "$LOG" | grep -c -E "$pat")
    if [ "$n" -gt 0 ]; then warn "$n x '$pat'"; else ok "no '$pat'"; fi
  done
fi

# --- 5. Comparison with a legacy control node --------------------------------
if [ -n "$LEGACY_NODE" ]; then
  echo
  echo "Comparing with legacy node $LEGACY_NODE:"
  LH=$(ssh -o ConnectTimeout=10 "$LEGACY_NODE" 'lks-cli getblockcount' 2>/dev/null)
  LHASH=$(ssh -o ConnectTimeout=10 "$LEGACY_NODE" "lks-cli getblockhash $BLOCKS" 2>/dev/null)
  echo "  legacy height: ${LH:-unreachable}"
  if [ -n "$LHASH" ]; then
    [ "$LHASH" = "$BESTHASH" ] && ok "block $BLOCKS hash identical on both clients" \
                               || bad "CONSENSUS DIVERGENCE at height $BLOCKS!"
  fi
fi

# --- 6. Resources ------------------------------------------------------------
echo
PID=$(pgrep -x lksd | head -1)
if [ -n "$PID" ]; then
  RSS=$(ps -o rss= -p "$PID" | tr -d ' ')
  ET=$(ps -o etime= -p "$PID" | tr -d ' ')
  echo "Process:     pid $PID, uptime $ET, RSS $((RSS/1024)) MB"
fi
df -h "$(dirname "$LOG")" 2>/dev/null | tail -1 | awk '{print "Disk:        "$4" free ("$5" used)"}'

echo
echo "=== end of check ==="
