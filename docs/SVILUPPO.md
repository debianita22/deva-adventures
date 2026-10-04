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
| il gioco sul PC senza RetroArch | per compilarlo solo `cc`; per farlo girare SDL2 (`libsdl2-2.0-0`) |
| il programma per Windows | Zig (da Linux), `curl` per la SDL2 ufficiale; l'installazione: `nsis` (`makensis` 3.x) |
| il programma per macOS | un Mac con gli strumenti a riga di comando di Xcode (`clang`, `lipo`, `codesign`, `hdiutil`) |
| `make test-linux` | `xvfb`, `openbox`, `xdotool`, `x11-utils`, ImageMagick; facoltativi `libsdl2-dev` (il confronto con gli header di SDL2), `desktop-file-utils`, `busybox` |
| `make test-windows` | `wine` (64 bit), `xvfb`, `openbox`, `xdotool`, `x11-utils`, ImageMagick |
| grafica e audio | Python 3 con Pillow e NumPy |
| la voce | Python 3, `sherpa-onnx`, `ffmpeg` con libvorbis, i modelli (vedi *La voce*) |
| i PDF della documentazione | Playwright con Chromium (`python3 -m playwright`) |
| profilazione | `valgrind` (callgrind) |

## Il repository

| Percorso | Che cosa c'è |
| --- | --- |
| `src/` | il core in C99: `libretro.c` (interfaccia), `game.c` (scene, pausa, sessione, diario), `gfx.c` (renderer software RGB565), `audio.c` (mixer e voce in streaming), `save.c` (profili, opzioni, log), `quiz.c` (motore delle domande a carte), una scena per file (`scene_*.c`), `story.c` (le quattro storie), `hero.c` (Deva), `anim.c` (respiro, salti, squash & stretch), `fx.c` (particelle), `trans.c` (transizioni), `amb.c` (sfondi animati), `plat.c` (sostituire e scrivere sul disco un file, su ogni sistema) |
| `platform/sdl/` | il gioco sul PC senza RetroArch: `main.c` (il frontend), `os.c` (quello che chiede al sistema: Linux, macOS, Windows), `sdl2_api.h` e `sdl2_functions.h` (la parte di SDL2 che usa) |
| `third_party/` | `libretro.h`, `stb_image.h`, `stb_vorbis.c`, `stb_image_write.h` (solo l'harness) |
| `data/deva_adventures/` | i dati del gioco: atlante e sfondi (`gfx/`), voce (`voce/`), effetti (`sfx/`), musiche (`musica/`), `deva_adventures.cfg` |
| `data/voce/` | `frasi.csv` (i testi della voce, con l'id di ogni frase), `registrate/` (registrazioni vostre) |
| `art/png/` | gli sprite, sorgente dell'atlante |
| `tools/art/` | i generatori della grafica, il packer dell'atlante, `pngpal.py` |
| `tools/audio/` | effetti, note e musiche, generati da codice |
| `tools/voice/` | sintesi della voce e verifica con Whisper |
| `tools/harness/` | il frontend di test senza schermo, i test (`run_tests.sh`, `test_saves.sh`, `test_linux.sh` con `fake_pad.c`, `test_windows.sh`), `check_sdl2_api.sh`, `transcript.py` |
| `tools/report/` | `report.py`, il resoconto delle partite per i grandi |
| `tools/release/` | `mkdist.py` (i pacchetti di rilascio), `mkapp.sh` (l'app e il disco per macOS), `notes.py` (le note della release), `test_install.sh`, `test_app.sh` (l'app del disco provata su un Mac), `try_windows.ps1` (i pacchetti provati su Windows), `ci_step.sh` (un passo della CI), `html2pdf.py`, `doc_images.py` |
| `packaging/` | `retroarch/` (install.sh, uninstall.sh, LEGGIMI.txt dei pacchetti), `linux/` (il pacchetto per PC: install.sh, LEGGIMI.txt, voce di menu e icona), `windows/` (installazione NSIS, manifest, risorse, icona), `macos/` (Info.plist, icona, LEGGIMI.txt), `sdl2/` (la SDL2 dei pacchetti: versione, impronte, licenza), `buildroot/` (br2-external per devaOS), `lakka/` (pacchetto per Lakka) |
| `.github/workflows/` | `ci.yml`: la CI di GitHub (build, test e pacchetti su Linux, Windows e macOS; la release) |
| `docs/` | il manuale (HTML e PDF), questa guida, il kit del playtest (`playtest/`), le schermate (`img/`) |

## Compilare

```sh
make                 # build/host/deva_adventures_libretro.so, per il PC (-O2 -g)
make aarch64         # build/aarch64/, il core per le console arm64 (Zig, -mcpu=cortex-a35)
make x86_64          # build/x86_64/, il core di rilascio per il PC (Zig)
make linux           # build/host/deva-adventures, il gioco sul PC senza RetroArch
make linux-x86_64    # build/x86_64/deva-adventures, lo stesso per il rilascio (Zig, glibc ≥ 2.17)
make windows         # build/win64/deva-adventures.exe, per Windows 10 e 11 x64 (Zig, da Linux)
make mac             # build/mac/deva-adventures, per macOS arm64 + x86_64 (su un Mac, clang di Xcode)
make dist            # i pacchetti in release/ (vedi Rilasciare)
make test-dist       # install.sh e uninstall.sh dei pacchetti su cartelle di RetroArch finte
make test-linux      # il programma per PC su uno schermo virtuale, e l'install.sh del suo pacchetto
make test-windows    # il pacchetto per Windows di release/ sotto Wine, su uno schermo virtuale
make install PREFIX=/usr DESTDIR=/tmp/stage         # core, .info e dati (per chi fa pacchetti)
make install-linux PREFIX=/usr DESTDIR=/tmp/stage   # il programma per PC, i dati, voce di menu e icona
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

### Il programma per PC

`make test-linux` (`tools/harness/test_linux.sh`, circa due minuti) prova il programma
`build/host/deva-adventures` (`BIN=` per un altro, ad esempio quello di rilascio) in tre parti: senza
finestra (opzioni, codici di uscita, messaggi, dove trova i dati); su uno schermo virtuale (Xvfb con
openbox: la finestra e la sua immagine, la classe e l'icona, i tasti veri mandati con xdotool lungo il
titolo, la prova dei tasti e il primo racconto, la pausa tenuta, la finestra ridotta a icona, F11 con i
pixel interi, la seconda copia, Alt+F4 e SIGTERM con salvataggio e diario chiusi; lo stesso giro con un
gamepad virtuale, per posizione e per colore, poi staccato; 12 secondi di ritmo con e senza suono); poi
`install.sh` del pacchetto di `release/` in una home finta (installazione, aggiornamento, pacchetto
rovinato, errore prima e dopo lo scambio delle cartelle, gioco aperto, cartella con lo spazio,
disinstallazione, con dash e busybox).
Il gamepad virtuale è di SDL stessa: `tools/harness/fake_pad.c` è una libreria che il programma carica
al posto di SDL2 (`DEVA_SDL2_LIB`), uguale alla vera tranne `SDL_Init`, che attacca il gamepad, e
`SDL_PollEvent`, che preme i tasti scritti dal test in un file (`DEVA_FAKE_PAD`).
`tools/harness/check_sdl2_api.sh` confronta `platform/sdl/sdl2_api.h` con gli header veri di SDL2:
valori, strutture e prototipi (serve `libsdl2-dev`).

### Windows e macOS

`make test-windows` (`tools/harness/test_windows.sh`, 48 controlli, dopo `make dist`) prova il
pacchetto per Windows sotto Wine (64 bit) su uno schermo virtuale: il programma dello zip (opzioni,
codici di uscita, la finestra e la sua immagine disegnata da Direct3D, i tasti lungo il titolo, la
pausa, Alt+Invio, F11, Alt+F4, la seconda copia, salvataggi e diario in `%APPDATA%` con gli a capo
LF); poi l'installazione silenziosa (`/S`), l'aggiornamento che tiene le impostazioni dei grandi e la
disinstallazione che tiene i salvataggi. Wine non è Windows: per questo la CI prova gli stessi
pacchetti anche su Windows vero, e l'app su un Mac.

Senza schermo, su qualsiasi sistema, basta una partita breve con il video e il suono finti di SDL:

```sh
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy DEVA_TEST_FRAMES=600 DEVA_NO_DIALOG=1 \
    build/host/deva-adventures --data data --saves /tmp/prova --verbose
tail -n 1 /tmp/prova/deva-adventures.log   # "600 frames in 10.0 s, ..."
tail -n 1 /tmp/prova/deva_adventures.sav   # "# fine"
```

`DEVA_TEST_FRAMES=N` chiude il programma in ordine dopo N fotogrammi (salvataggio e diario scritti).

### La CI

`.github/workflows/ci.yml`, a ogni push e pull request:

| Job | Che cosa fa |
| --- | --- |
| `linux` (Ubuntu 24.04) | shellcheck e flake8, `make harness`, `make test` (senza video), `make dist`, `make test-dist`, `make test-linux`; i pacchetti come artifact |
| `windows` (Windows Server) | `tools/release/try_windows.ps1`: il programma dello zip (`--version`, `--check`, una partita di 600 fotogrammi con SDL finta, salvataggio intero e in LF), poi l'installazione silenziosa, il programma installato (anche lui una partita), la voce in «App installate» e la disinstallazione |
| `macos` (macOS arm64) | `make mac`, `tools/release/mkapp.sh`, poi `tools/release/test_app.sh`: l'app presa dal disco, firma, `--check`, una partita di 600 fotogrammi; anche in x86_64 con Rosetta, quando c'è |
| `release` | ogni volta prepara la release (`SHA256SUMS` dei sette pacchetti, le note della voce di `CHANGELOG.md` con `tools/release/notes.py`); la pubblica con un tag `vX.Y.Z` (che deve essere `GAME_VERSION`) o con *Run workflow* su `main` e *release* spuntato |

Ogni passo gira dentro `tools/release/ci_step.sh` (o `try_windows.ps1`): se fallisce, la fine del suo
output diventa anche un'annotazione di errore, che l'API dei *checks* di GitHub mostra senza il log
completo (`gh api repos/<owner>/<repo>/check-runs/<job>/annotations`). Le prove su Windows e su macOS
lasciano anche una nota (*notice*) con il sistema provato.

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

## Il gioco sul PC senza RetroArch

`platform/sdl/main.c` è un frontend libretro minimo collegato al core (gli stessi sorgenti, nessuna
modifica): il gioco è identico a quello di RetroArch, fotogramma per fotogramma. Quello che cambia da
un sistema all'altro sta in `platform/sdl/os.c` (cartelle, librerie, finestre di dialogo, lucchetto,
file con nomi UTF-8) e, nel core, in `src/plat.c` (sostituire un file in un colpo solo e forzarlo sul
disco: `rename` e `fsync`, `F_FULLFSYNC` su macOS, `MoveFileExW` e `FlushFileBuffers` su Windows). Il
core scrive i file in modo binario: gli stessi byte, con gli a capo LF, su ogni sistema.

- **SDL2 all'avvio**, mai collegata: su Linux `libSDL2-2.0.so.0` (o `libSDL2.so`); su Windows
  `SDL2.dll` accanto al programma, con il percorso intero (mai una DLL trovata altrove); su macOS
  quella dell'app (`../Frameworks/SDL2.framework/SDL2`), poi `libSDL2-2.0.0.dylib` accanto o di
  Homebrew, poi `SDL2.framework`. `DEVA_SDL2_LIB` per un altro nome. Le funzioni che usa sono in
  `sdl2_functions.h`, valori e strutture in `sdl2_api.h`, e `check_sdl2_api.sh` li tiene uguali a
  quelli veri: così il programma si compila senza gli header di SDL2 (anche con Zig per glibc 2.17 o
  per Windows) e non dipende dalla versione di SDL2 del PC di build. Serve SDL 2.0.9 o successiva
  (anche sdl2-compat su SDL3); vibrazione e tipo di gamepad (2.0.12) sono facoltativi. I pacchetti per
  Windows e macOS contengono la SDL2 ufficiale 2.32.10 (`packaging/sdl2/README.md`: da dove viene e
  le sue impronte, controllate a ogni build).
- **Dati**: `--data`, `DEVA_ADVENTURES_DATA`, su macOS `../Resources` dell'app, accanto al programma
  (il pacchetto scompattato), `<programma>/../share`; fuori da Windows anche `/usr/local/share`,
  `/usr/share`, `/opt/homebrew/share` (macOS) e il `DATA_DIR` della build. **Salvataggi**: `--saves`,
  poi `$XDG_DATA_HOME/deva-adventures` o `~/.local/share/deva-adventures` (Linux),
  `%APPDATA%\deva-adventures` (Windows), `~/Library/Application Support/deva-adventures` (macOS); lì
  anche il lucchetto che tiene aperta una copia sola (`fcntl`; su Windows il file aperto senza
  condivisione) e `deva-adventures.log` (`.log.1` la volta prima).
- **Ritmo**: un fotogramma ogni 1/60 s con il contatore di SDL, così ogni immagine resta sullo
  schermo lo stesso tempo. Il suono va in coda (`SDL_QueueAudio`) e parte quando ce n'è un pezzo
  dell'altoparlante (1024 campioni) più due fotogrammi, circa 57 ms; l'altoparlante prende un pezzo
  alla volta e non deve mai trovarne meno, quindi una coda bassa anticipa il fotogramma successivo e
  una coda cresciuta (i due orologi si allontanano) salta un battito, al massimo uno al secondo. Con il
  suono a dettare il ritmo i fotogrammi arriverebbero a gruppi, a ogni pezzo preso dall'altoparlante:
  in una simulazione più di uno su quattro non si vedrebbe mai. Il log finale conta fotogrammi,
  ritardi oltre 25 ms, fotogrammi a coppie e battiti saltati.
- **Tasti**: i tasti premuti fra due fotogrammi contano per un fotogramma (un tocco brevissimo non si
  perde). Gamepad: SDL nomina i tasti per posizione (`SDL_GAMECONTROLLER_USE_BUTTON_LABELS=0`); sui
  pad che `SDL_GameControllerGetType` dice Xbox (360 e One) la mappa segue i colori stampati, quelli
  che dice la voce (rosso B, giallo Y, verde A, blu X), sugli altri la posizione della RF35H (rosso a
  destra, giallo in basso, blu in alto, verde a sinistra: i colori di un pad stile Super Nintendo);
  `--pad colori|posizione` la sceglie a mano. Levette escluse come nel core. F11, Alt+Invio e Cmd+F
  (schermo intero) sono del programma e non arrivano al gioco; con Alt o Cmd premuto la tastiera non
  arriva al gioco (Alt+F4, Cmd+Q e le altre scorciatoie non scelgono niente per sbaglio).
- **Finestra**: pixel interi (`SDL_RenderSetLogicalSize` + `SDL_RenderSetIntegerScale`), di serie la
  più grande che sta nel 90% dello schermo; classe X11 e app id Wayland `deva-adventures`, come la
  voce di menu (`StartupWMClass`). Ridotta a icona o non davanti: niente `retro_run` e suono in pausa.
- **Messaggi per i grandi**: sul terminale; avviato dal menu (stderr non è un terminale) anche in una
  finestra di SDL, o, quando SDL2 manca, di zenity, kdialog, xmessage (Linux), `MessageBoxW`
  (Windows), `osascript` (macOS). `DEVA_NO_DIALOG=1` li tiene fuori dai test.
- **Windows**: un programma grafico (`-Wl,--subsystem,windows`, niente finestra nera) che scrive sulla
  console solo se avviato da un Prompt dei comandi o con l'uscita rediretta; manifest con la code page
  UTF-8 (Windows 10 1903+: `fopen` del core e gli argomenti in UTF-8) e la scala per monitor
  (`PerMonitorV2`: pixel nitidi), icona e versione da `packaging/windows/*.in` (`make windows` li
  riempie con `GAME_VERSION`). UCRT e winpthreads dentro il programma: accanto serve solo `SDL2.dll`.
- **macOS**: un solo programma per arm64 e x86_64 (`clang -arch arm64 -arch x86_64
  -mmacosx-version-min=10.13`); l'app (`tools/release/mkapp.sh`) ha `SDL2.framework` com'è firmato
  dal progetto SDL e una firma ad hoc di tutto il pacchetto (niente notarizzazione: serve un account
  Apple Developer).

| Uscita | Quando |
| --- | --- |
| 0 | finito: Esci, la finestra chiusa, SIGTERM, `--check` andato bene |
| 2 | un'opzione sbagliata |
| 3 | dati che non si trovano o non si caricano, salvataggi che non si possono scrivere |
| 4 | niente SDL2 (o troppo vecchia), niente schermo |
| 5 | il gioco è già aperto |

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
5. `make dist`: in `release/` i sorgenti, i due pacchetti RetroArch, il pacchetto per PC Linux
   (`-linux-x86_64.tar.gz`), lo zip e l'installazione per Windows (`-windows-x64.zip`,
   `-windows-x64-setup.exe`; l'installazione solo se c'è `makensis`) e `SHA256SUMS`. Gli archivi sono
   riproducibili: rifatti dallo stesso albero hanno gli stessi byte (anche core e programmi: Zig non vi
   scrive percorsi né date; NSIS senza le date dei file, `SetDateSave off`).
6. `make test-dist`: lo script di installazione del pacchetto x86_64 su cartelle di RetroArch finte
   (installazione nuova, aggiornamento con impostazioni e salvataggi, copie di sicurezza e ritorno
   indietro, errore a metà con ripristino, disinstallazione), con busybox se c'è; il core installato
   viene anche fatto girare dall'harness. `NO_BUSYBOX=1` usa la `sh` e gli strumenti del sistema.
7. `make test-linux`: il programma per PC e l'`install.sh` del suo pacchetto (vedi *Provare*);
   `make test-windows`: il pacchetto per Windows sotto Wine.
8. macOS, su un Mac: `make mac`, poi `tools/release/mkapp.sh` (l'app, controllata e firmata ad hoc,
   e `release/deva-adventures-<v>-macos.dmg`). Il disco non è riproducibile (`hdiutil` vi scrive date
   e identificatori): quello della release è quello della CI.
9. Il commit su `main`, poi la release, in uno dei due modi: il tag
   (`git tag -a v1.2.0 -m "Deva's Awesome Adventures 1.2.0"` e `git push origin v1.2.0`), oppure da
   GitHub, *Actions → CI → Run workflow* su `main` con *release* spuntato (il tag `v<GAME_VERSION>` lo
   fa la CI sul commit appena provato, mai sopra uno che c'è già). La CI rifà tutto sui tre sistemi e,
   se ogni job passa, pubblica la release con i pacchetti, `SHA256SUMS` e le note della voce di
   `CHANGELOG.md`. Le impronte dei pacchetti di Linux e Windows della release devono essere quelle di
   `make dist` fatto in locale dallo stesso commit.

## devaOS (Buildroot) e Lakka

- **devaOS**: `packaging/buildroot` è un br2-external.
  `make BR2_EXTERNAL=/percorso/deva-adventures/packaging/buildroot menuconfig`, poi *External options*
  → *deva-adventures*. Core in `/usr/lib/libretro` (configurabile), dati in
  `/usr/share/deva_adventures`. Richiede un toolchain con i thread (`BR2_TOOLCHAIN_HAS_THREADS`).
- **Lakka**: `packaging/lakka/README.md`.
