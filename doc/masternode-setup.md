# How to set up a LKSCOIN masternode

**LKSCOIN Core 5.18.2.1 — step by step, assuming nothing.**

This guide is written for someone who has never run a server. Every command is
written out in full. If a step does not do what this page says it should, stop
there and ask before continuing — going on after something unexpected is how
people lose time, not coins.

---

## Before you start

You need four things:

| | |
|---|---|
| **100,000 LKS** | plus a few extra for transaction fees, say 100,010 |
| **A small server** | 2 CPU, 4 GB RAM, 60 GB disk, Ubuntu 22.04, with a fixed public IP address |
| **A computer** | Windows, macOS or Linux, where you install the LKSCOIN wallet |
| **About two hours** | most of it spent waiting for downloads |

A server like that costs roughly 5-10 euro a month at any provider — Contabo,
Hetzner, DigitalOcean, OVH and many others are fine. It must stay switched on
day and night: a masternode that goes offline stops earning and, once quorums
are active on the network, starts collecting penalties.

### The one thing to understand before typing anything

A masternode is **two machines working together**:

- **Your computer** holds the 100,000 LKS and the keys that own the masternode.
  The coins never leave it.
- **The server** runs the software day and night. It holds one key, which lets
  it sign network messages on your behalf — and nothing else. If someone stole
  the server tomorrow, they could not touch your coins.

That separation is the whole design. Nothing in this guide ever asks you to put
your 100,000 LKS on the server.

### The keys, in plain words

| Key | Lives on | If you lose it |
|---|---|---|
| **Collateral** | your computer | you lose the 100,000 LKS |
| **Owner** | your computer | you can never change the masternode's settings again |
| **Operator (BLS)** | the server | you ask the owner key to issue a new one |
| **Voting** | your computer | you cannot vote on proposals |

Back up your wallet. Everything else is replaceable.

---

# PART 1 — Prepare the server

Connect to your server with SSH. On Windows use PuTTY or the built-in
`ssh` command in PowerShell; on macOS and Linux open a terminal.

```bash
ssh root@YOUR_SERVER_IP
```

## 1.1 Create a user for the node

Running a service as `root` is a bad habit. Create a dedicated user:

```bash
adduser --disabled-password --gecos "" lksnode
```

## 1.2 Install LKSCOIN Core

```bash
apt update
apt install -y wget
wget https://www.lkschain.io/downloads/LKSCoinCore_5.18.2.1.deb
sha256sum LKSCoinCore_5.18.2.1.deb
```

The command prints a long line of letters and numbers. It **must** be:

```
59ae7deda88e330ce76282ce0d75a0a9f3a65a2872f8343d67d5ae408bc6b465
```

If it is different, do not install the file: it was corrupted or tampered with
in transit. Download it again.

```bash
dpkg -i LKSCoinCore_5.18.2.1.deb
```

## 1.3 Write the configuration

```bash
mkdir -p /home/lksnode/.lkscore
nano /home/lksnode/.lkscore/lks.conf
```

Paste this, replacing `YOUR_SERVER_IP` with the public address of your server
and inventing a long random password:

```
daemon=1
server=1
listen=1
externalip=YOUR_SERVER_IP
rpcuser=lksrpc
rpcpassword=PUT_A_LONG_RANDOM_PASSWORD_HERE
rpcallowip=127.0.0.1
rpcbind=127.0.0.1
```

Save with `Ctrl+O`, then `Enter`, then exit with `Ctrl+X`.

> The RPC password is not something you will ever type again. It protects the
> node from other programs on the same server, so make it long and forget it.
> `openssl rand -hex 32` prints a good one.

```bash
chown -R lksnode:lksnode /home/lksnode/.lkscore
```

## 1.4 Open the network port

Masternodes talk to each other on port **9400**. If your server has a firewall:

```bash
ufw allow 9400/tcp
ufw allow OpenSSH
ufw --force enable
```

