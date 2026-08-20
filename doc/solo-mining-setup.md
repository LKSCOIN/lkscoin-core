# Solo mining LKSCOIN — stratum server su m1074

Guida per puntare un ASIC X11 direttamente sulla chain LKSCOIN, senza passare da
un multipool.

## Perché serve

Zpool è un *multipool*: alloca l'hashrate sulla moneta X11 più profittevole del
momento. Il flag `zap=LKS` non garantisce l'allocazione, e la misura sul campo
(14-15 agosto 2026) lo conferma: con un Baikal da 153 MH/s collegato, la chain
riceveva **837 H/s** — meno di un centomillesimo — e la difficoltà restava
inchiodata al minimo (`powLimit`). Il segnale diagnostico è il campo `Diff1` del
miner: 153 quando il lavoro è per LKS, ~2300 quando la pool è passata ad altra
moneta.

Con uno stratum proprio l'hashrate va dove serve, e si ottiene il controllo sul
block template — cosa non secondaria dato che il template deve contenere i
pagamenti masternode e i superblock corretti.

## Architettura

```
   Baikal ASIC ──stratum──► Miningcore ──RPC/GBT──► lksd (m1074)
                                 │
                                 └──► PostgreSQL (statistiche, share)
```

Miningcore gira in Docker: m1074 è Ubuntu 18.04 e installare .NET a mano su
quella release è fragile, mentre il container lo isola.

## 1. Preparare lksd

