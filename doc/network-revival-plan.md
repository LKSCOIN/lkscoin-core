# LKSCOIN — Piano di rianimazione della rete e migrazione dei masternode

**Documento:** specifica del fork di rianimazione + guida operativa per gli operatori
**Versione software:** LKSCOIN Core 4.17.3.3 (ramo di sviluppo del fork)
**Stato:** proposta — altezze di attivazione non ancora fissate
**Data:** 14 agosto 2026

*(English version below)*

---

## 1. Il problema, con i numeri

Un censimento eseguito il 14 agosto 2026 con `contrib/lks-monitor/mn-census.py`, che
apre una connessione P2P verso ogni masternode registrato, ha prodotto questo esito:

| Misura | Valore |
|---|---|
| Masternode nella lista deterministica | **702** |
| Marcati `ENABLED` dalla lista | 503 |
| Effettivamente raggiungibili sulla porta P2P | **11** (1,6%) |
| Di cui su 4.17.3.2 | 6 |
| Di cui su 3.3.0 | 5 |
| Ultimo quorum LLMQ formato | blocco ~571.680 (2021) |
| Hashrate di rete | ~565 H/s |
| Blocchi prodotti | ~44 al giorno (target: 576) |

Ne discende una situazione che si auto-mantiene. I membri di un quorum vengono
estratti in modo deterministico dall'intera lista: con il 98% di voci inattive,
nessun DKG raggiunge il numero minimo di partecipanti e **nessun quorum si forma
dal 2021**. Poiché le penalità PoSe sono applicate proprio dai quorum, le voci
inattive non possono essere rimosse dal protocollo, e restano nella lista a
tempo indeterminato. Senza quorum non ci sono ChainLocks e InstantSend non ha
firmatari.

C'è anche una conseguenza economica: l'80% di ogni ricompensa di blocco è
destinato ai masternode, e la quasi totalità di quei pagamenti finisce oggi su
wallet abbandonati, a fronte di nessun servizio reso alla rete.

## 2. L'intervento

Tre modifiche, da attivare in un unico fork coordinato.

### 2.1 Finestra di ri-registrazione e rimozione delle voci inattive

Viene dichiarata pubblicamente una finestra fra due altezze di blocco,
`nMNPurgeStartHeight` e `nMNPurgeHeight`. Ogni masternode che, all'interno della
finestra, pubblica una transazione speciale di tipo **ProRegTx**, **ProUpServTx**,
**ProUpRegTx** o **ProUpRevTx** dimostra di essere presidiato: sono transazioni
firmate con chiavi che solo l'operatore o il proprietario possiedono, quindi non
falsificabili per conto di terzi.

All'altezza `nMNPurgeHeight`, tutte le voci che non hanno dato prova di vita
vengono rimosse dalla lista deterministica.

**Il collaterale non viene toccato.** Le 100.000 LKS di ogni masternode restano
di proprietà del titolare, spendibili in qualsiasi momento. La rimozione riguarda
esclusivamente l'iscrizione al registro dei fornitori di servizio: chi è stato
rimosso può ri-registrarsi quando vuole con una nuova ProRegTx, riprendendo a
percepire i pagamenti dal momento della nuova registrazione.

### 2.2 Quorum dimensionati sulla rete reale

Viene introdotto il tipo di quorum `LLMQ_LKS_10_60` — 10 membri, minimo 7
partecipanti, soglia di firma 6 — attivo dall'altezza `nLKSSmallQuorumHeight`.
Le ChainLocks passano da `LLMQ_400_60` (che richiede 300 partecipanti a un DKG,
irraggiungibili) a questo nuovo tipo.

**Va detto con chiarezza:** una soglia di 6 su 10 è un'assunzione di fiducia
debole. Chi controlla 6 membri può firmare una ChainLock da solo, e allo stato
attuale la Fondazione opera 6 degli 11 nodi vivi. Questi parametri sono un
**punto di partenza per far ripartire il meccanismo**, non un obiettivo: vanno
alzati appena il numero di operatori indipendenti lo consente. Fino ad allora le
ChainLocks prodotte da questo tipo di quorum non vanno presentate come finalità
decentralizzata, e la garanzia forte per i servizi (LKS Notary) resta
l'ancoraggio su Bitcoin.

### 2.3 Nessuna modifica ai pagamenti né al collaterale

