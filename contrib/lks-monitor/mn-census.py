#!/usr/bin/env python3
"""
LKSCOIN masternode census.

Reads the deterministic masternode list from a local node and, for every
registered masternode, opens a P2P connection and performs the version
handshake. This yields three things the RPC list alone cannot tell you:

  1. how many masternodes are actually reachable (the list marks nodes
     "ENABLED" even when they are long dead, because PoSe scoring depends on
     quorums that are currently not forming);
  2. which client version each one runs;
  3. the block height each one reports.

Usage:
    python3 mn-census.py                      # uses `lks-cli`
    python3 mn-census.py --cli /bin/lks-cli --datadir ~/.lkscore
    python3 mn-census.py --timeout 8 --workers 60 --csv census.csv

Output: a summary on stdout and, optionally, a CSV for further analysis.
No wallet access and no write operations: this is a read-only measurement.
"""

import argparse
import csv
import hashlib
import json
import socket
import struct
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor

# LKSCOIN mainnet message start bytes (src/chainparams.cpp: pchMessageStart)
MAGIC = bytes([0xbf, 0x0c, 0x6b, 0xbd])
DEFAULT_PORT = 9400
PROTOCOL_VERSION = 70219


def dsha256(b):
    return hashlib.sha256(hashlib.sha256(b).digest()).digest()


def msg(command, payload):
    """Wrap a payload in a P2P message header."""
    cmd = command.encode() + b"\x00" * (12 - len(command))
    return MAGIC + cmd + struct.pack("<I", len(payload)) + dsha256(payload)[:4] + payload


def varstr(s):
    b = s.encode()
    return bytes([len(b)]) + b


def netaddr(ip, port):
    """26-byte network address: services(8) + IPv6 (or IPv4-mapped) + port(2 BE)."""
    try:
        addr = b"\x00" * 10 + b"\xff\xff" + socket.inet_aton(ip)
    except OSError:
        try:
            addr = socket.inet_pton(socket.AF_INET6, ip)
        except OSError:
            addr = b"\x00" * 16
    return struct.pack("<Q", 0) + addr + struct.pack(">H", port)


def version_payload(ip, port):
    p = struct.pack("<i", PROTOCOL_VERSION)
    p += struct.pack("<Q", 0)                       # services
    p += struct.pack("<q", int(time.time()))        # timestamp
    p += netaddr(ip, port)                          # addr_recv
    p += netaddr("0.0.0.0", 0)                      # addr_from
    p += struct.pack("<Q", 0x1122334455667788)      # nonce
    p += varstr("/lks-census:0.1/")                 # user agent
    p += struct.pack("<i", 0)                       # start height
    p += b"\x00"                                    # relay
    return p


def read_exactly(sock, n):
    buf = b""
    while len(buf) < n:
        chunk = sock.recv(n - len(buf))
        if not chunk:
            raise EOFError("connection closed")
        buf += chunk
    return buf


def read_message(sock):
    header = read_exactly(sock, 24)
    if header[:4] != MAGIC:
        raise ValueError("wrong network magic")
    command = header[4:16].rstrip(b"\x00").decode(errors="replace")
    length = struct.unpack("<I", header[16:20])[0]
    if length > 4_000_000:
        raise ValueError("payload too large")
    payload = read_exactly(sock, length) if length else b""
    return command, payload


def parse_version(payload):
    """Extract protocol version, user agent and reported height."""
    off = 0
    version = struct.unpack_from("<i", payload, off)[0]; off += 4
    off += 8 + 8            # services + timestamp
    off += 26 + 26          # addr_recv + addr_from
    off += 8                # nonce
    ualen = payload[off]; off += 1
    if ualen >= 0xfd:       # varint > 252: not expected for user agents
        return version, "?", 0
    ua = payload[off:off + ualen].decode(errors="replace"); off += ualen
    height = struct.unpack_from("<i", payload, off)[0] if off + 4 <= len(payload) else 0
    return version, ua, height


def split_address(address):
    """Split "1.2.3.4:9400" or "[2001:db8::1]:9400" into (host, port).
    Returns (None, None) for unset/invalid entries such as "[::]:0"."""
    address = (address or "").strip()
    if address.startswith("["):                       # bracketed IPv6
        host, sep, rest = address[1:].partition("]")
        port = rest[1:] if rest.startswith(":") else ""
    elif address.count(":") > 1:                      # bare IPv6, no port
        host, port = address, ""
    else:
        host, _, port = address.partition(":")
    try:
        port = int(port) if port else DEFAULT_PORT
    except ValueError:
        return None, None
    if not host or host in ("::", "0.0.0.0") or not 0 < port < 65536:
        return None, None
    return host, port