Aggiungere a `~/.lkscore/lks.conf` (l'utente è quello che esegue il demone):

```ini
server=1
listen=1
daemon=1
rpcuser=lksrpc
rpcpassword=<password-lunga-e-casuale: openssl rand -hex 32>
rpcbind=127.0.0.1
rpcallowip=127.0.0.1
rpcport=3344
```

> **Trappola da conoscere:** `rpcallowip=127.0.0.1/0` **non** limita l'accesso a
> localhost: `/0` è una netmask di zero bit, quindi equivale a `0.0.0.0/0` e
> autorizza qualunque indirizzo IP. Su questa base di codice il default di
> `rpcbind` è localhost, quindi da solo non espone il nodo — ma diventa una porta
> aperta nel momento in cui si aggiunge un `rpcbind` o un proxy davanti. Meglio
> essere espliciti: `rpcallowip=127.0.0.1` (senza suffisso) e
> `rpcbind=127.0.0.1`. Verifica sempre su cosa ascolta davvero il demone con
> `ss -tlnp | grep <rpcport>`.

Riavviare il nodo e verificare:

```bash
lks-cli getblocktemplate '{"rules":["segwit"]}' | head -20
```

Nel risultato devono comparire tre cose:

- `masternode` — l'elenco dei pagamenti che la coinbase deve contenere
  (`payee`, `script`, `amount`), con `masternode_payments_enforced: true`;
- `superblock` — i pagamenti di tesoreria quando l'altezza è di superblock;
- `coinbase_payload` — la transazione coinbase speciale di DIP4 (CbTx), che il
  software di pool deve inserire testualmente nella coinbase come special
  transaction (versione 3, tipo 5).

Se uno dei tre viene ignorato dal software di pool, i blocchi trovati vengono
rifiutati: `bad-cb-payee` nel primo caso, errori su `cbtx` nel terzo. È il motivo
per cui serve un pool della famiglia Dash e non un NOMP generico Bitcoin.

Verifica di coerenza sui numeri (utile anche per accorgersi di un template
sbagliato): `coinbasevalue` deve essere il subsidy meno la quota superblock, e
l'importo verso il masternode deve esserne l'80%. Sulla mainnet LKSCOIN a regime
di 500 LKS per blocco: 450 totali, 360 al masternode, 90 al miner.

> Attenzione: `rpcbind=0.0.0.0` espone l'RPC su tutte le interfacce. Va
> accompagnato da un firewall che consenta la 9998 solo da localhost e dalla
> rete Docker:
> `ufw deny 9998` e `ufw allow from 172.17.0.0/16 to any port 9998`.

## 2. Installare Docker

```bash
sudo apt update && sudo apt install -y docker.io docker-compose
sudo systemctl enable --now docker
```

## 3. Miningcore

```bash
mkdir -p ~/lks-pool && cd ~/lks-pool
git clone https://github.com/oliverw/miningcore.git src
```

`docker-compose.yml`:

```yaml
version: "3"
services:
  db:
    image: postgres:14
    environment:
      POSTGRES_USER: miningcore
      POSTGRES_PASSWORD: <password-db>
      POSTGRES_DB: miningcore
    volumes:
      - ./pgdata:/var/lib/postgresql/data
    restart: unless-stopped

  pool:
    build: ./src
    depends_on: [db]
    network_mode: host        # per raggiungere lksd su 127.0.0.1:9998
    volumes:
      - ./config.json:/app/config.json:ro
    command: ["dotnet", "Miningcore.dll", "-c", "/app/config.json"]
    restart: unless-stopped
```

Con `network_mode: host` l'RPC di lksd può restare su `127.0.0.1` e non serve
esporlo: in quel caso in `lks.conf` bastano `rpcbind=127.0.0.1` e
`rpcallowip=127.0.0.1`, che è più sicuro. Se si usa questa modalità, saltare la
riga `rpcbind=0.0.0.0` del punto 1.

Inizializzare lo schema del database (una volta sola):

```bash
docker-compose up -d db
sleep 10
docker exec -i $(docker-compose ps -q db) psql -U miningcore miningcore < src/src/Miningcore/Persistence/Postgres/Scripts/createdb.sql
```

## 4. Definizione della moneta

Aggiungere a `src/src/Miningcore/coins.json` una voce per LKSCOIN. Punto di
partenza, modellato sulla voce `dash` (verificare i nomi dei campi contro la
versione di Miningcore usata: lo schema è cambiato fra le release):

```json
"lkscoin": {
  "name": "LKSCoin",
  "canonicalName": "LKSCoin",
  "symbol": "LKS",
  "family": "bitcoin",
  "algorithm": "x11",
  "coinbaseHasher": { "hash": "sha256d" },
  "headerHasher": { "hash": "x11" },
  "blockHasher": { "hash": "x11" },
  "hasMasterNodes": true,
  "hasPayee": true,
  "explorerBlockLink": "https://www.lkschain.io/blocks.html?blocknum=$height$",
  "explorerTxLink": "https://www.lkschain.io/tx.html?txid=$hash$"
}
```

`hasMasterNodes` è il campo decisivo: dice a Miningcore di costruire la coinbase
usando i pagamenti masternode indicati dal template. Senza, i blocchi trovati
vengono rifiutati dalla rete con `bad-cb-payee`.

## 5. Configurazione del pool

`config.json` (essenziale, per solo mining con un singolo ASIC):

```json
{
  "logging": { "level": "info", "enableConsoleLog": true },
  "banning": { "manager": "Integrated" },
  "notifications": { "enabled": false },
  "persistence": {
    "postgres": {
      "host": "127.0.0.1", "port": 5432,
      "user": "miningcore", "password": "<password-db>",
      "database": "miningcore"
    }
  },
  "paymentProcessing": { "enabled": false },
  "pools": [{
    "id": "lks1",
    "enabled": true,
    "coin": "lkscoin",
    "address": "<indirizzo-LKS-della-fondazione>",
    "rewardRecipients": [],
    "blockRefreshInterval": 400,
    "jobRebroadcastTimeout": 10,
    "clientConnectionTimeout": 600,
    "banning": { "enabled": false },
    "ports": {
      "3033": {
        "listenAddress": "0.0.0.0",
        "difficulty": 0.05,
        "varDiff": {
          "minDiff": 0.01,
          "maxDiff": 1000,
          "targetTime": 15,
          "retargetTime": 90,
          "variancePercent": 30
        }
      }
    },
    "daemons": [{
      "host": "127.0.0.1",
      "port": 3344,
      "user": "lksrpc",
      "password": "<password-rpc>"
    }]
  }]
}
```

Note sui parametri: `paymentProcessing` è disattivato perché in solo mining la
ricompensa arriva direttamente all'indirizzo della coinbase, senza contabilità
fra utenti. La difficoltà iniziale (0,05) e il vardiff sono tarati per un ASIC da
150 MH/s: il `targetTime` di 15 secondi produce abbastanza share per un vardiff
stabile senza saturare la rete.

Avvio:

```bash
docker-compose up -d
docker-compose logs -f pool
```

## 6. Configurare il Baikal

Nella pagina di configurazione del miner, sostituire la pool zpool con:

```
URL:  stratum+tcp://<ip-di-m1074>:3033
Algo: x11
User: <indirizzo-LKS-della-fondazione>
Pass: x
```

Aprire la porta: `sudo ufw allow 3033/tcp` (meglio limitandola all'IP del miner).

## 7. Verifiche

**Il miner sta lavorando sulla chain giusta?** Nel pannello dell'ASIC il campo
`Diff1` deve essere nell'ordine di 0,05-10, non migliaia. Se torna a valori alti,
il miner sta parlando con un'altra pool.

**Il primo blocco viene accettato?** È il controllo che conta:

```bash
watch -n 30 'lks-cli getblockcount; lks-cli getmininginfo | grep -E "difficulty|networkhashps"'
```

Attese: l'altezza avanza rapidamente nei primi minuti (la difficoltà è al
minimo), poi il Dark Gravity Wave la alza fino all'equilibrio — per 153 MH/s a
150 secondi di target il valore atteso è circa **5,3** — e l'intervallo fra i
blocchi si stabilizza sui 2,5 minuti. `networkhashps` dovrebbe convergere verso
la potenza reale del miner.

**I blocchi sono validi?** Nei log di Miningcore compaiono "Block accepted" e non
"Block rejected"; sul nodo, `getblock <hash>` sull'altezza appena trovata deve
mostrare la coinbase con l'output verso l'indirizzo configurato **e** l'output al
masternode di turno. Se i blocchi risultano trovati ma la chain non avanza, il
problema è quasi certamente nella costruzione della coinbase: rivedere
`hasMasterNodes` (punto 4).

## 8. Dopo la stabilizzazione

Con blocchi regolari, una sessione DKG parte ogni 24 blocchi (~1 ora). Le
sessioni falliranno finché la lista masternode contiene le voci inattive, ma i
tentativi diventeranno visibili nei log:

```bash
lks-cli logging '["llmq-dkg","llmq"]'
grep -iE "dkg|llmq" ~/.lkscore/debug.log | tail -40
```

È la conferma che serve prima di procedere con la finestra di ri-registrazione e
la pulizia della lista descritte in [`network-revival-plan.md`](network-revival-plan.md).

Da tenere presente: evitare variazioni brusche di potenza. Il DGW media su 24
blocchi, quindi accensioni e spegnimenti improvvisi provocano oscillazioni della
difficoltà; una potenza costante, anche inferiore, è preferibile a una potenza
alta e intermittente.