Lo schema di emissione, la quota dell'80% ai masternode e il collaterale di
100.000 LKS restano invariati. Questo fork non ridistribuisce valore: rimuove
dalla lista chi non fornisce servizio.

## 3. Calendario proposto

| Fase | Durata | Contenuto |
|---|---|---|
| **0. Annuncio** | — | Pubblicazione di questo documento, delle altezze e dei canali di supporto |
| **1. Finestra di ri-registrazione** | ≥ 8 settimane di calendario | Gli operatori pubblicano la prova di vita. Supporto attivo su chat, forum ed e-mail |
| **2. Rilascio del software** | entro la fase 1 | 4.17.3.3 con le regole del fork, binari e pacchetto `.deb` |
| **3. Attivazione** | `nMNPurgeHeight` | Rimozione delle voci inattive; da `nLKSSmallQuorumHeight` i nuovi quorum diventano validi |
| **4. Verifica** | 2 settimane | Formazione dei DKG, comparsa delle prime ChainLocks, monitoraggio PoSe |

Le altezze vanno fissate **in blocchi**, non in date, e convertite in date
indicative sulla base del ritmo di produzione dei blocchi al momento
dell'annuncio. Con l'aumento di hashrate previsto il ritmo cambierà: la finestra
va quindi dimensionata con margine abbondante, e l'annuncio deve riportare
entrambe le informazioni (altezza esatta e data stimata).

Nessuna data va comunicata prima che il software sia pronto e testato su devnet.

## 4. Cosa deve fare un operatore

### 4.1 In sintesi

1. Aggiornare il nodo alla 4.17.3.3.
2. Verificare che il masternode sia raggiungibile dall'esterno sulla porta 9400.
3. Pubblicare la prova di vita con una `protx update_service`.
4. Verificare che la prova sia stata registrata.

### 4.2 Comandi

Aggiornamento del software: si veda [`upgrade-guide-masternodes.md`](upgrade-guide-masternodes.md).
Nessun reindex, nessuna riconfigurazione, nessuna nuova registrazione.

Prova di vita, dal wallet che detiene la chiave operatore:

```
protx update_service <proTxHash> <indirizzoIP:9400> <chiaveOperatoreBLS> "" <indirizzoPerLeFee>
```

- `<proTxHash>` si legge con `lks-cli masternode status` sul nodo.
- `<chiaveOperatoreBLS>` è la chiave privata operatore già in uso (`masternodeblsprivkey`
  nel `lks.conf`).
- `<indirizzoPerLeFee>` è un indirizzo del wallet con un piccolo saldo per pagare
  la commissione (pochi LKS).

Chi ha smarrito la chiave operatore ma controlla il collaterale può usare in
alternativa `protx update_registrar` con la chiave owner, oppure registrare
nuovamente il masternode con `protx register_fund` / `protx register`.

### 4.3 Verifica

```
lks-cli protx info <proTxHash>
```

La Fondazione pubblicherà inoltre, per tutta la durata della finestra, l'elenco
aggiornato dei masternode che hanno già dato prova di vita, in modo che ciascuno
possa controllare la propria posizione senza dipendere da noi.

### 4.4 Se non si fa nulla

Alla rimozione il masternode smette di comparire nella lista e di ricevere
pagamenti. **Il collaterale resta intatto e disponibile.** La ri-registrazione è
sempre possibile in seguito, con una nuova ProRegTx, e comporta di ripartire in
fondo alla coda dei pagamenti come qualsiasi nuova registrazione.

## 5. Rischi e contromisure

| Rischio | Contromisura |
|---|---|
| Operatori mai raggiunti dall'annuncio | Finestra lunga; annuncio ripetuto su tutti i canali; contatto diretto agli operatori identificabili; possibilità di ri-registrarsi anche dopo |
| Contestazione della legittimità | Finestra pubblica dichiarata in anticipo, criterio oggettivo e verificabile da chiunque sulla catena, collaterale intoccato |
| Rete troppo piccola anche dopo la pulizia | Se i masternode superstiti fossero meno di 7, nemmeno `LLMQ_LKS_10_60` formerebbe quorum: in quel caso si rinvia l'attivazione dei quorum e si procede solo con la pulizia |
| Miner non aggiornati | I miner vanno contattati prima dell'attivazione: senza di loro il fork non viene prodotto |
| Servizi terzi (exchange, explorer) con nodi non aggiornati | Censimento e contatto prima di fissare le altezze |
| Concentrazione della firma nei nuovi quorum | Dichiarata apertamente (§2.2); parametri da rialzare appena possibile; ancoraggio Bitcoin come garanzia indipendente |

