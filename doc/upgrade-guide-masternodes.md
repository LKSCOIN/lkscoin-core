# LKSCOIN Core 5.18.2.1 — Masternode upgrade guide

*(English below / versione inglese in fondo)*

---

## Italiano

### In breve

L'aggiornamento richiede tre comandi. **Non serve reindicizzare la blockchain,
non serve rifare la registrazione del masternode** e non va modificato nulla
nella configurazione: stesso `lks.conf`, stessa chiave BLS operatore, stessa
ProTx, stesso collaterale.

Questa versione **non cambia le regole di consenso e non attiva alcun hard
fork**: i nodi 5.18.2.1, 4.17.x e 3.x convivono sulla stessa rete, quindi ognuno
può aggiornare quando preferisce.

Una sola avvertenza importante, spiegata al punto 4: **il primo avvio dopo
l'aggiornamento può durare fino a un'ora** su VPS a singola CPU.

### 1. Prima di iniziare — fotografa lo stato

```bash
lksd --version | head -1
lks-cli masternode status
lks-cli getblockcount
```

Annota `proTxHash` e verifica che lo stato sia `READY` con `PoSePenalty: 0`.
Se il tuo masternode sta per ricevere un pagamento, conviene aspettare che il
pagamento avvenga prima di riavviare.

Verifica anche come è gestito il demone:

```bash
systemctl list-units --type=service | grep -i lks
```

Se compare un servizio (di solito `lksd.service`), usa la procedura systemd.

### 2. Backup

```bash
cp ~/.lkscore/lks.conf ~/lks.conf.backup
cp ~/.lkscore/wallet.dat ~/wallet.dat.backup-$(date +%F) 2>/dev/null
dpkg -l | grep lkscoincore        # annota la versione attuale, per il rollback
```

### 3. Aggiornamento

**Con systemd** (da root; il servizio continuerà a girare con l'utente
configurato, di norma un utente dedicato tipo `nodemaster`):

```bash
systemctl stop lksd
sleep 20
dpkg -i LKSCoinCore_5.18.2.1.deb
systemctl start lksd
```

**Senza systemd** (come utente proprietario della datadir, **mai come root**):

```bash
lks-cli stop
sleep 20
sudo dpkg -i LKSCoinCore_5.18.2.1.deb
lksd -daemon
```

> Attenzione: `lksd` e `lks-cli` cercano la cartella dati nella home
> dell'utente che li lancia. Se il demone gira con un utente dedicato, esegui
> questi comandi con quell'utente (`su - nodemaster`), altrimenti creeresti file
> di proprietà di root nella datadir e il nodo non ripartirebbe.

Aspetta che il processo sia davvero terminato prima di installare il pacchetto
(`ps aux | grep [l]ksd` deve essere vuoto). Non usare mai `kill -9`: il demone
sta scrivendo il database su disco.

### 4. Il primo avvio è lento — è normale

Dash 18 ha suddiviso il vecchio database `~/.lkscore/llmq` in tre database
separati (`llmq/dkgdb`, `llmq/recsigdb`, `llmq/isdb`) e converte il formato del
`txindex`. Entrambe le operazioni avvengono **una sola volta**, al primo avvio
dopo l'aggiornamento, e la migrazione LLMQ non scrive nulla nel log ordinario.

Il risultato è che per parecchi minuti il nodo sembra bloccato:

```
$ lks-cli getblockcount
error code: -28
error message:
Loading block index...
```

Su un VPS a singola CPU abbiamo misurato **46 minuti**; su macchine più veloci
sono pochi minuti. In quell'intervallo il masternode risulta offline.

- **Non interrompere il processo** e non riavviare il servizio: una migrazione
  interrotta a metà lascia i database incoerenti e costringe a una
  reindicizzazione completa.
- Per seguire l'avanzamento, avvia il demone con `-debug=llmq`.
- I riavvii successivi tornano a pochi secondi.

Nei nostri test un masternode rimasto offline circa 50 minuti non ha subito
alcuna penalità PoSe, ma conviene comunque programmare l'aggiornamento in un
momento tranquillo e non aggiornare tutti i propri nodi nello stesso istante.

Il momento in cui il nodo è pronto si riconosce così:

