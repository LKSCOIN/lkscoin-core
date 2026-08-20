# LKSCOIN Core 4.17.3.2 — Masternode upgrade guide

*(English below / versione inglese in fondo)*

---

## Italiano

### In breve

L'aggiornamento richiede tre comandi e meno di un minuto di fermo. **Non serve
reindicizzare la blockchain, non serve rifare la registrazione del masternode**
e non va modificato nulla nella configurazione: stesso `lks.conf`, stessa chiave
BLS operatore, stessa ProTx, stesso collaterale.

Questa versione **non cambia le regole di consenso e non attiva alcun hard
fork**: i nodi 4.17.3.2 e quelli 3.x convivono sulla stessa rete, quindi ognuno
può aggiornare quando preferisce.

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
sleep 10
dpkg -i LKSCoinCore_4.17.3.2.deb
systemctl start lksd
```

**Senza systemd** (come utente proprietario della datadir, **mai come root**):

```bash
lks-cli stop
sleep 15
sudo dpkg -i LKSCoinCore_4.17.3.2.deb
lksd -daemon
```

> Attenzione: `lksd` e `lks-cli` cercano la cartella dati nella home
> dell'utente che li lancia. Se il demone gira con un utente dedicato, esegui
> questi comandi con quell'utente (`su - nodemaster`), altrimenti creeresti file
> di proprietà di root nella datadir e il nodo non ripartirebbe.

### 4. Verifica

```bash
lks-cli getnetworkinfo | grep subversion     # /Lksc Core:4.17.3.2/
lks-cli masternode status                    # state: READY, PoSePenalty: 0
lks-cli mnsync status | grep AssetName       # MASTERNODE_SYNC_FINISHED
lks-cli getblockcount
```

Controlla **`getnetworkinfo`**, non solo `lksd --version`: un processo già
avviato continua a eseguire il vecchio binario anche dopo che il file è stato
sostituito, e su alcune macchine un supervisore può riavviare `lksd`
automaticamente.

Subito dopo il riavvio `mnsync` può mostrare `MASTERNODE_SYNC_BLOCKCHAIN`: è
normale, entro qualche minuto deve arrivare a `MASTERNODE_SYNC_FINISHED`.

### 5. Nelle 24-48 ore successive

```bash
lks-cli protx info <proTxHash> | grep -i pose
```

`PoSePenalty` deve restare `0` e `PoSeBanHeight` deve essere `-1`.

### Rollback

Se qualcosa non va, si torna indietro reinstallando il pacchetto precedente: il
formato della cartella dati non è cambiato e non viene fatto alcun aggiornamento
irreversibile del database.

```bash
systemctl stop lksd
dpkg -i LKSCoinCore_3300.deb
systemctl start lksd
```

### Note

- **Sentinel** serve ancora con questa versione e continua a funzionare senza
  modifiche.
- Se hai usato una build di prova precedente al rilascio, esegui una volta
  `lks-cli clearbanned`: potresti avere in lista peer sani, bannati per un
  difetto poi corretto.
- Il pacchetto `.deb` è compilato su Ubuntu 18.04 e funziona su 18.04, 20.04,
  22.04, 24.04 e Debian equivalenti.

---

## English

### In short

The upgrade takes three commands and less than a minute of downtime. **No
reindex is required, no masternode re-registration is needed**, and nothing in
your configuration changes: same `lks.conf`, same BLS operator key, same ProTx,
same collateral.

This release **changes no consensus rules and activates no hard fork**: 4.17.3.2
and 3.x nodes interoperate on the same network, so you can upgrade whenever you
prefer.

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
sleep 10
dpkg -i LKSCoinCore_4.17.3.2.deb
systemctl start lksd
```

**Without systemd** (as the user owning the data directory, **never as root**):

```bash
lks-cli stop
sleep 15
sudo dpkg -i LKSCoinCore_4.17.3.2.deb
lksd -daemon
```

> `lksd` and `lks-cli` look for the data directory in the home of the user
> running them. If the daemon runs under a dedicated user, run these commands as
> that user (`su - nodemaster`), otherwise you will create root-owned files in
> the data directory and the node will fail to start.

### 4. Verify

```bash
lks-cli getnetworkinfo | grep subversion     # /Lksc Core:4.17.3.2/
lks-cli masternode status                    # state: READY, PoSePenalty: 0
lks-cli mnsync status | grep AssetName       # MASTERNODE_SYNC_FINISHED
```

Check `getnetworkinfo`, not just `lksd --version`: a running process keeps
executing the old binary image after the file has been replaced, and some setups
restart `lksd` automatically.

### 5. Over the next 24-48 hours

```bash
lks-cli protx info <proTxHash> | grep -i pose
```

`PoSePenalty` must stay `0` and `PoSeBanHeight` must remain `-1`.

### Rollback

Reinstall the previous package; the data directory format is unchanged and no
irreversible database upgrade is performed.

### Notes

- **Sentinel** is still required by this release and keeps working unchanged.
- If you ran a pre-release test build, run `lks-cli clearbanned` once.
- The `.deb` is built on Ubuntu 18.04 and runs on 18.04, 20.04, 22.04, 24.04 and
  equivalent Debian releases.
