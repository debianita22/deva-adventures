# Deva's Awesome Adventures — guida per lo sviluppo

Come è fatto il gioco, come si compila, si prova, si modifica e si rilascia. Per installarlo e usarlo
basta il manuale (`docs/manuale.pdf`); le novità di ogni versione sono in `CHANGELOG.md`.

## Requisiti

| Per | Serve |
| --- | --- |
| compilare il core per il PC | `cc` (gcc o clang), `make` |
| i core di rilascio (aarch64, x86_64) | Zig come compilatore C: `pip install ziglang` (glibc ≥ 2.17 come obiettivo) |
| i test automatici | il core per il PC, `ffmpeg` per i video (`NOVIDEO=1` per farne a meno) |
| i test ARM senza console | `qemu-user` e `libc6-arm64-cross` (Ubuntu/Debian) |
| grafica e audio | Python 3 con Pillow e NumPy |
| la voce | Python 3, `sherpa-onnx`, `ffmpeg` con libvorbis, i modelli (vedi *La voce*) |
| i PDF della documentazione | Playwright con Chromium (`python3 -m playwright`) |
| profilazione | `valgrind` (callgrind) |

## Il repository

| Percorso | Che cosa c'è |
| --- | --- |
| `src/` | il core in C99: `libretro.c` (interfaccia), `game.c` (scene, pausa, sessione, diario), `gfx.c` (renderer software RGB565), `audio.c` (mixer e voce in streaming), `save.c` (profili, opzioni, log), `quiz.c` (motore delle domande a carte), una scena per file (`scene_*.c`), `story.c` (le quattro storie), `hero.c` (Deva), `anim.c` (respiro, salti, squash & stretch), `fx.c` (particelle), `trans.c` (transizioni), `amb.c` (sfondi animati) |
| `third_party/` | `libretro.h`, `stb_image.h`, `stb_vorbis.c`, `stb_image_write.h` (solo l'harness) |
| `data/deva_adventures/` | i dati del gioco: atlante e sfondi (`gfx/`), voce (`voce/`), effetti (`sfx/`), musiche (`musica/`), `deva_adventures.cfg` |
| `data/voce/` | `frasi.csv` (i testi della voce, con l'id di ogni frase), `registrate/` (registrazioni vostre) |
| `art/png/` | gli sprite, sorgente dell'atlante |
| `tools/art/` | i generatori della grafica, il packer dell'atlante, `pngpal.py` |
| `tools/audio/` | effetti, note e musiche, generati da codice |
| `tools/voice/` | sintesi della voce e verifica con Whisper |
| `tools/harness/` | il frontend di test senza schermo, i test (`run_tests.sh`, `test_saves.sh`), `transcript.py` |
| `tools/report/` | `report.py`, il resoconto delle partite per i grandi |
| `tools/release/` | `mkdist.py`, i pacchetti di rilascio |
| `packaging/` | `retroarch/` (install.sh, uninstall.sh, LEGGIMI.txt dei pacchetti), `buildroot/` (br2-external per devaOS), `lakka/` (pacchetto per Lakka) |
| `docs/` | il manuale (HTML e PDF), questa guida, il kit del playtest (`playtest/`), le schermate (`img/`) |

## Compilare

```sh
make                 # build/host/deva_adventures_libretro.so, per il PC (-O2 -g)
make aarch64         # build/aarch64/, il core per le console arm64 (Zig, -mcpu=cortex-a35)
make x86_64          # build/x86_64/, il core di rilascio per il PC (Zig)
make dist            # i pacchetti in release/ (vedi Rilasciare)
make test-dist       # install.sh e uninstall.sh dei pacchetti su cartelle di RetroArch finte
make install PREFIX=/usr DESTDIR=/tmp/stage   # core, .info e dati (per chi fa pacchetti)
make clean
```

I core di rilascio usano `-O2 -ffunction-sections -fdata-sections` e `-s -Wl,--gc-sections`.
`PREFIX` e `DATA_DIR` contano anche in compilazione: `DATA_DIR` (di serie
`$(PREFIX)/share/deva_adventures`) è la cartella dove il core cerca i dati se in
`<system>/deva_adventures` non ci sono.

## Provare

```sh
make test            # 16 partite automatiche, il resoconto di due di esse e i salvataggi interrotti
make asan            # core e harness con AddressSanitizer + UBSan, poi:
OUT=build/test-asan NOVIDEO=1 CORE=build/asan/deva_adventures_libretro.so \
    HARNESS=./build/harness-asan ./tools/harness/run_tests.sh
tools/harness/test_saves.sh   # solo i salvataggi interrotti (8 situazioni, 24 controlli)
```

Le 16 partite: demo (tutti i giochi, risposte giuste, sbagliate e guidate, video), sblocco graduale,
le quattro storie, rivedi una storia dall'album, ritorno a metà avventura, la partita lunga (30 minuti, 110.060 frame),
livelli alti, i giochi della 0.6, della 0.10 e della 0.11, il giro dei menu, due «scimmiette» da 25
minuti. Schermate, video e log finiscono in `build/test/<partita>/`. Nessun avviso è accettato: ogni
suono mancante, file illeggibile o salvataggio fallito è un avviso contato.

ThreadSanitizer (il thread dei salvataggi), quando serve:

```sh
make BUILDDIR=build/tsan CFLAGS="-O1 -g -fsanitize=thread" LDFLAGS="-fsanitize=thread"
cc -O1 -g -fsanitize=thread -std=c99 -D_POSIX_C_SOURCE=200809L -Ithird_party \
    -o build/harness-tsan tools/harness/harness.c -ldl -lm -lpthread
CORE=build/tsan/deva_adventures_libretro.so HARNESS=./build/harness-tsan tools/harness/test_saves.sh
```

### L'harness

`build/harness <core.so> <system> <save> <out> [opzioni]` fa girare il core senza schermo, con un bot
che legge le righe `BOT` del log del core e gioca.

| Opzione | Che cosa fa |
| --- | --- |
| `--frames N` | quanti frame (60 al secondo) |
| `--plan cwh` | per ogni risposta: `c` giusta, `w` un errore, `h` due errori (aiuto guidato), a rotazione |
| `--games cps...` | i giochi scelti dal menu, in ordine: c conta, p parole, s sequenze, b balla, n nome, m memory, r ritmo, d dove, e emozioni, o storie, g ginnastica, t trucco, f forme, l lettere, h ombre, w sentiero, q negozio, u misure |
| `--shots`, `--shot-at F:nome` | schermate dei momenti principali, o al frame F |
| `--video file.mp4`, `--audio file.raw` | la partita registrata |
| `--tour` | prima il giro del menu principale, con L e R tenuti, la tastiera, la pausa, Esci |
| `--monkey SEME` | niente bot: una bambina che preme a caso, a umori di qualche secondo |
| `--rivedi`, `--idle-acts`, `--press-at F:K` | rivede una storia dall'album; non fa i gesti dei finali; preme un tasto al frame F |
| `--digest` | stampa l'impronta di tutti i fotogrammi e di tutto il suono |
| `--frame-hashes file` | l'impronta di ogni fotogramma nuovo, per trovare dove due partite si separano |

`DEVA_SEED` fissa il seme del gioco: con lo stesso seme, le stesse opzioni e lo stesso core una
partita è identica, fotogramma per fotogramma. Il copione di una partita, ogni frase con il suo testo:
`python3 tools/harness/transcript.py build/test/storia2/out/log.txt`.

### Su ARM senza console

```sh
sudo apt install qemu-user libc6-arm64-cross
make aarch64 harness-aarch64
qemu-aarch64 -L /usr/aarch64-linux-gnu build/aarch64/harness build/aarch64/deva_adventures_libretro.so \
    <system> <save> <out> --frames 9000 --digest
CORE=build/aarch64/deva_adventures_libretro.so \
    HARNESS="qemu-aarch64 -L /usr/aarch64-linux-gnu build/aarch64/harness" tools/harness/test_saves.sh
```

Con lo stesso seme ARM e x86 danno gli stessi eventi (righe `BOT`), lo stesso salvataggio, lo stesso
log e lo stesso diario. Le immagini differiscono di qualche pixel nelle transizioni (le loro mappe sono
calcolate con funzioni in virgola mobile della libm) e il suono di ±1 sul campione (stb_vorbis decodifica
in virgola mobile). qemu controlla il codice ARM, non la velocità della console.

### Un'ottimizzazione non deve cambiare niente

Le ottimizzazioni della 1.0 sono state accettate solo con le stesse impronte (`--digest`) di prima su
cinque partite di riferimento: soak, demo, quarta storia, scimmietta, giro dei menu (circa 454.000
frame), con gcc, con gcc vettorizzato (le opzioni di devaOS), con clang/Zig e su ARM. Per rifarlo:
la stessa partita con `--digest` sul core vecchio e su quello nuovo, poi confrontare le due righe
`digest video=... audio=...`; se differiscono, `--frame-hashes` dice il primo fotogramma diverso e
`--shot-at` lo fotografa.

## Prestazioni

Misurate sul PC con la partita lunga (30 minuti con tutti i giochi, 110.060 frame), 0.14 e 1.0
alternate due volte, con lo stesso compilatore; `retro_run` per frame:

| Compilazione | 0.14 media | 1.0 media | 1.0 mediana | 1.0 99° perc. |
| --- | --- | --- | --- | --- |
| Zig/clang `-O2` (i core dei pacchetti) | 0,155 ms | 0,070 ms | 0,058 ms | 0,30–0,37 ms |
| gcc `-O2 -ftree-vectorize -fvect-cost-model=dynamic` (devaOS, Lakka) | 0,165 ms | 0,075 ms | 0,064 ms | 0,35–0,39 ms |
| gcc `-O2` (`make`) | 0,170 ms | 0,108 ms | 0,093 ms | 0,40 ms |

Istruzioni eseguite (callgrind, 30.000 frame): da 58,1 a 32,2 miliardi. Decodifica di tutte le immagini:
da 38,8 a 12,9 ms. Picco di memoria del processo: circa 32,5 MB, come prima.

Per profilare: `valgrind --tool=callgrind ./build/harness ... --frames 30000`, poi
`callgrind_annotate --inclusive=no callgrind.out.*`. Sulla RK3326 (Cortex-A35 a 1,3–1,5 GHz) il budget
è 16,7 ms per frame. In-order e con NEON a 64 bit, quindi: i cicli interni senza test per pixel,
scritti in modo che il compilatore li vettorizzi (selezione bit a bit, puntatori `restrict`, aritmetica
su 16 bit). Sul device, con il log di RetroArch su file: `cat retroarch.log | grep '\[deva\]' | grep took`
mostra i salvataggi da 10 ms in su.

## La grafica

Tutto è generato da codice: `python3 tools/art/build_art.py` (tutto), oppure `draw` (gli sprite in
`art/png/`), `pack` (l'atlante da `art/png/`), `bg` (gli sfondi). Per ritoccare uno sprite a mano:
modificate il PNG in `art/png/`, poi `build_art.py pack`. Le PNG sono scritte da `tools/art/pngpal.py`:
indicizzate quando hanno al massimo 256 colori (senza perdita, verificato pixel per pixel).
`python3 tools/art/pngpal.py <file.png ...>` impacchetta file già esistenti. Se ridisegnate un
mostro, la strega o il mago, aggiornate i punti del trucco (`CLIENTS`, `FOES` in `src/scene_trucco.c`).

## Effetti e musiche

`python3 tools/audio/make_audio.py` (tutto) oppure `make_audio.py valle nota_do` (solo quelli). Le
musiche sono masterizzate per l'altoparlante della console: niente sotto i 150 Hz.

## La voce

```sh
python3 tools/voice/gen_voice.py --model <vits-piper-it_IT-paola-medium> \
    --asr <sherpa-onnx-whisper-small> --nome Deva
python3 tools/voice/check_voice.py --whisper <sherpa-onnx-whisper-small>
```

I modelli: `vits-piper-it_IT-paola-medium.tar.bz2` dalle release di sherpa-onnx (tts-models) e
`sherpa-onnx-whisper-small`. Le frasi sono in `data/voce/frasi.csv` (id, testo, voce del personaggio);
`--nome` aggiunge i saluti con il nome; `--asr` ripete una frase finché Whisper non la capisce come il
gioco la farà sentire. `data/voce/registrate/<id>.wav` sostituisce la sintesi, anche per le voci dei
personaggi (per le parole `s_<parola>` aggiornate a mano anche `voce/sillabe.txt`, i millisecondi di
inizio di ogni sillaba). Sulla licenza del modello: `THIRD_PARTY.md`.

Le voci dei personaggi (terzo campo di `frasi.csv`) sono la stessa voce trasformata: `mago`, `strega`,
`strega_buona`, `mostro`, `mostro_buono`, `tuono`, `stregone`, `stregone_buono`, `orco`, `orco_buono`, `re`.

## Come gira il core

- `retro_run`: legge i tasti, `game_frame()` aggiorna e disegna la scena (320×240, RGB565), poi il
  mixer riempie 735 campioni (44,1 kHz). Un fotogramma uguale al precedente non viene ripassato a
  RetroArch (`video_cb(NULL)`).
- Le scene (`scene_t`: enter, update, draw) cambiano con `game_goto`; il quiz a carte (`quiz.c`) è
  condiviso da dodici giochi e dalle sfide. Livelli: +1 dopo tre giuste al primo colpo, −1 dopo
  un aiuto guidato.
- Le voci e le musiche restano OGG compressi in memoria e sono decodificate mentre suonano; i file si
  leggono dalla scheda un poco per frame. Gli sfondi delle storie si caricano un poco per volta.
- I salvataggi: scritti in `<file>.tmp` con la riga `# fine`, poi un thread di appoggio fa `fsync`,
  tiene la versione prima come `.prev` e rinomina; all'avvio un file rovinato lascia il posto alla
  copia intera più recente. Log delle risposte e diario si allungano una riga alla volta.
- Ogni estrazione casuale è in un'istruzione a sé: l'ordine di valutazione degli argomenti in C non è
  stabilito, e gcc e clang lo fanno al contrario (`rng_pair` per le coppie).

## I file scritti nella cartella dei salvataggi

| File | Contenuto |
| --- | --- |
| `deva_adventures.sav`, `_2.sav`, `_3.sav` | i tre profili: nome, livelli, giochi aperti, trucchi, storia, statistiche; ultima riga `# fine` |
| `….prev`, `….rotto` | la versione prima; un file trovato rotto all'avvio |
| `deva_adventures_opzioni.cfg` | le opzioni cambiate nel gioco e il profilo in uso |
| `deva_adventures_log.csv` (e `_2_`, `_3_`) | una riga per risposta: `data;sessione;gioco;livello;domanda;risposta_giusta;scelta;esito;tentativo;secondi_risposta;riascolti;secondo_sessione` |
| `deva_adventures_diario.csv` (e `_2_`, `_3_`) | una riga per evento: `data;sessione;secondo_sessione;evento;dettaglio` (inizio, scena, pausa, promemoria, cambio_profilo, chiusa) |

Il manuale spiega come leggerli; `tools/report/report.py` ne fa una pagina HTML. Le righe del log
scritte prima della 0.15 hanno nove colonne. `tentativo` 3 = risposta arrivata con l'aiuto guidato;
`secondi_risposta` parte da quando poteva rispondere (domanda detta, carte arrivate; dopo un errore, da
quando può riprovare; in Memory la partita intera); `riascolti` conta B e Y (nel sentiero solo Y).

La colonna `domanda`, gioco per gioco: `conta:stelle`, `piu:3-5-4` (Conta); `quanti:farfalla`,
`rima:cane` (Parole); `dopo-AB:ABABA?` (Sequenze); `memoria:3-passi` (Balla con me); `nome:DEVA`
(Il mio nome); `memory:4x3`; `dove:scatola:fuori` (Sopra e sotto); `storia:palloncino` (Emozioni);
`avanti:seme` (La storia in ordine); `vedi:salto3`, `memoria:ruota2+salto3` (Ginnastica: S salto,
C capriola, R ruota, U saluto, `-insieme` = aiuto guidato); `trucco:melmoso:guance+fiore`;
`forma:quadrato_verde`, `non_colore:rosa`, `diverso:cuore_blu` (Forme e colori); `lettera:mela:M`,
`figura:mela:M` (Le lettere); `ombra:gatto`, `ombra:gatto:nuvola`, `colore:luna` (Le ombre);
`passi:4:sassi:2:fiore` (Il sentiero: U D L R, `:insieme` = aiuto guidato); `paga:orsetto:5` (le monete
come `5+2+1`), `vale:2+1+1` (Il negozio); `grande:orsetto:4-0-2`, `pieno:bicchiere:2-0-4`,
`peso:piu:palla-anguria-foglia`, `fila:lungo:serpente` (Le misure). Nelle sfide anche `salti:3`,
`passi:su-sx`, `tamburo:3-SL` (S veloce, L lento), `dove:tavolo:sotto`, `sparito:fiore`, `strada:3` e
le domande finali `stregone:spaventato`, `orco:arrabbiato`, `re:triste`.

Il punto delle avventure nel `.sav`: `avventura` (1–4), `capitolo` (0–5, 5 = avventura finita),
`carica` (round nella bacchetta), `racconti` (i racconti già visti, un bit ciascuno), `epilogo` (la festa
dopo la quarta: 1 da fare, 2 fatta). Per rifare una sola avventura, ad
esempio la seconda: `avventura = 2`, `capitolo = 0`, `carica = 0`, `racconti = 255`; la terza:
`avventura = 3`, …, `racconti = 65535`. Lasciate l'ultima riga, `# fine`.

## Rilasciare

1. La versione: `GAME_VERSION` in `src/common.h`, `display_version` in `deva_adventures_libretro.info`,
   `DEVA_ADVENTURES_VERSION` in `packaging/buildroot/package/deva-adventures/deva-adventures.mk`,
   `PKG_VERSION` in `packaging/lakka/deva_adventures/package.mk`.
2. `CHANGELOG.md`: una voce `## [x.y.z] - AAAA-MM-GG` (la data diventa la data degli archivi).
3. I controlli: `make test`, `make asan` con le partite, i salvataggi su ARM, le impronte se avete
   toccato il renderer o il mixer.
4. Le schermate del manuale, rifatte dalle partite appena giocate da `make test`:
   `python3 tools/release/doc_images.py` (`docs/img/`, l'elenco è nello script). Poi i PDF:
   `python3 tools/release/html2pdf.py docs/manuale.html docs/playtest/scheda_osservazione.html`.
5. `make dist`: in `release/` i sorgenti, i due pacchetti RetroArch e `SHA256SUMS`. Gli archivi sono
   riproducibili: rifatti dallo stesso albero hanno gli stessi byte (anche i core: Zig non vi scrive
   percorsi né date).
6. `make test-dist`: lo script di installazione del pacchetto x86_64 su cartelle di RetroArch finte
   (installazione nuova, aggiornamento con impostazioni e salvataggi, copie di sicurezza e ritorno
   indietro, errore a metà con ripristino, disinstallazione), con busybox se c'è; il core installato
   viene anche fatto girare dall'harness. `NO_BUSYBOX=1` usa la `sh` e gli strumenti del sistema.

## devaOS (Buildroot) e Lakka

- **devaOS**: `packaging/buildroot` è un br2-external.
  `make BR2_EXTERNAL=/percorso/deva-adventures/packaging/buildroot menuconfig`, poi *External options*
  → *deva-adventures*. Core in `/usr/lib/libretro` (configurabile), dati in
  `/usr/share/deva_adventures`. Richiede un toolchain con i thread (`BR2_TOOLCHAIN_HAS_THREADS`).
- **Lakka**: `packaging/lakka/README.md`.
