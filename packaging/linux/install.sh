#!/bin/sh
# Deva's Awesome Adventures - installs the PC version (the game on its own, no RetroArch).
# POSIX sh. Run it from the unpacked package folder. Messages are in Italian, for the grown-ups.
#
#   ./install.sh                  installs or updates for this user (in ~/.local), after asking
#   ./install.sh --yes            the same without asking
#   ./install.sh --dry-run        only says what it would do
#   ./install.sh --prefix DIR     elsewhere; for every user: sudo ./install.sh --prefix /usr/local
#   ./install.sh --uninstall      removes it (the saves stay); --prefix DIR if it went elsewhere
#   <prefix>/lib/deva-adventures/install.sh --uninstall      the same, from the installed copy
#
# Program and data go to <prefix>/lib/deva-adventures (a copy of the package: the program finds its
# data next to itself), with a link in <prefix>/bin, the menu entry in <share>/applications and the
# icon in <share>/icons/hicolor/256x256/apps (<share>: $XDG_DATA_HOME or ~/.local/share; with
# --prefix, <prefix>/share). The saves belong to the program ($XDG_DATA_HOME/deva-adventures, or
# ~/.local/share/deva-adventures): before an update they are copied to <saves>/copie/<date-time>/.
set -eu

VERSION="@VERSION@"
APP=deva-adventures
DATA=deva_adventures
HERE=$(cd "$(dirname "$0")" && pwd)

say() { printf '%s\n' "$*"; }
die() {
    printf 'ERRORE: %s\n' "$*" >&2
    exit 1
}

usage() { # (for the grown-ups: in Italian)
    cat <<EOF
Deva's Awesome Adventures $VERSION - installazione sul PC Linux (senza RetroArch).

  ./install.sh                  installa o aggiorna per il tuo utente (in ~/.local), dopo aver chiesto
  ./install.sh --yes            lo stesso, senza domande
  ./install.sh --dry-run        mostra soltanto che cosa farebbe
  ./install.sh --prefix DIR     in un'altra cartella; per tutti gli utenti del PC:
                                sudo ./install.sh --prefix /usr/local
  ./install.sh --uninstall      toglie il gioco (i salvataggi restano); con --prefix DIR se era altrove
                                (dalla copia installata basta: .../lib/deva-adventures/install.sh --uninstall)

Senza installare niente: ./deva-adventures in questa cartella. Tutto il resto: LEGGIMI.txt.
EOF
}

# ------------------------------------------------------------------ options
MODE=install YES=0 DRY=0 PREFIX=''
need() { [ $# -ge 2 ] || die "$1 vuole un valore"; }
while [ $# -gt 0 ]; do
    case "$1" in
    -y | --yes) YES=1; shift ;;
    -n | --dry-run) DRY=1; shift ;;
    --prefix) need "$@"; PREFIX=$2; shift 2 ;;
    --uninstall) MODE=uninstall; shift ;;
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

ask() { # question: yes or stop
    [ "$YES" = 1 ] && return 0
    [ "$DRY" = 1 ] && return 0
    [ -t 0 ] || die "nessun terminale per chiedere conferma: rilancia con --yes"
    printf '%s [s/N] ' "$1"
    read -r answer || answer=
    case "$answer" in
    s | S | si | sì | SI | y | Y | yes) return 0 ;;
    *) say "Fermato, niente è cambiato."; exit 1 ;;
    esac
}

# ------------------------------------------------------------------ the folders
[ -n "${HOME:-}" ] || die "manca la variabile HOME"
ME=$(id -u)
SYSTEMWIDE=0 # root with --prefix: for every user, whose saves are their own business
if [ -z "$PREFIX" ] && [ "$MODE" = uninstall ]; then # the installed copy knows where it went
    case "$HERE" in
    "$HOME/.local/lib/$APP") ;;
    */lib/"$APP") PREFIX=${HERE%/lib/"$APP"} ;;
    esac
