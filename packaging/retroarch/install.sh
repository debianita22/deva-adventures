#!/bin/sh
# Deva's Awesome Adventures - puts the core, its .info and its data into RetroArch, with a backup.
# POSIX sh (busybox ash on the handhelds). Run it from the unpacked package folder.
#
#   sh install.sh                    finds retroarch.cfg, shows the folders, asks, installs
#   sh install.sh --yes              the same without asking
#   sh install.sh --cfg FILE         another retroarch.cfg
#   sh install.sh --cores DIR --info DIR --system DIR --saves DIR    folders by hand
#   sh install.sh --dry-run          only says what it would do
#   sh install.sh --uninstall        removes core, .info and data (saves and backups stay)
#   sh install.sh --restore DIR      puts back a backup (core, .info, data; saves with --with-saves)
#   sh install.sh --list-backups     the backups made so far
#
# Before writing, what was there (core, .info, data with its settings, saves) is kept in
# <system>/deva_adventures_backup/<date-time>/. Messages are in Italian, for the grown-ups.
set -eu

VERSION="@VERSION@"
CORE=deva_adventures_libretro.so
INFO_FILE=deva_adventures_libretro.info
DATA=deva_adventures
LIBRARY_NAME="Deva's Awesome Adventures" # (retro_get_system_info)
HERE=$(cd "$(dirname "$0")" && pwd)

say() { printf '%s\n' "$*"; }
die() {
    printf 'ERRORE: %s\n' "$*" >&2
    exit 1
}

usage() { # (for the grown-ups: in Italian)
    cat <<EOF
Deva's Awesome Adventures $VERSION - installazione in RetroArch, con copia di sicurezza.
Da lanciare nella cartella del pacchetto, con sh (anche busybox).

  sh install.sh                     trova retroarch.cfg, mostra le cartelle, chiede, installa
  sh install.sh --yes               lo stesso, senza domande
  sh install.sh --dry-run           mostra soltanto che cosa farebbe
  sh install.sh --cfg FILE          un altro retroarch.cfg
  sh install.sh --cores DIR --info DIR --system DIR --saves DIR    le cartelle a mano
  sh install.sh --list-backups      le copie di sicurezza fatte finora
  sh install.sh --restore DIR       rimette una copia (core, .info, dati)
  sh install.sh --restore DIR --with-saves     ... e anche i salvataggi di allora
  sh install.sh --uninstall         toglie core, .info e dati (salvataggi e copie restano)

Prima di scrivere, quello che c'era (core, .info, dati con le impostazioni, salvataggi) va in
<cartella di sistema>/deva_adventures_backup/<data-ora>/. Tutto il resto: docs/manuale.pdf.
EOF
}

# ------------------------------------------------------------------ options
MODE=install
CFG='' CORES='' INFO='' SYSTEM='' SAVES='' YES=0 DRY=0 WITH_SAVES=0 RESTORE=''
need() { [ $# -ge 2 ] || die "$1 vuole un valore"; }
while [ $# -gt 0 ]; do
    case "$1" in
    --cfg) need "$@"; CFG=$2; shift 2 ;;
    --cores) need "$@"; CORES=$2; shift 2 ;;
    --info) need "$@"; INFO=$2; shift 2 ;;
    --system) need "$@"; SYSTEM=$2; shift 2 ;;
    --saves) need "$@"; SAVES=$2; shift 2 ;;
    -y | --yes) YES=1; shift ;;
    -n | --dry-run) DRY=1; shift ;;
    --uninstall) MODE=uninstall; shift ;;
    --restore) need "$@"; MODE=restore; RESTORE=$2; shift 2 ;;
    --with-saves) WITH_SAVES=1; shift ;;
    --list-backups) MODE=list; shift ;;
    -h | --help) usage; exit 0 ;;
    *) printf 'Opzione sconosciuta: %s\n\n' "$1" >&2; usage >&2; exit 2 ;;
    esac
done

run() { # a command that changes something (shown, not run, with --dry-run)
    if [ "$DRY" = 1 ]; then
        say "  [prova] $*"
    else
        "$@"
    fi
}

# ------------------------------------------------------------------ RetroArch's folders
find_cfg() {
    for c in "${XDG_CONFIG_HOME:-${HOME:-/root}/.config}/retroarch/retroarch.cfg" \
        /storage/.config/retroarch/retroarch.cfg /home/ark/.config/retroarch/retroarch.cfg \
        /root/.config/retroarch/retroarch.cfg /opt/retroarch/retroarch.cfg; do
        if [ -f "$c" ]; then
            printf '%s' "$c"
            return 0
        fi
    done
    return 0
}