## 6. Stato dell'implementazione

Nel ramo di sviluppo (versione 4.17.3.3) sono già presenti:

- `LLMQ_LKS_10_60` con i relativi parametri, registrato su tutte le reti e
  **inattivo** finché `nLKSSmallQuorumHeight` resta a 0;
- `llmqTypeChainLocks` spostato sul nuovo tipo;
- `CDeterministicMNManager::PurgeInactiveMNs()`, che alla sola altezza
  `nMNPurgeHeight` legge i blocchi della finestra, raccoglie le prove di vita e
  rimuove le voci restanti;
- i tre parametri di consenso, tutti a 0, cioè funzionalità disattivata.

Da completare prima del rilascio: test su devnet dell'intero ciclo
(finestra → purge → formazione quorum → ChainLock), strumento pubblico di
verifica delle prove di vita, e fissazione delle tre altezze.

---

# LKSCOIN — Network revival plan and masternode migration

## 1. The problem, in numbers

A census run on 14 August 2026 with `contrib/lks-monitor/mn-census.py`, which
opens a P2P connection to every registered masternode, found:

| Measure | Value |
|---|---|
| Masternodes in the deterministic list | **702** |
| Marked `ENABLED` | 503 |
| Actually reachable on the P2P port | **11** (1.6%) |
| Last LLMQ quorum formed | block ~571,680 (2021) |
| Network hashrate | ~565 H/s |
| Blocks produced | ~44/day (target: 576) |

Quorum members are drawn deterministically from the whole list. With 98% of the
entries inactive, no DKG reaches its minimum size, so **no quorum has formed
since 2021**. Because PoSe penalties are applied by quorums, the protocol cannot
evict the stale entries by itself: the state is self-sustaining. Without quorums
there are no ChainLocks and InstantSend has no signers. Economically, 80% of
every block reward goes to masternodes, and almost all of it currently lands in
abandoned wallets in exchange for no service.

## 2. The change

**Re-registration window and removal of inactive entries.** A window is publicly
announced between two block heights. Any masternode publishing a ProRegTx,
ProUpServTx, ProUpRegTx or ProUpRevTx inside the window proves it is operated —
these are signed with keys only the operator or owner holds. At the purge height,
entries without such a proof are removed from the deterministic list.
**Collateral is not touched**: the 100,000 LKS remain the owner's, and removed
operators may re-register at any time.

**Quorums sized for the real network.** A new type `LLMQ_LKS_10_60` (10 members,
minimum 7, threshold 6) is enabled from an activation height, and ChainLocks move
to it from `LLMQ_400_60`, whose DKG would need 300 participants.

A 6-of-10 threshold is a weak trust assumption: whoever runs 6 members can sign a
ChainLock alone, and the Foundation currently operates 6 of the 11 live nodes.
These are bootstrap parameters to restart the mechanism, to be raised as soon as
the number of independent operators allows. Until then ChainLocks from this
quorum type must not be presented as decentralised finality, and the strong
guarantee for services (LKS Notary) remains the Bitcoin anchoring.

**No change to payments or collateral.** Emission, the 80% masternode share and
the 100,000 LKS collateral are unchanged. This fork does not redistribute value;
it removes from the list those who provide no service.

## 3. What an operator has to do

1. Upgrade to 4.17.3.3 — see [`upgrade-guide-masternodes.md`](upgrade-guide-masternodes.md).
   No reindex, no reconfiguration, no re-registration.
2. Make sure the node is reachable from the outside on port 9400.
3. Publish the proof of life:

```
protx update_service <proTxHash> <ip:9400> <blsOperatorKey> "" <feeSourceAddress>
```

4. Check it with `lks-cli protx info <proTxHash>`. The Foundation will also
   publish, throughout the window, the list of masternodes that have already
   proved liveness.

If you do nothing, your masternode stops being listed and paid at the purge
height. **Your collateral is unaffected** and you can re-register later with a
new ProRegTx, joining the payment queue as any new registration.

## 4. Timeline

Announcement → re-registration window (at least 8 calendar weeks) → software
release → activation at the announced heights → two weeks of verification.
Heights are expressed in blocks and converted to indicative dates at announcement
time; with the planned hashrate increase the block rate will change, so the
window is sized with a wide margin. No date is announced before the software has
been tested on devnet.
