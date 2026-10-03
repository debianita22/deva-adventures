# Deva's Awesome Adventures in Lakka

`deva_adventures/package.mk` compila il core con il toolchain di Lakka (il sistema di build di
LibreELEC) e lo mette nell'immagine:

| Che cosa | Dove nell'immagine | Come lo vede RetroArch |
| --- | --- | --- |
| `deva_adventures_libretro.so` e `.info` | `/usr/lib/libretro/` | `/tmp/cores/` (overlay con `/storage/cores`) |
| dati del gioco (circa 16,5 MB) | `/usr/share/deva_adventures/` | la cartella di riserva del core (`DATA_DIR`) |

Il core è contentless: RetroArch lo elenca in *Contentless Cores* («Nuclei senza contenuto» con i
menu in italiano; il suo `.info` lo dichiara `single_purpose`, quindi compare anche con la scelta di
serie *Uso singolo*), oppure *Load Core* → *Start Core*.
Salvataggi, log e diario vanno nella cartella dei salvataggi di RetroArch (`/storage/savefiles`).

## Passi

```sh
# nell'albero di Lakka (Lakka-LibreELEC), con questo sorgente accanto
cp -r ../deva-adventures/packaging/lakka/deva_adventures packages/lakka/libretro_cores/
# aggiungi deva_adventures alla lista LIBRETRO_CORES (una riga "deva_adventures \" nell'elenco)
cat packages/lakka/libretro_cores/package.mk | grep -n "LIBRETRO_CORES="
# oppure, se costruisci una lista tua: CUSTOM_LIBRETRO_CORES="... deva_adventures" (sostituisce l'elenco)
# solo questo pacchetto, per provarlo
PROJECT=<progetto> DEVICE=<device> ARCH=aarch64 scripts/build deva_adventures
# poi l'immagine come sempre
PROJECT=<progetto> DEVICE=<device> ARCH=aarch64 make image
```

Il sorgente viene preso da `${ROOT}/../deva-adventures` (la cartella accanto all'albero di Lakka);
per un altro percorso: `DEVA_ADVENTURES_SRC=/percorso/deva-adventures` nell'ambiente della build.

## Su un Lakka già installato, senza rifare l'immagine

Il pacchetto RetroArch (`deva-adventures-<versione>-aarch64.zip`) con il suo `install.sh`: su Lakka
trova `/storage/.config/retroarch/retroarch.cfg`, copia il core in `/tmp/cores` (che scrive in
`/storage/cores`, persistente) e i dati in `/tmp/system` (cioè `/storage/system`).
