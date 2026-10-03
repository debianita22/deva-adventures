# Deva's Awesome Adventures

Un gioco educativo per bambini di cinque anni, in italiano e tutto a voce, per **RetroArch**: un core
libretro «senza contenuto» in C99, pensato per la console XiFan RF35H (RK3326) e per qualsiasi RetroArch
su Linux arm64 o x86_64.

<p>
<img src="docs/img/menu.png" width="32%" alt="Il menu principale">
<img src="docs/img/sfida.png" width="32%" alt="Una sfida con il mostro">
<img src="docs/img/sentiero.png" width="32%" alt="Il sentiero, il primo coding">
</p>

- **Diciotto giochi** con livelli da 1 a 5 che si adattano da soli e l'aiuto guidato dopo due errori:
  numeri, monete, misure, parole, lettere, forme, ombre, sequenze, ritmo, memoria, posizioni, emozioni,
  ballo, ginnastica, trucchi, primo coding.
- **Quattro avventure** con la mappa, le sfide e un gesto gentile che fa tornare buono il cattivo, poi una
  festa finale; camerino dei trucchi, album, tre profili.
- **Tutto a voce** (923 frasi): non serve saper leggere. Personaggi animati, musiche ed effetti originali.
- **A prova di bambina**: opzioni, salvataggi e uscita si aprono solo tenendo premuti L e R; sessioni a
  tempo che finiscono con la buonanotte; salvataggi che reggono uno spegnimento a metà.
- **Per i grandi**: un resoconto delle partite (`tools/report/report.py`) e il kit per la prima prova.
- Niente rete, pubblicità o acquisti: tutto resta sulla console.

## Installare

| Pacchetto | Per |
| --- | --- |
| `deva-adventures-1.0.0-aarch64.zip` | console Linux arm64 con RetroArch (glibc ≥ 2.17) |
| `deva-adventures-1.0.0-x86_64.zip` | PC Linux con RetroArch (glibc ≥ 2.17) |
| `deva-adventures-1.0.0-src.tar.gz` | i sorgenti: devaOS (Buildroot), Lakka, altre architetture |

```sh
unzip deva-adventures-1.0.0-aarch64.zip
cd deva-adventures-1.0.0-aarch64
sh install.sh --dry-run    # mostra le cartelle e che cosa farebbe
sh install.sh              # copia di sicurezza, installazione, verifica dei file copiati
```

Poi in RetroArch: *Contentless Cores* → **Deva's Awesome Adventures**. Installazione a mano, devaOS,
Lakka, aggiornamenti e ritorno alla versione di prima: capitolo 1 e 12 del manuale.

## Documentazione

| | |
| --- | --- |
| [`docs/manuale.pdf`](docs/manuale.pdf) ([HTML](docs/manuale.html)) | il manuale per i genitori: installazione, giochi, avventure, opzioni, file, problemi e soluzioni |
| [`docs/playtest/scheda_osservazione.pdf`](docs/playtest/scheda_osservazione.pdf) | la prima prova: la guida per chi guarda e la scheda da stampare |
| [`docs/SVILUPPO.md`](docs/SVILUPPO.md) | compilare, provare, modificare e rilasciare: build, test, harness, grafica, audio, voce, architettura |
| [`CHANGELOG.md`](CHANGELOG.md) | le novità di ogni versione |
| [`THIRD_PARTY.md`](THIRD_PARTY.md) | i componenti di terzi e le licenze, voce compresa |
| [`packaging/`](packaging) | lo script di installazione per RetroArch, il pacchetto Buildroot (devaOS) e quello per Lakka |

## Compilare

```sh
make              # il core per il PC: build/host/deva_adventures_libretro.so
make test         # 16 partite automatiche e i salvataggi interrotti
make dist         # i pacchetti di rilascio in release/ (core aarch64 e x86_64 con Zig)
```

Requisiti, test su ARM con qemu, sanitizer e tutto il resto: [`docs/SVILUPPO.md`](docs/SVILUPPO.md).

## Licenza

Il codice è sotto licenza MIT ([`LICENSE`](LICENSE)); disegni, musiche, storie e personaggi sono
originali, generati dagli script del repository. La voce è sintetizzata con il modello Piper
*it_IT paola*: va bene per l'uso in famiglia, ma prima di pubblicare il gioco conviene sostituirla
(perché, e come: [`THIRD_PARTY.md`](THIRD_PARTY.md)).