fi
if [ -n "$PREFIX" ]; then
    [ "$ME" != 0 ] || SYSTEMWIDE=1
    case "$PREFIX" in /*) ;; *) PREFIX="$(pwd)/$PREFIX" ;; esac
    while [ "${PREFIX%/}" != "$PREFIX" ]; do PREFIX=${PREFIX%/}; done
    [ -n "$PREFIX" ] || die "--prefix /: scegli una cartella come /usr/local"
    SHARE="$PREFIX/share"
else
    if [ "$ME" = 0 ] && [ -z "${DEVA_INSTALL_AS_ROOT:-}" ]; then # (the tests run as root)
        die "non serve sudo: il gioco si installa per il tuo utente (rilancia ./install.sh senza sudo). Per tutti gli utenti del PC: sudo ./install.sh --prefix /usr/local"
    fi
    PREFIX="$HOME/.local"
    case "${XDG_DATA_HOME:-}" in /*) SHARE=$XDG_DATA_HOME ;; *) SHARE="$HOME/.local/share" ;; esac
fi
LIB="$PREFIX/lib/$APP"
BIN="$PREFIX/bin/$APP"
MENU="$SHARE/applications/$APP.desktop"
ICON="$SHARE/icons/hicolor/256x256/apps/$APP.png"
# where the program keeps the saves (platform/sdl/main.c: $XDG_DATA_HOME only when it is absolute)
case "${XDG_DATA_HOME:-}" in /*) SAVES="$XDG_DATA_HOME/$APP" ;; *) SAVES="$HOME/.local/share/$APP" ;; esac

show_dirs() {
    say "  programma e dati : $LIB"
    say "  comando          : $BIN"
    say "  menu             : $MENU"
    say "  icona            : $ICON"
    if [ "$SYSTEMWIDE" = 0 ]; then
        say "  salvataggi       : $SAVES (restano dove sono)"
    else
        say "  salvataggi       : quelli di ogni utente, in ~/.local/share/$APP"
    fi
}

have_sha=0
command -v sha256sum >/dev/null 2>&1 && have_sha=1

running() { # the game open (by anyone): its files must not change under it
    command -v pgrep >/dev/null 2>&1 || return 1
    pgrep -x "$APP" >/dev/null 2>&1
}

installed_version() {
    v=$("$LIB/$APP" --version 2>/dev/null | sed -n "s/^$APP //p") || v=
    printf '%s' "${v:-?}"
}

ours() { # the folder holds this game (or nothing)
    [ ! -e "$LIB" ] || [ -f "$LIB/$DATA/gfx/atlas.txt" ]
}

cfg_values() { # the settings of a deva_adventures.cfg: no comments, no spacing, no CR, sorted
    tr -d '\r' <"$1" | sed -e 's/#.*//' -e 's/[[:space:]]*=[[:space:]]*/=/' -e 's/^[[:space:]]*//' \
        -e 's/[[:space:]]*$//' | grep -v '^$' | sort
}

save_files() { # the saves of the game
    for f in "$SAVES/$DATA".sav* "$SAVES/$DATA"_*.sav* "$SAVES/$DATA"_*.csv* "$SAVES/$DATA"_opzioni.cfg*; do
        [ -e "$f" ] && printf '%s\n' "$f"
    done
    return 0
}

desktop_exec() { # the program's path as Exec= of a .desktop file wants it, or failure
    case "$1" in
    *[\"\`\$\\%]* | *'
'*) return 1 ;; # (these would need escapes the menus do not all read alike)
    *[!A-Za-z0-9/._+-]*) printf '"%s"' "$1" ;;
    *) printf '%s' "$1" ;;
    esac
}

in_path() {
    case ":${PATH:-}:" in *":$1:"*) return 0 ;; esac
    return 1
}

refresh_menus() { # (the desktops mostly notice by themselves; these help the others)
    if command -v update-desktop-database >/dev/null 2>&1; then
        update-desktop-database -q "$SHARE/applications" 2>/dev/null || true
    fi
    [ -d "$SHARE/icons/hicolor" ] && touch "$SHARE/icons/hicolor" 2>/dev/null
    return 0
}

# ------------------------------------------------------------------ uninstall
if [ "$MODE" = uninstall ]; then
    say "Deva's Awesome Adventures: disinstallazione"
    show_dirs
    if [ ! -e "$LIB" ] && [ ! -e "$MENU" ] && [ ! -L "$BIN" ] && [ ! -e "$ICON" ]; then
        say "Non è installato qui: niente da togliere."
        exit 0
    fi
    ours || die "$LIB non sembra questo gioco: non lo tocco"
    if [ "$DRY" = 0 ] && running; then
        die "il gioco è aperto: chiudilo e rilancia"
    fi
    [ -e "$LIB" ] && say "  versione         : $(installed_version)"
    ask "Tolgo il gioco?"
    # the grown-ups' settings, when they changed them, go with the saves (the program dies with $LIB)
    CFG="$LIB/$DATA/$DATA.cfg"
    if [ "$SYSTEMWIDE" = 0 ] && [ "$have_sha" = 1 ] && [ -f "$CFG" ] && [ -f "$LIB/SHA256SUMS" ]; then
        shipped=$(sed -n "s|^\([0-9a-f]*\)  $DATA/$DATA.cfg\$|\1|p" "$LIB/SHA256SUMS")
        now=$(sha256sum "$CFG" | cut -d' ' -f1)
        if [ "$shipped" != "$now" ]; then
            KEPT="$SAVES/$DATA.cfg.$(date +%Y%m%d-%H%M%S)"
            run mkdir -p "$SAVES"
            run cp -p "$CFG" "$KEPT"
            say "Le tue impostazioni ($DATA.cfg) restano in $KEPT."
        fi
    fi
    if [ -L "$BIN" ]; then
        target=$(readlink "$BIN" 2>/dev/null) || target="$LIB/$APP"
        [ "$target" != "$LIB/$APP" ] || run rm -f "$BIN"
    fi
    run rm -f "$MENU" "$ICON"
    run rm -rf "${LIB:?}"
    [ "$DRY" = 1 ] || refresh_menus
    say "Fatto: il gioco è stato tolto."
    [ "$SYSTEMWIDE" = 1 ] || say "I salvataggi sono rimasti in $SAVES (cancellali a mano se non servono più)."
    exit 0