If your provider has its own firewall in the control panel, open 9400 there too.

## 1.5 Start the node and wait

```bash
su - lksnode -c "lksd"
```

The node now downloads the whole blockchain. **This takes a few hours.** Check
on it now and then:

```bash
su - lksnode -c "lks-cli getblockchaininfo" | grep -E 'blocks|verificationprogress'
```

Two things tell you it has finished:

- `verificationprogress` is very close to `1` (like `0.9999`);
- `blocks` matches the number shown at https://www.lkschain.io/blockchain.html

While it downloads, `lks-cli` may answer `error code: -28`. That means "still
loading, ask me later". It is normal.

Do not continue until the server is fully synchronised.

---

# PART 2 — Prepare your wallet

## 2.1 Install the wallet on your own computer

Download the LKSCOIN Core wallet for your system from
https://www.lkschain.io/network.html and install it. Open it and let it
synchronise — again, a few hours the first time.

## 2.2 Encrypt the wallet and back it up

In the wallet: **Settings → Encrypt Wallet**. Choose a passphrase you will not
forget, because there is no way to recover it.

Then **File → Backup Wallet** and save the file somewhere safe that is not this
computer. This file is your 100,000 LKS.

## 2.3 Get the coins into the wallet

Send your 100,000 LKS (plus a little extra for fees) to an address of this
wallet. Wait until the balance shows as confirmed.

---

# PART 3 — Generate the server's key

Back on the server:

```bash
su - lksnode -c "lks-cli bls generate"
```

It prints something like:

```json
{
  "secret": "3f2c...long string...",
  "public": "8a91...longer string..."
}
```

**Copy both somewhere safe right now.** You need `public` in Part 4 and `secret`
in Part 5. If you close the window without copying them, generate a new pair and
start again — no harm done, as long as you have not registered yet.

---

# PART 4 — Register the masternode

Everything in this part happens **in your wallet on your computer**, not on the
server. Open **Help → Debug window → Console**.

## 4.1 Create the addresses you need

Type each line and write down what it answers:

```
getnewaddress "mn-collateral"
getnewaddress "mn-owner"
getnewaddress "mn-payout"
```

You now have three addresses. They are used for:

- **collateral** — where the 100,000 LKS will sit, locked;
- **owner** — the key that controls the masternode's settings;
- **payout** — where your rewards will arrive.

Using three separate addresses is not required, but it keeps things clear.

## 4.2 Unlock the wallet

```
walletpassphrase "your passphrase" 300
```

That unlocks it for 300 seconds. If you take longer, run it again.

## 4.3 Register

This is the one long command. Type it on a single line, replacing the five
placeholders:

```
protx register_fund "COLLATERAL_ADDRESS" "YOUR_SERVER_IP:9400" "OWNER_ADDRESS" "BLS_PUBLIC_KEY" "OWNER_ADDRESS" 0 "PAYOUT_ADDRESS"
```

Reading it left to right, you are saying: *put the collateral here, my server is
at that address, this key owns the masternode, that key operates it, the same
owner key votes, the operator takes 0% of the rewards, and pay me there.*

Note that the owner address appears twice: the second time it is the voting key.
That is normal when you are both the owner and the voter.

If it works, it answers with a long transaction id. **Your 100,000 LKS are now
locked as collateral.** They are still yours and you can free them at any time
by spending them, but doing so immediately destroys the masternode.

If it complains instead, look at the message: it usually says exactly which
argument it did not like. Nothing has been sent, so you can correct and retry.

## 4.4 Find your masternode's identifier

```
protx list wallet
```

It prints one or more long identifiers. Yours is the new one — this is your
**proTxHash**. Write it down; it is how the network names your masternode.

---

# PART 5 — Tell the server it is a masternode

Back on the server:

```bash
su - lksnode -c "lks-cli stop"
sleep 20
nano /home/lksnode/.lkscore/lks.conf
```

