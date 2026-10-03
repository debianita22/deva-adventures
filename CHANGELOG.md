# Novità, versione per versione

Le date sono quelle dei pacchetti. I numeri di prestazioni sono misurati con l'harness sul PC (x86_64),
sempre confrontando due versioni sulla stessa macchina, una dopo l'altra.

## [1.0.0] - 2026-10-02

Prima versione completa: più veloce, documentata e impacchettata per RetroArch, devaOS e Lakka.

### Più veloce, con lo stesso gioco

- `retro_run` per frame nella partita lunga (30 minuti con tutti i giochi, 110.060 frame; 0.14 e 1.0
  alternate due volte sulla stessa macchina, con lo stesso compilatore):

  | Compilazione | 0.14 | 1.0 | |
  | --- | --- | --- | --- |
  | Zig/clang `-O2`, i core dei pacchetti | 0,155 ms | **0,070 ms** | −55% |
  | gcc `-O2` vettorizzato, come devaOS e Lakka | 0,165 ms | **0,075 ms** | −55% |
  | gcc `-O2`, il `make` del PC | 0,170 ms | **0,108 ms** | −37% |

  Il 99° percentile scende da 0,46–0,52 a 0,30–0,40 ms, i frame oltre 1 ms da 92–97 a 15–24; le
  istruzioni eseguite (callgrind, 30.000 frame) da 58,1 a 32,2 miliardi. Immagini e suono **identici**:
  cinque partite di riferimento (circa 454.000 frame: tutti i giochi, errori e aiuti, la quarta storia,
  la «scimmietta», il giro dei menu) danno la stessa impronta di ogni fotogramma e di ogni campione audio
  prima e dopo, con gcc, con gcc vettorizzato, con clang e su ARM.
- Il bordo dei pannelli arrotondati (le sfide della storia) faceva cinque radici quadrate per pixel e
  passava anche sull'interno vuoto: era il 17% del tempo. Ora una radice per riga e l'interno saltato.
- Sprite: la copia con la maschera non fa più quattro controlli per pixel e diventa codice vettoriale
  (NEON sulla console, 16 pixel per volta), anche per gli sprite specchiati.
- Dissolvenze e pannelli trasparenti: la stessa formula su corsie a 16 bit, vettorizzata; le tinte da
  piccole tabelle preparate una volta.
- Un fotogramma uguale al precedente si riconosce con un confronto che si ferma al primo pixel diverso,
  non con l'hash di tutto lo schermo; il mixer audio lavora canale per canale; i suoni si trovano per
  nome con una tabella hash; niente divisioni per pixel negli sprite ingranditi.
- Immagini: PNG indicizzate senza perdita (gli stessi colori, esatti): grafica da 1,08 a 0,51 MB,
  decodifica di tutte le immagini 3 volte più veloce (38,8 → 12,9 ms sul PC), caricamento del core
  (`retro_load_game`) da 28–30 a 18 ms (mediana di 15 avvii, ogni versione con i suoi dati). La
  pipeline della grafica scrive già così (`tools/art/pngpal.py`).
- Build di rilascio con sezioni separate e `--gc-sections`: il core aarch64 passa da 586 a 564 KB
  (la 0.15 era di 581 KB); quello x86_64 ora è compilato con Zig per glibc ≥ 2.17 (prima chiedeva la
  glibc del PC di build).

### Pacchetti

- `make dist` scrive in `release/` i sorgenti, i pacchetti RetroArch **aarch64** e **x86_64** e
  `SHA256SUMS`; gli archivi sono riproducibili (stesso contenuto = stessi byte).
- Ogni pacchetto RetroArch ha `install.sh` (POSIX, anche busybox): trova `retroarch.cfg`, mostra le
  cartelle e chiede conferma, verifica il pacchetto, mette da parte la versione installata e i
  salvataggi, sostituisce core e `.info` senza mai lasciarne mezzi, tiene le impostazioni dei grandi
  (quando i loro valori sono diversi da quelli nuovi di serie), controlla i file copiati e, se qualcosa
  si interrompe a metà, rimette tutto com'era; `--restore` torna indietro, `uninstall.sh` toglie tutto
  tranne i salvataggi. `make test-dist` lo prova su cartelle di RetroArch finte, con la `sh` e i comandi
  di busybox come sulle console: 55 controlli, compreso il core installato fatto girare dall'harness.
- **devaOS**: `packaging/buildroot` è un br2-external pronto (vettorizzazione completa e
  `--gc-sections` già impostate). **Lakka**: `packaging/lakka/deva_adventures/package.mk`.
- `make install` con `PREFIX`, `DESTDIR`, `DATA_DIR`, `LIBRETRO_DIR` per chi impacchetta per una
  distribuzione.

### Correzioni

- Il `.info` dichiara `single_purpose = "true"`: con la scelta di serie di RetroArch per i *Contentless
  Cores* («Uso singolo») il gioco prima non compariva nell'elenco (si avviava solo da *Load Core*).
- `deva_adventures.cfg`: i commenti dicevano ancora che le Opzioni si aprono tenendo il bottone rosso
  (dalla 0.14 sono L e R) e parlavano di una storia sola; per ricominciare le avventure ora elencano tutte
  le righe da togliere (anche `avventura` ed `epilogo`) e ricordano di lasciare `# fine`. Valori invariati.