```bash
grep 'init message: Done loading' ~/.lkscore/debug.log | tail -1
```

### 5. Verifica

```bash
lks-cli getnetworkinfo | grep -E 'subversion|protocolversion'   # 5.18.2.1, 70224
lks-cli masternode status                    # state: READY, PoSePenalty: 0
lks-cli mnsync status | grep AssetName       # MASTERNODE_SYNC_FINISHED
lks-cli getblockcount
```

Controlla **`getnetworkinfo`**, non solo `lksd --version`: un processo già
avviato continua a eseguire il vecchio binario anche dopo che il file è stato
sostituito, e su alcune macchine un supervisore può riavviare `lksd`
automaticamente.

Subito dopo il riavvio `mnsync` può mostrare `MASTERNODE_SYNC_BLOCKCHAIN`: è
normale, entro qualche minuto deve arrivare a `MASTERNODE_SYNC_FINISHED`. Se
resta bloccato lì e `getnetworkinfo` mostra `outboundconnections: 0`, controlla
di non avere righe `connect=` in `lks.conf`, che disabilitano le connessioni
automatiche.

### 6. Nelle 24-48 ore successive

```bash
lks-cli protx info <proTxHash> | grep -i pose
```

`PoSePenalty` deve restare `0` e `PoSeBanHeight` deve essere `-1`.

### Rollback

Se qualcosa non va, si torna indietro reinstallando il pacchetto precedente: le
regole di consenso non sono cambiate e il formato della cartella dati resta
leggibile dalle versioni 4.17.x.

```bash
systemctl stop lksd
dpkg -i LKSCoinCore_4.17.3.2.deb
systemctl start lksd
```

L'unica cosa che non torna indietro è la migrazione dei database LLMQ: il
vecchio `~/.lkscore/llmq` viene svuotato. Contiene solo cache (firme recuperate
e contributi DKG), quindi la 4.17 riparte comunque, ricostruendole quando serve.

### Note

- **Perché aggiornare.** Oggi i masternode 4.17 e 5.18 convivono senza problemi
  perché sulla rete non si forma alcun quorum. Ma 18.x richiede il protocollo
  70221 o superiore per partecipare ai DKG, e la 4.17 parla 70219: quando i
  quorum verranno riattivati, un masternode rimasto alla 4.17 sarebbe trattato
  come assente e accumulerebbe penalità PoSe. Tutti i masternode devono essere
  su 5.18.x **prima** di quel momento.
- **Sentinel** serve ancora con questa versione e continua a funzionare senza
  modifiche.
- Il pacchetto `.deb` è compilato su Ubuntu 18.04 e funziona su 18.04, 20.04,
  22.04, 24.04 e Debian equivalenti.
- Non incollare mai in chat, forum o issue il contenuto di `lks.conf` o le righe
  di `debug.log` che contengono `masternodeblsprivkey`, `rpcuser` o
  `rpcpassword`. Filtra con:
  `grep -v -iE 'blsprivkey|rpcpassword|rpcuser' debug.log`

---

## English

### In short

The upgrade takes three commands. **No reindex is required, no masternode
re-registration is needed**, and nothing in your configuration changes: same
`lks.conf`, same BLS operator key, same ProTx, same collateral.

This release **changes no consensus rules and activates no hard fork**: 5.18.2.1,
4.17.x and 3.x nodes interoperate on the same network, so you can upgrade
whenever you prefer.

One important caveat, explained in section 4: **the first start after the
upgrade can take up to an hour** on single-CPU VPS instances.

### 1. Before you start — record the current state

```bash
lksd --version | head -1
lks-cli masternode status
lks-cli getblockcount
systemctl list-units --type=service | grep -i lks
```

Note your `proTxHash` and check the state is `READY` with `PoSePenalty: 0`. If a
payment is imminent, wait for it before restarting.

### 2. Backup

```bash
cp ~/.lkscore/lks.conf ~/lks.conf.backup
cp ~/.lkscore/wallet.dat ~/wallet.dat.backup-$(date +%F) 2>/dev/null
dpkg -l | grep lkscoincore
```

### 3. Upgrade

**With systemd** (as root; the service keeps running as its configured user):