def probe(address, timeout):
    """Connect to a masternode and complete enough of the handshake to learn
    its version. Returns a dict describing the outcome."""
    host, port = split_address(address)
    res = {"address": address, "reachable": False, "protocol": "",
           "subversion": "", "height": "", "error": ""}
    if host is None:
        res["error"] = "NoAddress"
        return res
    sock = None
    try:
        sock = socket.create_connection((host, port), timeout=timeout)
        sock.settimeout(timeout)
        sock.sendall(msg("version", version_payload(host, port)))
        deadline = time.time() + timeout
        while time.time() < deadline:
            command, payload = read_message(sock)
            if command == "version":
                v, ua, h = parse_version(payload)
                res.update(reachable=True, protocol=v, subversion=ua, height=h)
                return res
        res["error"] = "no version message"
    except Exception as e:                                   # noqa: BLE001
        res["error"] = type(e).__name__ + (f": {e}" if str(e) else "")
    finally:
        if sock:
            try:
                sock.close()
            except OSError:
                pass
    return res


def load_masternodes(cli, datadir):
    cmd = [cli]
    if datadir:
        cmd.append(f"-datadir={datadir}")
    cmd += ["masternodelist", "json"]
    out = subprocess.check_output(cmd, timeout=60)
    data = json.loads(out)
    nodes = []
    for outpoint, mn in data.items():
        nodes.append({
            "outpoint": outpoint,
            "proTxHash": mn.get("proTxHash", ""),
            "address": mn.get("address", ""),
            "status": mn.get("status", ""),
            "lastpaidblock": mn.get("lastpaidblock", 0),
            "payee": mn.get("payee", ""),
        })
    return nodes


def main():
    ap = argparse.ArgumentParser(description="LKSCOIN masternode census")
    ap.add_argument("--cli", default="lks-cli")
    ap.add_argument("--datadir", default="")
    ap.add_argument("--timeout", type=float, default=6.0)
    ap.add_argument("--workers", type=int, default=50)
    ap.add_argument("--csv", default="")
    args = ap.parse_args()

    print("Reading masternode list ...", file=sys.stderr)
    nodes = load_masternodes(args.cli, args.datadir)
    print(f"{len(nodes)} masternodes registered. Probing on port 9400 "
          f"({args.workers} workers, {args.timeout}s timeout) ...", file=sys.stderr)

    started = time.time()
    with ThreadPoolExecutor(max_workers=args.workers) as pool:
        results = list(pool.map(lambda n: probe(n["address"], args.timeout), nodes))
    for node, res in zip(nodes, results):
        node.update(res)
    elapsed = time.time() - started

    total = len(nodes)
    enabled = sum(1 for n in nodes if n["status"] == "ENABLED")
    reachable = sum(1 for n in nodes if n["reachable"])
    reach_enabled = sum(1 for n in nodes if n["reachable"] and n["status"] == "ENABLED")

    print()
    print("=" * 62)
    print(f"  LKSCOIN masternode census - {time.strftime('%Y-%m-%d %H:%M:%S UTC', time.gmtime())}")
    print("=" * 62)
    print(f"  Registered in the deterministic list : {total}")
    print(f"  Marked ENABLED by the list           : {enabled}")
    print(f"  Actually reachable on the P2P port   : {reachable}"
          f"  ({100.0 * reachable / total:.1f}% of registered)")
    print(f"  ENABLED *and* reachable              : {reach_enabled}"
          f"  ({100.0 * reach_enabled / enabled:.1f}% of ENABLED)" if enabled else "")
    print(f"  Probe time                           : {elapsed:.0f}s")
    print()

    versions = {}
    for n in nodes:
        if n["reachable"]:
            versions[n["subversion"]] = versions.get(n["subversion"], 0) + 1
    if versions:
        print("  Client versions among reachable masternodes:")
        for ua, count in sorted(versions.items(), key=lambda kv: -kv[1]):
            print(f"    {count:5d}  {ua}")
        print()

    heights = [n["height"] for n in nodes if n["reachable"] and isinstance(n["height"], int) and n["height"] > 0]
    if heights:
        heights.sort()
        print(f"  Reported heights: min {heights[0]}, median {heights[len(heights)//2]}, max {heights[-1]}")
        print()

    errors = {}
    for n in nodes:
        if not n["reachable"]:
            key = n["error"].split(":")[0]
            errors[key] = errors.get(key, 0) + 1
    if errors:
        print("  Failure reasons:")
        for err, count in sorted(errors.items(), key=lambda kv: -kv[1]):
            print(f"    {count:5d}  {err}")
        print()

    print("  Why this matters: quorum members are drawn from the *whole* list.")
    print("  A DKG can only complete if enough of the drawn members are alive,")
    print("  so the percentage above - not the absolute count - is what decides")
    print("  whether LLMQs (and therefore ChainLocks) can be restored.")
    print("=" * 62)

    if args.csv:
        with open(args.csv, "w", newline="") as fh:
            w = csv.DictWriter(fh, fieldnames=[
                "proTxHash", "address", "status", "reachable", "protocol",
                "subversion", "height", "lastpaidblock", "payee", "error"])
            w.writeheader()
            for n in nodes:
                w.writerow({k: n.get(k, "") for k in w.fieldnames})
        print(f"\nCSV written to {args.csv}", file=sys.stderr)


if __name__ == "__main__":
    main()