- Opzioni: la riga «Storia della strega» si chiama «Avventure (mappa e sfide)».
- `install.sh` trova i salvataggi anche quando RetroArch li ordina in cartelle per nome del core o li
  scrive nella cartella del contenuto; legge `~` nei percorsi di `retroarch.cfg` come la home di chi
  possiede quella configurazione (root che installa per l'utente di RetroArch); alla fine dice tutti e
  due i modi di avviare il gioco.

### Documentazione

- Manuale per i genitori (`docs/manuale.pdf`, 11 pagine, con l'HTML accanto) con le schermate della
  1.0: installazione, giochi, avventure, opzioni, file, la prima prova, problemi e soluzioni.
  `tools/release/doc_images.py` rifà le schermate dalle partite di `make test`,
  `tools/release/html2pdf.py` i PDF.
- `docs/SVILUPPO.md` (build, test, harness, pipeline, architettura, formati dei file, rilascio),
  `THIRD_PARTY.md` (le licenze, con quella della voce verificata), questo file; il README è diventato
  una pagina di presentazione. Tolta la cartella `dist/` con i core vecchi: i pacchetti sono in
  `release/`.

### Strumenti

- Harness: `--digest` stampa l'impronta di tutti i fotogrammi e di tutto il suono di una partita.

## [0.15.0] - 2026-10-02

- Salvataggi a prova di spegnimento: riga `# fine`, `fsync` del file e della cartella, copia `.prev`,
  ripristino automatico della copia intera più recente; la parte lenta in un thread di appoggio.
  `tools/harness/test_saves.sh`: 8 situazioni, 24 controlli.
- Kit per il primo playtest: log delle risposte con tempi e riascolti, diario delle sessioni,
  `tools/report/report.py` (pagina HTML), scheda di osservazione da stampare.
- Verifica su ARM sotto `qemu-aarch64`: stessi eventi, salvataggi, log e diario dell'x86; corretto
  l'ordine non specificato di due estrazioni casuali nella stessa espressione (19 punti).

## [0.14.0] - 2026-10-02

- A prova di bambina: Opzioni, Salvataggi ed Esci con **L e R tenuti 2 secondi**; pausa con START
  tenuto da solo; in una domanda nuova il rosso risponde quando la domanda è stata detta.
- Una sola carta «?» nel menu; da album e camerino si torna su «Gioca»; promemoria che si diradano;
  «Ancora un gioco, e poi la nanna!»; 11 frasi nuove per variare le più ripetute.
- Harness: la «scimmietta» (`--monkey`), una bambina che preme a caso. In 4 ore di tasti a caso: pause
  aperte per caso da 484 a 0, porte dei grandi da 3 a 0.

## [0.13.0] - 2026-10-02

- Personaggi vivi: respirano, sbattono le palpebre (41 disegni), parlano a tempo con la voce.
- Squash & stretch senza ridimensionare: salti con preparazione e atterraggio, carte che atterrano.
- Otto luoghi ridipinti con luce e volume; uccellini, farfalle, nuvolette, stelle cadenti.

## [0.12.0] - 2026-10-02

- Il gesto gentile dei finali si fa con i bottoni (il ballo, la lanterna, la ninna nanna, il gioco a
  turno) e non si può sbagliare.
- Una scena per ogni gioco del palco (il prato di Conta, il pappagallo di Parole, il trenino di
  Sequenze...); il luogo resta stregato durante la sfida; l'epilogo con la festa a sorpresa.
- Circa 50 frasi riscritte; il bentornato sulla mappa conta i tesori ritrovati.

## [0.11.0] - 2026-10-01

- Due giochi: **Il negozio** (monete da 1, 2 e 5) e **Le misure** (grande, lungo, pieno, pesante, in fila).
- La quarta avventura, «I giocattoli del Re Capriccio».
- Rivedi le storie dall'album.

## [0.10.0] - 2026-09-30

- Tre giochi: **Le lettere**, **Le ombre**, **Il sentiero** (primo coding).
- La terza avventura, «La musica perduta».

## [0.9.0] - 2026-09-30

- Animazioni: transizioni fra le scene, sfondi vivi, carte che entrano con un rimbalzo, la stella che
  vola, coriandoli; `animazioni = 0` per spegnerle.

## [0.8.0] - 2026-09-29

- Adesivi per l'album dopo tutti i trucchi; correzioni emerse giocando (Sopra e sotto, Conta, Emozioni,
  Trucca i mostri, Memory, duelli); luoghi e oggetti ridisegnati.

## [0.7.1] - 2026-09-29

- Le due storie ricontrollate frase per frase dal copione di una partita (`tools/harness/transcript.py`).

## [0.7.0] - 2026-09-29

- La seconda avventura, «La notte senza stelle»: nei duelli entrano tutti i tredici giochi.

## [0.6.0] - 2026-09-29

- Tre giochi: **Ginnastica**, **Trucca i mostri**, **Forme e colori**, anche nei duelli.

## [0.5.0] - 2026-09-28

- Menu principale a carte (Gioca, Camerino, Album, Salvataggi, Opzioni, Esci), pausa, tre profili con
  la tastiera per il nome, opzioni nel gioco.

## [0.4.0] - 2026-09-28

- La storia: «L'incantesimo grigio», con la mappa, la bacchetta da caricare, quattro mostri e la strega.

## [0.3.0] - 2026-09-28

- Dieci giochi (Il mio nome, Memory, Batti il ritmo, Sopra e sotto, Emozioni, La storia in ordine), che
  si aprono uno alla volta con una carta «?».

## [0.2.1] - 2026-09-28

- I tasti nominati per colore come sulla RF35H; la prova dei tasti nelle prime sessioni.

## [0.2.0] - 2026-09-28

- **Deva's Awesome Adventures**: Deva, la ballerina in pixel art; quattro giochi (Conta, Parole,
  Sequenze, Balla con me) con livelli adattivi da 1 a 5, aiuto guidato, camerino dei trucchi e tempo di
  gioco con la buonanotte.

## [0.1.0] - 2026-09-28

- Il prototipo, «Piroetta»: il core libretro contentless e un solo gioco, Conta.