Add one line at the end, using the **secret** from Part 3 — not the public one:

```
masternodeblsprivkey=THE_SECRET_FROM_PART_3
```

Save and exit, then start it again:

```bash
su - lksnode -c "lksd"
```

> From now on, this node has no wallet: adding that line disables it on purpose.
> The server does not need one, and cannot be robbed of what it does not hold.

---

# PART 6 — Check that it worked

Wait a few minutes, then on the server:

```bash
su - lksnode -c "lks-cli masternode status"
```

You are looking for two things:

```
"state": "READY"
"PoSePenalty": 0
```

`READY` means the network sees your masternode and it is doing its job.

If it says `WAITING_FOR_PROTX` your registration has not been confirmed yet —
wait for a few blocks. If it says something about the BLS key not matching, you
pasted the public key instead of the secret one, or the wrong pair entirely.

Then, from your wallet:

```
protx info YOUR_PROTXHASH
```

Check that `service` shows your server's address and port, and that
`PoSeBanHeight` is `-1` (meaning: not banned).

**That is it. Your masternode is running.**

---

# Living with a masternode

**Keep it running.** Rewards go to masternodes in turn, in a rotation that takes
a long while with the current network size. Patience is part of the arrangement.

**Never spend the collateral.** Moving those 100,000 LKS, even to another wallet
of your own, ends the masternode immediately and you would have to register
again from scratch.

**Never run two nodes with the same BLS key.** Two machines signing as the same
masternode is treated as misbehaviour and gets punished.

**Keep the software up to date.** Announcements go out on the project's
channels. Upgrades are usually: stop, install, start.

**Watch for penalties.** If `PoSePenalty` starts climbing, your node is being
seen as unreliable — usually the server was down, or the port is not reachable
from outside. Fix the cause and the penalty decays on its own.

---

# When something does not work

| What you see | What it means | What to do |
|---|---|---|
| `error code: -28` | the node is still loading | wait, it is not an error |
| `verificationprogress` stuck below 1 | still downloading the chain | wait; check the disk is not full with `df -h` |
| `WAITING_FOR_PROTX` | registration not confirmed yet | wait a few blocks |
| `POSE_BANNED` | the network stopped seeing your node | check the server is up and port 9400 is open, then ask the community — recovering needs one command from the owner key |
| `masternode status` says the BLS key is wrong | wrong key in `lks.conf` | it must be the **secret**, not the public one |
| Connections stay at 0 | the node cannot reach the network | check `ufw status` and your provider's firewall |
| `Insufficient funds` when registering | fees | you need slightly more than 100,000 LKS |

To see what the node is doing at any moment:

```bash
tail -f /home/lksnode/.lkscore/debug.log
```

Press `Ctrl+C` to stop watching.

---

# Why this matters, not just how

A masternode is not only a way to earn rewards. Masternodes form the quorums
that sign blocks, and those signatures are what make a proof recorded on this
chain hard to rewrite. The strength of that guarantee depends on the quorum
being spread across many independent people.

If a single organisation runs most of the masternodes, it can produce those
signatures on its own — and anyone examining the network can see that. The
value of what you are running comes precisely from the fact that you are not
them.

---

## Getting help

- Upgrade guide for existing masternodes: [`upgrade-guide-masternodes.md`](upgrade-guide-masternodes.md)
- Release notes: [`release-notes.md`](release-notes.md)
- Block explorer: https://www.lkschain.io/blockchain.html
- Source code and issues: https://github.com/LKSCOIN/lkscoin-core

When you ask for help, include the output of `lks-cli masternode status` and the
last twenty lines of `debug.log`. **Never paste your `lks.conf`**, and never
paste anything containing `masternodeblsprivkey`, `rpcpassword` or a private
key — filter it first:

```bash
grep -v -iE 'blsprivkey|rpcpassword|rpcuser' /home/lksnode/.lkscore/debug.log | tail -20
```