```bash
systemctl stop lksd
sleep 20
dpkg -i LKSCoinCore_5.18.2.1.deb
systemctl start lksd
```

**Without systemd** (as the user owning the data directory, **never as root**):

```bash
lks-cli stop
sleep 20
sudo dpkg -i LKSCoinCore_5.18.2.1.deb
lksd -daemon
```

> `lksd` and `lks-cli` look for the data directory in the home of the user
> running them. If the daemon runs under a dedicated user, run these commands as
> that user (`su - nodemaster`), otherwise you will create root-owned files in
> the data directory and the node will fail to start.

Make sure the process is really gone (`ps aux | grep [l]ksd`) before installing
the package, and never use `kill -9`: the daemon is flushing its databases.

### 4. The first start is slow — this is expected

Dash 18 split the old `~/.lkscore/llmq` database into three separate databases
(`llmq/dkgdb`, `llmq/recsigdb`, `llmq/isdb`) and converts the `txindex` format.
Both happen **once**, on the first start after the upgrade, and the LLMQ
migration writes nothing to the ordinary log.

So for a long while the node looks stuck:

```
$ lks-cli getblockcount
error code: -28
error message:
Loading block index...
```

We measured **46 minutes** on a single-CPU VPS; on faster machines it is a few
minutes. During that window the masternode is offline.

- **Do not interrupt it** and do not restart the service: an interrupted
  migration leaves the databases inconsistent and forces a full reindex.
- Start with `-debug=llmq` if you want to watch the progress.
- Subsequent restarts are back to a few seconds.

In our tests a masternode offline for about 50 minutes took no PoSe penalty, but
schedule the upgrade at a quiet time anyway, and do not upgrade all of your nodes
at the same moment.

You can tell when the node is ready with:

```bash
grep 'init message: Done loading' ~/.lkscore/debug.log | tail -1
```

### 5. Verify

```bash
lks-cli getnetworkinfo | grep -E 'subversion|protocolversion'   # 5.18.2.1, 70224
lks-cli masternode status                    # state: READY, PoSePenalty: 0
lks-cli mnsync status | grep AssetName       # MASTERNODE_SYNC_FINISHED
```

Check `getnetworkinfo`, not just `lksd --version`: a running process keeps
executing the old binary image after the file has been replaced, and some setups
restart `lksd` automatically.

Right after the restart `mnsync` may report `MASTERNODE_SYNC_BLOCKCHAIN`; within
a few minutes it must reach `MASTERNODE_SYNC_FINISHED`. If it stays there and
`getnetworkinfo` shows `outboundconnections: 0`, check that `lks.conf` has no
`connect=` lines, which disable automatic connections.

### 6. Over the next 24-48 hours

```bash
lks-cli protx info <proTxHash> | grep -i pose
```

`PoSePenalty` must stay `0` and `PoSeBanHeight` must remain `-1`.

### Rollback

Reinstall the previous package; consensus rules are unchanged and the data
directory stays readable by 4.17.x.

```bash
systemctl stop lksd
dpkg -i LKSCoinCore_4.17.3.2.deb
systemctl start lksd
```

The only one-way step is the LLMQ database migration: the old `~/.lkscore/llmq`
is wiped. It holds caches only (recovered signatures and DKG contributions), so
4.17 starts fine and rebuilds them as needed.

### Notes

- **Why upgrade.** 4.17 and 5.18 masternodes coexist today only because no
  quorum forms on the network. But 18.x requires protocol 70221 or later to take
  part in a DKG, and 4.17 speaks 70219: once quorums are switched back on, a
  masternode left on 4.17 would count as absent and would accumulate PoSe
  penalties. Every masternode must be on 5.18.x **before** that happens.
- **Sentinel** is still required by this release and keeps working unchanged.
- The `.deb` is built on Ubuntu 18.04 and runs on 18.04, 20.04, 22.04, 24.04 and
  equivalent Debian releases.
- Never paste `lks.conf` or `debug.log` lines containing `masternodeblsprivkey`,
  `rpcuser` or `rpcpassword` into a chat, forum or issue. Filter them out with:
  `grep -v -iE 'blsprivkey|rpcpassword|rpcuser' debug.log`