fi

# ------------------------------------------------------------------ install
say "Deva's Awesome Adventures $VERSION: installazione sul PC"
if [ ! -f "$HERE/$APP" ] || [ ! -f "$HERE/$DATA/gfx/atlas.txt" ] || [ ! -f "$HERE/SHA256SUMS" ]; then
    die "pacchetto incompleto: accanto a install.sh servono $APP, la cartella $DATA e SHA256SUMS"
fi
case "$(uname -m)" in
x86_64 | amd64) ;;
*) die "questo pacchetto è per i PC a 64 bit Intel o AMD (x86_64); questo computer è $(uname -m)" ;;
esac
if [ "$have_sha" = 1 ]; then
    (cd "$HERE" && sha256sum -c SHA256SUMS >/dev/null 2>&1) ||
        die "il pacchetto non corrisponde a SHA256SUMS (file rovinato o modificato): scaricalo di nuovo"
    say "Pacchetto verificato (SHA256)."
else
    say "(sha256sum non c'è: salto la verifica del pacchetto)"
fi
EXEC=$(desktop_exec "$LIB/$APP") ||
    die "il percorso $LIB contiene caratteri che i menu non accettano (\" \` \$ \\ %): scegli un'altra cartella con --prefix"
ours || die "$LIB c'è già e non è questo gioco: non lo tocco (scegli un'altra cartella con --prefix)"
[ "$HERE" != "$LIB" ] || die "questa è la copia già installata: per aggiornare lancia install.sh del pacchetto nuovo"
show_dirs
[ -e "$LIB/$APP" ] && say "  già installata   : versione $(installed_version)"
if [ "$DRY" = 0 ] && running; then
    die "il gioco è aperto: chiudilo e rilancia"
fi
ask "Procedo?"

NEW="$PREFIX/lib/.$APP.new" OLD="$PREFIX/lib/.$APP.old"
HAD_LIB=0 HAD_BIN=0 HAD_MENU=0 HAD_ICON=0
[ -e "$LIB" ] && HAD_LIB=1
{ [ -e "$BIN" ] || [ -L "$BIN" ]; } && HAD_BIN=1
[ -e "$MENU" ] && HAD_MENU=1
[ -e "$ICON" ] && HAD_ICON=1
OLD_IN=0 NEW_IN=0 DONE=0
rollback() { # the install stopped halfway (an error, a full disk, Ctrl-C): back to what was there
    set +e
    [ "$DONE" = 1 ] && return 0
    [ "$DRY" = 1 ] && return 0
    printf 'Qualcosa è andato storto: rimetto com'"'"'era.\n' >&2
    [ "$NEW_IN" = 1 ] && rm -rf "${LIB:?}"
    rm -rf "$NEW"
    [ "$OLD_IN" = 1 ] && [ ! -e "$LIB" ] && mv "$OLD" "$LIB"
    [ "$HAD_BIN" = 0 ] && [ -L "$BIN" ] && rm -f "$BIN"
    [ "$HAD_MENU" = 0 ] && rm -f "$MENU"
    [ "$HAD_ICON" = 0 ] && rm -f "$ICON"
    rm -f "$MENU.new" "$ICON.new"
    return 0
}
trap rollback EXIT
trap 'exit 130' INT TERM HUP

run mkdir -p "$PREFIX/lib" "$PREFIX/bin" "$SHARE/applications" "$SHARE/icons/hicolor/256x256/apps"
[ "$DRY" = 1 ] || [ -w "$PREFIX/lib" ] || die "non posso scrivere in $PREFIX/lib"

# the package, file by file as SHA256SUMS lists it, next to the installed one, then checked
copy_package() {
    sed -n 's/^[0-9a-f]\{64\}  //p' "$HERE/SHA256SUMS" | while IFS= read -r f; do
        mkdir -p "$1/$(dirname "$f")"
        cp "$HERE/$f" "$1/$f"
    done
    cp "$HERE/SHA256SUMS" "$1/SHA256SUMS"
}
run rm -rf "$NEW" "$OLD"
run copy_package "$NEW"
if [ "$DRY" = 0 ]; then
    if [ "$have_sha" = 1 ]; then
        (cd "$NEW" && sha256sum -c SHA256SUMS >/dev/null 2>&1) ||
            die "i file copiati non corrispondono al pacchetto (disco pieno o rovinato?)"
    fi
    n=$(grep -c . "$NEW/SHA256SUMS")
    say "Copiati e verificati $n file."