cfg_value() { # key: its value in retroarch.cfg ("" when absent or "default")
    [ -n "$CFG" ] || return 0
    v=$(sed -n "s/^[[:space:]]*$1[[:space:]]*=[[:space:]]*\"\{0,1\}\([^\"]*\)\"\{0,1\}[[:space:]]*\$/\1/p" "$CFG" |
        tail -n 1)
    [ "$v" = default ] && v=
    printf '%s' "$v"
}

cfg_values() { # the settings of a deva_adventures.cfg: no comments, no spacing, no CR, sorted
    tr -d '\r' <"$1" | sed -e 's/#.*//' -e 's/[[:space:]]*=[[:space:]]*/=/' -e 's/^[[:space:]]*//' \
        -e 's/[[:space:]]*$//' | grep -v '^$' | sort
}

cfg_home() { # the home RetroArch's ~ means: the one its config lives in (root installing for ark)
    case "$CFG" in
    */.config/retroarch/retroarch.cfg) printf '%s' "${CFG%/.config/retroarch/retroarch.cfg}" ;;
    *) printf '%s' "${HOME:-/root}" ;;
    esac
}

expand() { # RetroArch's ~ (home) and : (its own folder: here the folder of retroarch.cfg)
    case "$1" in
    "~") cfg_home ;;
    "~"/*) printf '%s%s' "$(cfg_home)" "${1#\~}" ;;
    :/*) printf '%s' "$CFG_DIR${1#:}" ;;
    *) printf '%s' "$1" ;;
    esac
}

resolve_dirs() {
    if [ -z "$CFG" ] && { [ -z "$CORES" ] || [ -z "$SYSTEM" ]; }; then
        CFG=$(find_cfg)
    fi
    [ -z "$CFG" ] || [ -f "$CFG" ] || die "non trovo $CFG"
    CFG_DIR=
    [ -z "$CFG" ] || CFG_DIR=$(cd "$(dirname "$CFG")" && pwd)
    if [ -z "$CORES" ] || [ -z "$SYSTEM" ]; then
        [ -n "$CFG" ] || die "non trovo retroarch.cfg: indicalo con --cfg FILE, oppure dai --cores e --system"
    fi
    [ -n "$CORES" ] || CORES=$(expand "$(cfg_value libretro_directory)")
    [ -n "$CORES" ] || CORES="$CFG_DIR/cores"
    [ -n "$INFO" ] || INFO=$(expand "$(cfg_value libretro_info_path)")
    [ -n "$INFO" ] || INFO=$CORES
    [ -n "$SYSTEM" ] || SYSTEM=$(expand "$(cfg_value system_directory)")
    [ -n "$SYSTEM" ] || SYSTEM="$CFG_DIR/system"
    if [ -z "$SAVES" ] && [ "$(cfg_value savefiles_in_content_dir)" != true ]; then
        SAVES=$(expand "$(cfg_value savefile_directory)")
        # "Sort saves into folders by core name": RetroArch hands the core a folder named after it
        if [ -n "$SAVES" ] && [ "$(cfg_value sort_savefiles_enable)" = true ]; then
            SAVES="$SAVES/$LIBRARY_NAME"
        fi
    fi
    [ -n "$SAVES" ] || SAVES=$SYSTEM # (the core saves in the system folder when RetroArch gives none)
    BACKUPS="$SYSTEM/${DATA}_backup"
}

show_dirs() {
    say "  retroarch.cfg : ${CFG:-(nessuno: cartelle date a mano)}"
    say "  core          : $CORES"
    say "  .info         : $INFO"
    say "  dati          : $SYSTEM/$DATA"
    say "  salvataggi    : $SAVES"
}

ask() { # question: yes or stop
    [ "$YES" = 1 ] && return 0
    [ "$DRY" = 1 ] && return 0
    if [ ! -t 0 ]; then
        die "nessun terminale per chiedere conferma: rilancia con --yes"
    fi
    printf '%s [s/N] ' "$1"
    read -r answer || answer=
    case "$answer" in
    s | S | si | sì | SI | y | Y | yes) return 0 ;;
    *) say "Fermato, niente è cambiato."; exit 1 ;;
    esac
}

on_tmpfs() { # a folder in RAM loses its files at the next boot
    fs=$(df -P "$1" 2>/dev/null | tail -n 1 | awk '{print $1}')
    [ "$fs" = tmpfs ]
}

writable() {
    run mkdir -p "$1" || die "non posso creare $1"
    [ "$DRY" = 1 ] || [ -w "$1" ] || die "$1 è in sola lettura: scegli un'altra cartella (--cores, --info o --system)"
}

have_sha=0
if command -v sha256sum >/dev/null 2>&1; then
    have_sha=1
fi

# ------------------------------------------------------------------ backup and restore
save_files() { # the saves of the game in the saves folder
    for f in "$SAVES/$DATA".sav* "$SAVES/$DATA"_*.sav* "$SAVES/$DATA"_*.csv* "$SAVES/$DATA"_opzioni.cfg*; do
        [ -e "$f" ] && printf '%s\n' "$f"
    done
    return 0
}

make_backup() { # -> BK, with what is installed now (nothing to keep: no folder)
    BK="$BACKUPS/$(date +%Y%m%d-%H%M%S)"
    k=1
    while [ -e "$BK" ]; do # (two installs in the same second)
        k=$((k + 1))
        BK="$BACKUPS/$(date +%Y%m%d-%H%M%S)-$k"
    done
    found=0
    [ -e "$CORES/$CORE" ] && found=1
    [ -e "$INFO/$INFO_FILE" ] && found=1
    [ -d "$SYSTEM/$DATA" ] && found=1
    [ -n "$(save_files)" ] && found=1
    if [ "$found" = 0 ]; then
        BK=
        say "Nessuna versione precedente da salvare."
        return 0
    fi
    say "Copia di sicurezza in $BK"
    run mkdir -p "$BK/saves"
    [ ! -e "$CORES/$CORE" ] || run cp -p "$CORES/$CORE" "$BK/"
    [ ! -e "$INFO/$INFO_FILE" ] || run cp -p "$INFO/$INFO_FILE" "$BK/"
    save_files | while IFS= read -r f; do run cp -p "$f" "$BK/saves/"; done
    if [ -d "$SYSTEM/$DATA" ]; then
        run mv "$SYSTEM/$DATA" "$BK/$DATA" # (a move: instant on the same card)
        DATA_MOVED=1
    fi
    if [ "$DRY" = 0 ]; then
        {
            say "versione_tolta = $(old_version)"
            say "data = $(date '+%Y-%m-%d %H:%M:%S')"
            say "core = $CORES"
            say "info = $INFO"
            say "dati = $SYSTEM/$DATA"
            say "salvataggi = $SAVES"
        } >"$BK/BACKUP.txt"
    fi
}

old_version() {
    if [ -f "$INFO/$INFO_FILE" ]; then
        sed -n 's/^display_version = "\(.*\)"/\1/p' "$INFO/$INFO_FILE"
    else
        printf '?'
    fi
}

restore_from() { # backup folder: data, core, .info (and the saves with --with-saves)
    b=$1
    [ -f "$b/BACKUP.txt" ] || die "$b non è una copia di sicurezza di questo gioco (manca BACKUP.txt)"
    if [ -d "$b/$DATA" ]; then # copied beside first: a copy cut short leaves the installed data as it is
        run rm -rf "$SYSTEM/.$DATA.new" "$SYSTEM/.$DATA.old"
        run cp -pR "$b/$DATA" "$SYSTEM/.$DATA.new"
        [ ! -d "$SYSTEM/$DATA" ] || run mv "$SYSTEM/$DATA" "$SYSTEM/.$DATA.old"
        run mv "$SYSTEM/.$DATA.new" "$SYSTEM/$DATA"
        run rm -rf "$SYSTEM/.$DATA.old"
    fi
    if [ -e "$b/$CORE" ]; then
        run cp -p "$b/$CORE" "$CORES/.$CORE.new"
        run mv "$CORES/.$CORE.new" "$CORES/$CORE"
    else
        run rm -f "$CORES/$CORE"
    fi
    if [ -e "$b/$INFO_FILE" ]; then
        run cp -p "$b/$INFO_FILE" "$INFO/.$INFO_FILE.new"
        run mv "$INFO/.$INFO_FILE.new" "$INFO/$INFO_FILE"
    else
        run rm -f "$INFO/$INFO_FILE"
    fi
    if [ "$WITH_SAVES" = 1 ] && [ -d "$b/saves" ]; then
        for f in "$b"/saves/*; do
            [ -e "$f" ] && run cp -p "$f" "$SAVES/"
        done
    fi
    # Back to a version before 0.15 (it writes saves without "# fine" and knows nothing of .prev):
    # the .prev copies would later be taken for newer than its saves by 0.15 and up. Away with them.
    v=$(sed -n 's/^versione_tolta = //p' "$b/BACKUP.txt")
    case "$v" in
    0.1[5-9]* | 0.[2-9][0-9]* | [1-9]*) ;;
    *)
        for f in "$SAVES/$DATA"*.prev "$SAVES/$DATA"*.tmp; do
            [ -e "$f" ] && run rm -f "$f"
        done
        ;;
    esac
    return 0
}

# ------------------------------------------------------------------ the modes
resolve_dirs

case "$MODE" in
list)
    say "Copie di sicurezza in $BACKUPS:"
    if [ -d "$BACKUPS" ]; then
        for b in "$BACKUPS"/*; do
            [ -f "$b/BACKUP.txt" ] || continue
            say "  $b  (versione $(sed -n 's/^versione_tolta = //p' "$b/BACKUP.txt"))"
        done
    fi
    exit 0
    ;;
uninstall)
    say "Deva's Awesome Adventures: disinstallazione"
    show_dirs
    say "Restano i salvataggi e le copie di sicurezza ($BACKUPS)."
    ask "Tolgo core, .info e dati?"
    run rm -f "$CORES/$CORE" "$INFO/$INFO_FILE"
    run rm -rf "${SYSTEM:?}/$DATA"
    say "Fatto. Riavvia RetroArch."
    exit 0
    ;;
restore)
    case "$RESTORE" in /*) ;; *) RESTORE="$(pwd)/$RESTORE" ;; esac
    say "Deva's Awesome Adventures: ripristino da $RESTORE"
    show_dirs
    [ "$WITH_SAVES" = 1 ] && say "Anche i salvataggi tornano come nella copia."
    ask "Rimetto la versione della copia?"
    restore_from "$RESTORE"
    say "Fatto. Riavvia RetroArch."
    exit 0
    ;;
esac

# ------------------------------------------------------------------ install
say "Deva's Awesome Adventures $VERSION: installazione"
if [ ! -f "$HERE/cores/$CORE" ] || [ ! -f "$HERE/info/$INFO_FILE" ] || [ ! -d "$HERE/system/$DATA" ]; then
    die "pacchetto incompleto: accanto a install.sh servono cores/, info/ e system/"
fi
if [ "$have_sha" = 1 ]; then
    (cd "$HERE" && sha256sum -c SHA256SUMS >/dev/null 2>&1) ||
        die "il pacchetto non corrisponde a SHA256SUMS (file rovinato o modificato): scaricalo di nuovo"
    say "Pacchetto verificato (SHA256)."
else
    say "(sha256sum non c'è: salto la verifica del pacchetto)"
fi
show_dirs
if [ -e "$INFO/$INFO_FILE" ]; then
    say "  già installata: versione $(old_version)"
fi
for d in "$CORES" "$SYSTEM"; do
    if [ -d "$d" ] && on_tmpfs "$d"; then
        say "ATTENZIONE: $d è in RAM (tmpfs): al riavvio i file spariscono. Meglio una cartella sulla scheda."
    fi
done
ask "Procedo?"

writable "$CORES"
writable "$INFO"
writable "$SYSTEM"

# The settings of the grown-ups stay theirs when their values differ from the new defaults; a file
# with the default values (only its comments older) gives way to the new one.
KEEP_CFG=0
OLD_CFG="$SYSTEM/$DATA/$DATA.cfg"
if [ -f "$OLD_CFG" ] && [ "$(cfg_values "$OLD_CFG")" != "$(cfg_values "$HERE/system/$DATA/$DATA.cfg")" ]; then
    KEEP_CFG=1
fi

STAGE="$SYSTEM/.$DATA.new"
HAD_CORE=0 HAD_INFO=0
[ -e "$CORES/$CORE" ] && HAD_CORE=1
[ -e "$INFO/$INFO_FILE" ] && HAD_INFO=1
BK='' DATA_MOVED=0 DATA_IN=0 CORE_IN=0 INFO_IN=0 DONE=0
rollback() { # the install stopped halfway (an error, a full card, Ctrl-C): back to what was there
    set +e
    [ "$DONE" = 1 ] && return 0
    [ "$DRY" = 1 ] && return 0
    printf 'Qualcosa è andato storto: rimetto com'"'"'era.\n' >&2
    rm -rf "$STAGE" "$CORES/.$CORE.new" "$INFO/.$INFO_FILE.new"
    [ "$DATA_IN" = 1 ] && rm -rf "${SYSTEM:?}/$DATA"
    [ "$DATA_MOVED" = 1 ] && [ ! -e "$SYSTEM/$DATA" ] && mv "$BK/$DATA" "$SYSTEM/$DATA"
    if [ "$CORE_IN" = 1 ]; then
        if [ "$HAD_CORE" = 1 ]; then cp -p "$BK/$CORE" "$CORES/$CORE"; else rm -f "$CORES/$CORE"; fi
    fi
    if [ "$INFO_IN" = 1 ]; then
        if [ "$HAD_INFO" = 1 ]; then cp -p "$BK/$INFO_FILE" "$INFO/$INFO_FILE"; else rm -f "$INFO/$INFO_FILE"; fi
    fi
    return 0
}
trap rollback EXIT
trap 'exit 130' INT TERM HUP

# the new data next to the old, complete, before anything is touched
run rm -rf "$STAGE"
run cp -R "$HERE/system/$DATA" "$STAGE"

make_backup

# data: the new folder takes the name, with the grown-ups' settings if they are theirs
run mv "$STAGE" "$SYSTEM/$DATA"
DATA_IN=1
KEPT_CFG=0
if [ "$KEEP_CFG" = 1 ]; then
    run cp -p "$HERE/system/$DATA/$DATA.cfg" "$SYSTEM/$DATA/$DATA.cfg.default"
    if [ "$DRY" = 1 ]; then
        run cp -p "$OLD_CFG" "$SYSTEM/$DATA/$DATA.cfg"
    else
        run cp -p "$BK/$DATA/$DATA.cfg" "$SYSTEM/$DATA/$DATA.cfg"
    fi
    KEPT_CFG=1
fi
# core and .info: copied beside, then renamed over the old ones (never half a core)
run cp "$HERE/cores/$CORE" "$CORES/.$CORE.new"
CORE_IN=1
run mv "$CORES/.$CORE.new" "$CORES/$CORE"
run cp "$HERE/info/$INFO_FILE" "$INFO/.$INFO_FILE.new"
INFO_IN=1
run mv "$INFO/.$INFO_FILE.new" "$INFO/$INFO_FILE"

# what is installed is what the package holds
if [ "$have_sha" = 1 ] && [ "$DRY" = 0 ]; then
    LIST="$SYSTEM/$DATA/.verifica"
    {
        sed -n "s|^\([0-9a-f]*\)  cores/$CORE\$|\1  $CORES/$CORE|p" "$HERE/SHA256SUMS"
        sed -n "s|^\([0-9a-f]*\)  info/$INFO_FILE\$|\1  $INFO/$INFO_FILE|p" "$HERE/SHA256SUMS"
        sed -n "s|^\([0-9a-f]*\)  system/$DATA/|\1  $SYSTEM/$DATA/|p" "$HERE/SHA256SUMS" |
            if [ "$KEPT_CFG" = 1 ]; then grep -v "/$DATA/$DATA.cfg\$"; else cat; fi
    } >"$LIST"
    n=$(wc -l <"$LIST" | tr -d ' ')
    if ! sha256sum -c "$LIST" >/dev/null 2>&1; then
        rm -f "$LIST"
        die "i file copiati non corrispondono al pacchetto (scheda piena o rovinata?)"
    fi
    rm -f "$LIST"
    say "Installazione verificata: $n file uguali al pacchetto."
fi

if [ "$DRY" = 0 ]; then
    {
        say "Deva's Awesome Adventures $VERSION"
        say "installato il $(date '+%Y-%m-%d %H:%M:%S')"
        say "core = $CORES/$CORE"
        say "info = $INFO/$INFO_FILE"
        say "salvataggi = $SAVES"
        say "copia di sicurezza = ${BK:-nessuna (prima installazione)}"
    } >"$SYSTEM/$DATA/INSTALLATO.txt"
fi
DONE=1
sync 2>/dev/null || true

say ""
say "Fatto: Deva's Awesome Adventures $VERSION è installato."
[ "$KEPT_CFG" = 1 ] && say "Le tue impostazioni ($DATA.cfg) sono rimaste; quelle nuove di serie sono in $DATA.cfg.default."
[ -n "$BK" ] && say "Per tornare indietro: sh install.sh --restore \"$BK\""
say "Riavvia RetroArch, poi: Contentless Cores (Nuclei senza contenuto) -> Deva's Awesome Adventures,"
say "oppure: Load Core (Carica core) -> Deva's Awesome Adventures -> Start Core (Avvia core)."