fi

# The settings of the grown-ups stay theirs when their values differ from the new defaults; a file
# with the default values (only its comments older) gives way to the new one.
KEPT_CFG=0
if [ -f "$LIB/$DATA/$DATA.cfg" ] &&
    [ "$(cfg_values "$LIB/$DATA/$DATA.cfg")" != "$(cfg_values "$HERE/$DATA/$DATA.cfg")" ]; then
    run cp "$HERE/$DATA/$DATA.cfg" "$NEW/$DATA/$DATA.cfg.default"
    run cp -p "$LIB/$DATA/$DATA.cfg" "$NEW/$DATA/$DATA.cfg"
    KEPT_CFG=1
fi

# an update: the saves as they are now, aside (the game keeps its own .prev copies too)
BK=''
if [ "$HAD_LIB" = 1 ] && [ "$SYSTEMWIDE" = 0 ] && [ -n "$(save_files)" ]; then
    BK="$SAVES/copie/$(date +%Y%m%d-%H%M%S)"
    k=1
    while [ -e "$BK" ]; do
        k=$((k + 1))
        BK="$SAVES/copie/$(date +%Y%m%d-%H%M%S)-$k"
    done
    run mkdir -p "$BK"
    save_files | while IFS= read -r f; do run cp -p "$f" "$BK/"; done
    say "Salvataggi copiati in $BK"
fi

# the new folder takes the name of the old one
if [ "$HAD_LIB" = 1 ]; then
    run mv "$LIB" "$OLD"
    OLD_IN=1
fi
run mv "$NEW" "$LIB"
NEW_IN=1

# the command, the menu entry (with the full path: ~/.local/bin is not always in the PATH of the
# menus) and the icon
if [ -e "$BIN" ] && [ ! -L "$BIN" ]; then
    say "C'è già un file $BIN che non è un collegamento: lo lascio com'è (il menu usa il percorso completo)."
else
    run ln -sf "$LIB/$APP" "$BIN"
fi
write_menu() {
    awk -v exe="$EXEC" -v tryexec="$LIB/$APP" -v icon="$ICON" '
        /^TryExec=/ { print "TryExec=" tryexec; next }
        /^Exec=deva-adventures/ { sub(/^Exec=deva-adventures/, ""); print "Exec=" exe $0; next }
        /^Icon=/ { print "Icon=" icon; next }
        { print }' "$LIB/$APP.desktop" >"$MENU.new"
    chmod 644 "$MENU.new"
    mv "$MENU.new" "$MENU"
}
run write_menu
run cp "$LIB/$APP.png" "$ICON.new"
run mv "$ICON.new" "$ICON"

# the installed program, asked whether it finds its data and SDL2
if [ "$DRY" = 0 ]; then
    if out=$(DEVA_NO_DIALOG=1 "$LIB/$APP" --check 2>&1); then
        say "Controllo: $out"
    else
        rc=$?
        if [ "$rc" = 4 ]; then
            say ""
            say "ATTENZIONE: il gioco è installato, ma per partire gli manca la libreria SDL2:"
            say "  ${out#"$APP": }"
        else
            die "il programma installato non parte (codice $rc): $out"
        fi
    fi
fi
DONE=1
run rm -rf "$OLD"
[ "$DRY" = 1 ] || refresh_menus

say ""
say "Fatto: Deva's Awesome Adventures $VERSION è installato."
[ "$KEPT_CFG" = 1 ] && say "Le tue impostazioni ($DATA.cfg) sono rimaste; quelle nuove di serie sono in $DATA.cfg.default."
say "Si avvia dal menu delle applicazioni (Giochi): Deva's Awesome Adventures, a schermo intero"
say "(con il tasto destro: Gioca in una finestra). Da un terminale:"
if in_path "$PREFIX/bin"; then
    say "  $APP"
else
    say "  $LIB/$APP"
fi
say "Per i grandi: Q e W tenuti 2 secondi = L e R (Opzioni, Salvataggi, Esci); F11 = schermo intero"
say "o finestra; Esc tenuto = pausa. Il resto è in $LIB/LEGGIMI.txt."
[ -n "$BK" ] && say "I salvataggi di prima sono copiati in $BK."
if [ "$SYSTEMWIDE" = 1 ]; then
    say "Per disinstallare: sudo $LIB/install.sh --uninstall"
else
    say "Per disinstallare: $LIB/install.sh --uninstall"
fi
