#!/bin/sh
# The PC program (platform/sdl) on a virtual X screen, then the install.sh of the PC package (1.1).
#   tools/harness/test_linux.sh               (make test-linux)
#
# 1. without a window: options, exit codes, the messages for the grown-ups, where it finds its data;
# 2. on a virtual screen (Xvfb with openbox: full screen and focus need a window manager; xdotool,
#    xwininfo, xprop and ImageMagick): the window and its picture, the keys through the first screens,
#    the pause held, the window minimized, full screen, one copy at a time, Alt+F4 and SIGTERM with the
#    saves complete; a virtual gamepad (fake_pad.c, needs the SDL2 headers) through the same screens,
#    by position and by colour (--pad colori, what an Xbox pad gets), and pulled out; the pace with and
#    without sound;
# 3. install.sh of release/deva-adventures-<v>-linux-x86_64.tar.gz (make dist) in a fake home: install,
#    update keeping the grown-ups' settings and copying the saves, a broken package, a failure halfway,
#    the game open, a folder with a space, uninstall; with dash and busybox too.
# Parts whose tools or package are missing are skipped (and counted). Output in build/test/linux.
set -u
cd "$(dirname "$0")/../.." || exit 1
BIN=${BIN:-build/host/deva-adventures}
OUT=${OUT:-build/test}/linux
V=$(sed -n 's/^#define GAME_VERSION "\(.*\)"/\1/p' src/common.h)
TITLE="Deva's Awesome Adventures"
rm -rf "$OUT"
mkdir -p "$OUT"
ABS=$(cd "$OUT" && pwd)
ROOT=$(pwd)
PASS=0 FAIL=0 SKIP=0
ok() { PASS=$((PASS + 1)); printf '  ok    %s\n' "$*"; }
ko() { FAIL=$((FAIL + 1)); printf '  FAIL  %s\n' "$*"; }
skip() { SKIP=$((SKIP + 1)); printf '  skip  %s\n' "$*"; }
expect() { # description, exit code wanted, command...: output in $OUT/last.txt
    d=$1 want=$2
    shift 2
    "$@" >"$OUT/last.txt" 2>&1
    got=$?
    if [ "$got" = "$want" ]; then
        ok "$d"
    else
        ko "$d (exit $got, wanted $want)"
        sed 's/^/          /' "$OUT/last.txt" | head -8
    fi
}
has() { # description, pattern, file
    if grep -q -- "$2" "$3" 2>/dev/null; then ok "$1"; else ko "$1 (no \"$2\" in $3)"; fi
}
hasnt() {
    if grep -q -- "$2" "$3" 2>/dev/null; then ko "$1 (\"$2\" in $3)"; else ok "$1"; fi
}
present() { # files...: all there
    for f; do [ -e "$f" ] || [ -L "$f" ] || return 1; done
}
absent() { # files...: none there
    for f; do
        if [ -e "$f" ] || [ -L "$f" ]; then return 1; fi
    done
    return 0
}
yes_if() { # description, condition (a command)
    d=$1
    shift
    if "$@"; then ok "$d"; else ko "$d"; fi
}
export DEVA_NO_DIALOG=1 SDL_AUDIODRIVER=dummy
unset DEVA_ADVENTURES_DATA DEVA_SDL2_LIB DEVA_SEED

[ -x "$BIN" ] || { echo "no $BIN: make linux first"; exit 1; }

# ------------------------------------------------------------------ 1. without a window
echo "== the program without a window ($BIN)"
./tools/harness/check_sdl2_api.sh >"$OUT/api.txt" 2>&1
case "$?:$(cat "$OUT/api.txt")" in
0:*skipped*) skip "SDL2 API (no development headers)" ;;
0:*) ok "$(sed -n 's/^== //p' "$OUT/api.txt")" ;;
*) ko "SDL2 API"; cat "$OUT/api.txt" ;;
esac
expect "--version" 0 "$BIN" --version
has "  ... says $V" "^deva-adventures $V\$" "$OUT/last.txt"
expect "--help" 0 "$BIN" --help
has "  ... lists --fullscreen" "--fullscreen" "$OUT/last.txt"
expect "an unknown option: exit 2" 2 "$BIN" --nope
expect "--data without a folder: exit 2" 2 "$BIN" --data
expect "--check with the data" 0 "$BIN" --check --data data
has "  ... all there" "tutto pronto" "$OUT/last.txt"
expect "data that are not there: exit 3" 3 "$BIN" --check --data "$OUT/nowhere"
has "  ... says what it needs" "Non trovo i dati del gioco in" "$OUT/last.txt"
expect "no SDL2: exit 4" 4 env DEVA_SDL2_LIB="$ABS/libSDL2-missing.so" "$BIN" --check --data data
has "  ... names the packages to install" "sudo apt install libsdl2-2.0-0" "$OUT/last.txt"

mkdir -p "$OUT/portable" "$OUT/prefix/bin" "$OUT/prefix/share" "$OUT/bare"
cp "$BIN" "$OUT/portable/deva-adventures"
ln -s "$ROOT/data/deva_adventures" "$OUT/portable/deva_adventures"
cp packaging/linux/deva-adventures.png "$OUT/portable/"
cp "$BIN" "$OUT/prefix/bin/deva-adventures"
ln -s "$ROOT/data/deva_adventures" "$OUT/prefix/share/deva_adventures"
cp "$BIN" "$OUT/bare/deva-adventures"
expect "data next to the program (the unpacked package)" 0 "$OUT/portable/deva-adventures" --check
has "  ... found there" "dati in $ABS/portable/deva_adventures," "$OUT/last.txt"
expect "data in <program>/../share (an installed tree)" 0 "$OUT/prefix/bin/deva-adventures" --check
has "  ... found there" "dati in $ABS/prefix/bin/../share/deva_adventures," "$OUT/last.txt"
expect "data from DEVA_ADVENTURES_DATA" 0 env DEVA_ADVENTURES_DATA="$ROOT/data/deva_adventures" \
    "$OUT/bare/deva-adventures" --check
if [ -d /usr/share/deva_adventures ] || [ -d /usr/local/share/deva_adventures ]; then
    skip "no data anywhere (this PC has them in /usr/share or /usr/local/share)"
else
    expect "no data anywhere: exit 3" 3 "$OUT/bare/deva-adventures" --check
    has "  ... says where they go" "accanto al programma ($ABS/bare)" "$OUT/last.txt"
fi

# ------------------------------------------------------------------ 2. on a virtual screen
MARK=0 # the lines of the run's output before the last press: a step looks only past them
mark() { MARK=$(wc -l <"$OUT/run.txt" 2>/dev/null || echo 0); }
wait_for() { # pattern, seconds: a new line of the run's output (past MARK) with it
    n=0
    while ! tail -n "+$((MARK + 1))" "$OUT/run.txt" 2>/dev/null | grep -q -- "$1"; do
        n=$((n + 1))
        [ "$n" -le $(($2 * 10)) ] || return 1
        sleep 0.1
    done
}
step() { # description, pattern, seconds: then a breath, as a hand would take
    if wait_for "$2" "${3:-6}"; then ok "$1"; else ko "$1 (no \"$2\")"; fi
    sleep 0.4
}
start() { # options...: PID of the game, WID of its window (empty if none came)
    "$OUT/portable/deva-adventures" "$@" >"$OUT/run.txt" 2>&1 &
    PID=$!
    MARK=0
    WID=$(timeout 15 xdotool search --sync --onlyvisible --name "$TITLE" 2>/dev/null | head -1)
}
finish() { # seconds: waits for the game to end by itself; RC its exit code (137: it had to be killed)
    (sleep "$1" && kill -9 "$PID" 2>/dev/null) >/dev/null 2>&1 &
    dog=$!
    wait "$PID"
    RC=$?
    kill "$dog" 2>/dev/null
    wait "$dog" 2>/dev/null
}
key() {
    mark
    xdotool key --window "$WID" "$@"
}
hold() { # key, seconds
    mark
    xdotool keydown --window "$WID" "$1"
    sleep "$2"
    xdotool keyup --window "$WID" "$1"
}
size() { xwininfo -id "$WID" | awk '/Width:/ { w = $2 } /Height:/ { h = $2 } END { print w "x" h }'; }
shot() { import -window "$WID" "$1" 2>/dev/null; }
mean() { convert "$1" -format '%[fx:mean]' info: 2>/dev/null; }
above() { awk -v a="$1" -v b="$2" 'BEGIN { exit !(a > b) }'; }
pace() { # the last "frames in" line: "frames seconds late bursts skipped"
    sed -n 's/^.*[^0-9]\([0-9][0-9]*\) frames in \([0-9.]*\) s, .*; \([0-9]*\) late .*, \([0-9]*\) times .*, \([0-9]*\) beats.*/\1 \2 \3 \4 \5/p' \
        "$1" | tail -n 1
}

echo "== on a virtual screen"
if ! command -v Xvfb >/dev/null 2>&1 || ! command -v xdotool >/dev/null 2>&1 || ! command -v xwininfo >/dev/null 2>&1 ||
    ! command -v import >/dev/null 2>&1; then
    skip "the window (needs Xvfb, xdotool, xwininfo and ImageMagick)"
else
    n=97
    while [ -e "/tmp/.X11-unix/X$n" ] || [ -e "/tmp/.X$n-lock" ]; do n=$((n - 1)); done
    Xvfb ":$n" -screen 0 1280x1024x24 -nolisten tcp >"$OUT/xvfb.txt" 2>&1 &
    XPID=$!
    export DISPLAY=":$n"
    i=0
    while ! xdpyinfo >/dev/null 2>&1 && ! xwininfo -root >/dev/null 2>&1; do
        i=$((i + 1))
        [ $i -lt 50 ] || break
        sleep 0.1
    done
    WMPID=
    if command -v openbox >/dev/null 2>&1; then
        openbox >"$OUT/openbox.txt" 2>&1 &
        WMPID=$!
        sleep 1
    else
        skip "no window manager (openbox): full screen and focus are not tried"
    fi
    OLDHOME=$HOME
    export HOME="$ABS/home" XDG_DATA_HOME="$ABS/xdg"
    mkdir -p "$HOME"
    SAVES="$XDG_DATA_HOME/deva-adventures"

    DEVA_SEED=7 start --scale 2 --verbose
    if [ -n "$WID" ]; then ok "a window opens"; else ko "a window opens"; fi
    if [ -n "$WID" ]; then
        yes_if "  ... 2 x 320x240 = 640x480 with --scale 2" [ "$(size)" = 640x480 ]
        if command -v xprop >/dev/null 2>&1; then
            has_class() { xprop -id "$WID" WM_CLASS | grep -q '"deva-adventures", "deva-adventures"'; }
            yes_if "  ... its class is deva-adventures (the menu entry's StartupWMClass)" has_class
            has_icon() { xprop -len 8 -id "$WID" _NET_WM_ICON | grep -q '256 x 256'; }
            yes_if "  ... with the icon (256x256)" has_icon
        fi
        step "the title screen" "BOT scene=title" 10
        [ -z "$WMPID" ] || xdotool windowactivate --sync "$WID" >/dev/null 2>&1 || true
        sleep 1
        shot "$OUT/title.png"
        m=$(mean "$OUT/title.png")
        yes_if "  ... is on the screen (mean brightness ${m:-?})" above "${m:-0}" 0.1
        yes_if "saves in \$XDG_DATA_HOME/deva-adventures" [ -f "$SAVES/.deva-adventures.lock" ]
        expect "a second copy: exit 5" 5 "$OUT/portable/deva-adventures"
        has "  ... says it is already open" "Il gioco è già aperto" "$OUT/last.txt"
        key Right
        step "Right: the next card" "BOT title sel=1 item=camerino"
        key Left
        step "Left: back to Gioca" "BOT title sel=0 item=gioca"
        key Return
        step "Return (red): Gioca" "BOT title_pick item=gioca"
        step "  ... the controls tutorial" "BOT prova step=croce" 8
        sleep 2
        key Right
        step "an arrow: the cross done" "BOT prova step=rosso" 8
        sleep 2
        key space
        step "Space (red): done" "BOT prova step=fatto" 8
        step "  ... and the first tale" "BOT racconto prologo step=1" 10
        hold Escape 1.3
        step "Esc held: the pause" "BOT pause open" 5
        shot "$OUT/pause.png"
        key BackSpace
        step "Backspace (yellow): on with the game" "BOT pause choice=0" 5
        if [ -n "$WMPID" ]; then
            xdotool windowminimize --sync "$WID" 2>/dev/null
            step "minimized: the game waits" "window not in front: the game waits" 5
            xdotool windowactivate --sync "$WID" 2>/dev/null
            step "back in front: it goes on" "back in front: the game goes on" 5
            key F11
            sleep 2
            yes_if "F11: full screen ($(size))" [ "$(size)" = 1280x1024 ]
            shot "$OUT/full.png"
            convert "$OUT/full.png" -crop 1280x32+0+0 +repage "$OUT/full_bar.png" 2>/dev/null
            convert "$OUT/full.png" -crop 1280x1+0+32 +repage "$OUT/full_row.png" 2>/dev/null
            bar=$(mean "$OUT/full_bar.png") row=$(mean "$OUT/full_row.png")
            yes_if "  ... 4 x 320x240, whole pixels, black bands of 32 px" above "${row:-0}" 0.01
            yes_if "  ... (the band: ${bar:-?})" [ "${bar:-1}" = 0 ]
            key F11
            sleep 2
            yes_if "F11 again: the window ($(size))" [ "$(size)" = 640x480 ]
            key alt+F4
            finish 15
            yes_if "Alt+F4 closes it (exit $RC)" [ "$RC" = 0 ]
        else
            kill -TERM "$PID"
            finish 15
        fi
        has "  ... the log tells the frames" "frames in" "$SAVES/deva-adventures.log"
        last=$(tail -n 1 "$SAVES/deva_adventures.sav" 2>/dev/null)
        yes_if "  ... the save is complete (# fine)" [ "$last" = "# fine" ]
        last=$(tail -n 1 "$SAVES/deva_adventures_diario.csv" 2>/dev/null)
        case "$last" in *";chiusa;"*) ok "  ... the diary closes the session" ;; *) ko "  ... the diary: $last" ;; esac
    fi

    # a gamepad: SDL's virtual one, plugged in by fake_pad.so (the real SDL2 underneath)
    # shellcheck disable=SC2046 # (the flags of sdl2-config are several words)
    if command -v sdl2-config >/dev/null 2>&1 &&
        ${CC:-cc} -std=c99 -shared -fPIC $(sdl2-config --cflags) -o "$OUT/fake_pad.so" tools/harness/fake_pad.c \
            $(sdl2-config --libs) -ldl >"$OUT/fake_pad.txt" 2>&1; then
        pad() { printf '%s\n' "$1" >>"$OUT/pad.txt"; }
        tap() {
            mark
            pad "+$1"
            sleep 0.3
            pad "-$1"
        }
        : >"$OUT/pad.txt"
        export DEVA_SDL2_LIB="$ABS/fake_pad.so" DEVA_FAKE_PAD="$ABS/pad.txt"
        start --saves "$ABS/saves-pad" --verbose --scale 2
        unset DEVA_SDL2_LIB DEVA_FAKE_PAD
        step "a gamepad plugged in: seen, buttons by position (not an Xbox pad)" \
            "gamepad [0-9]* connected: buttons by position" 10
        step "  ... the title screen" "BOT scene=title" 10
        sleep 1
        tap 14
        step "D-pad right: the next card" "BOT title sel=1 item=camerino"
        tap 13
        step "D-pad left: back to Gioca" "BOT title sel=0 item=gioca"
        tap 0
        sleep 1.5
        hasnt "the bottom button (yellow) does not choose" "BOT title_pick" "$OUT/run.txt"
        tap 1
        step "the right button (red) chooses Gioca" "BOT title_pick item=gioca"
        step "  ... the controls tutorial" "BOT prova step=croce" 8
        sleep 2
        tap 12
        step "the D-pad: the cross done" "BOT prova step=rosso" 8
        sleep 2
        tap 1
        step "red: done" "BOT prova step=fatto" 8
        step "  ... and the first tale" "BOT racconto prologo step=1" 10
        mark
        pad +6
        sleep 1.3
        pad -6
        step "START held: the pause" "BOT pause open" 5
        tap 0
        step "the bottom button (yellow): on with the game" "BOT pause choice=0" 5
        mark
        pad unplug
        step "pulled out: noted, and the game goes on" "gamepad [0-9]* disconnected" 5
        kill -TERM "$PID"
        finish 15
        yes_if "  ... and closes in order (exit $RC)" [ "$RC" = 0 ]

        # by colour, as an Xbox pad gets by itself: red B on the right, yellow Y on top
        : >"$OUT/pad.txt"
        export DEVA_SDL2_LIB="$ABS/fake_pad.so" DEVA_FAKE_PAD="$ABS/pad.txt"
        start --saves "$ABS/saves-pad2" --verbose --scale 2 --pad colori
        unset DEVA_SDL2_LIB DEVA_FAKE_PAD
        step "--pad colori: buttons by colour" "gamepad [0-9]* connected: buttons by colour" 10
        step "  ... the title screen" "BOT scene=title" 10
        sleep 1
        tap 3
        sleep 1.5
        hasnt "  ... Y (yellow, on top) does not choose" "BOT title_pick" "$OUT/run.txt"
        tap 1
        step "  ... B (red, on the right) chooses Gioca" "BOT title_pick item=gioca"
        step "  ... the controls tutorial" "BOT prova step=croce" 8
        sleep 2
        tap 11
        step "  ... the D-pad: the cross done" "BOT prova step=rosso" 8
        sleep 2
        tap 1
        step "  ... red: done" "BOT prova step=fatto" 8
        step "  ... and the first tale" "BOT racconto prologo step=1" 10
        mark
        pad +6
        sleep 1.3
        pad -6
        step "  ... START held: the pause" "BOT pause open" 5
        tap 3
        step "  ... Y (yellow): on with the game" "BOT pause choice=0" 5
        kill -TERM "$PID"
        finish 15
    else
        skip "the gamepad (needs the SDL2 development headers: libsdl2-dev)"
    fi

    # the pace, 12 s with the clock alone and 12 s with the (dummy) sound card, in the same saves folder
    for mode in no-audio audio; do
        if [ $mode = no-audio ]; then start --no-audio --saves "$ABS/saves-pace"; else start --saves "$ABS/saves-pace"; fi
        sleep 12
        kill -TERM "$PID"
        finish 15
        yes_if "SIGTERM ($mode run) closes it in order (exit $RC)" [ "$RC" = 0 ]
        # shellcheck disable=SC2046 # (five numbers)
        set -- $(pace "$ABS/saves-pace/deva-adventures.log")
        if [ $# = 5 ]; then
            fps=$(awk -v f="$1" -v s="$2" 'BEGIN { printf "%.2f", f / s }')
            yes_if "  ... $1 frames in $2 s: $fps a second" above "$fps" 58.5
            yes_if "  ... no faster than 62 a second" above 62 "$fps"
            yes_if "  ... late $3, two at once $4 (each under 2% of the frames)" \
                awk -v f="$1" -v l="$3" -v b="$4" 'BEGIN { exit !(l * 50 < f && b * 50 < f) }'
            [ $mode = no-audio ] || yes_if "  ... beats skipped for the sound: $5" [ "$5" -le 2 ]
        else
            ko "  ... no pace line in the log"
        fi
    done
    has "the run before is kept as deva-adventures.log.1" "frames in" "$ABS/saves-pace/deva-adventures.log.1"

    # without XDG_DATA_HOME: ~/.local/share/deva-adventures
    unset XDG_DATA_HOME
    start --no-audio
    sleep 2
    yes_if "without XDG_DATA_HOME: saves in ~/.local/share/deva-adventures" \
        [ -f "$HOME/.local/share/deva-adventures/.deva-adventures.lock" ]
    kill -TERM "$PID"
    finish 15
    export HOME="$OLDHOME"

    [ -z "$WMPID" ] || kill "$WMPID" 2>/dev/null
    kill "$XPID" 2>/dev/null
    wait 2>/dev/null
    unset DISPLAY
fi

# ------------------------------------------------------------------ 3. install.sh of the PC package
echo "== install.sh of the PC package"
PKG=release/deva-adventures-$V-linux-x86_64.tar.gz
if [ ! -f "$PKG" ]; then
    skip "install.sh (no $PKG: make dist)"
else
    mkdir -p "$OUT/pkg"
    tar -xzf "$PKG" -C "$OUT/pkg"
    P="$ABS/pkg/deva-adventures-$V-linux-x86_64"
    H="$ABS/ihome"
    L="$H/.local/lib/deva-adventures"
    MENU="$H/.local/share/applications/deva-adventures.desktop"
    ICON="$H/.local/share/icons/hicolor/256x256/apps/deva-adventures.png"
    S="$H/.local/share/deva-adventures"
    mkdir -p "$H" "$OUT/fakebin"
    inst() { # shell, options...: install.sh of the package in the fake home
        shell_=$1
        shift
        # shellcheck disable=SC2086 # ("busybox sh" is two words)
        env HOME="$H" XDG_DATA_HOME= DEVA_INSTALL_AS_ROOT=1 PATH="$H/.local/bin:$PATH" $shell_ "$P/install.sh" "$@"
    }
    expect "the package as unpacked: every file as in SHA256SUMS" 0 sh -c "cd '$P' && sha256sum -c SHA256SUMS"
    expect "install.sh --help" 0 inst sh --help
    if [ "$(id -u)" = 0 ]; then
        expect "with sudo, no --prefix: refused" 1 env HOME="$H" sh "$P/install.sh" --yes
        has "  ... says why" "non serve sudo" "$OUT/last.txt"
    fi
    expect "--dry-run" 0 inst sh --dry-run
    yes_if "  ... writes nothing" [ ! -e "$H/.local" ]
    expect "install" 0 inst sh --yes
    has "  ... the package checked" "Pacchetto verificato" "$OUT/last.txt"
    has "  ... the installed program checked" "Controllo: deva-adventures $V: dati in $L/deva_adventures" "$OUT/last.txt"
    yes_if "  ... program and data" present "$L/deva-adventures" "$L/deva_adventures/gfx/atlas.txt"
    yes_if "  ... the program executable" [ -x "$L/deva-adventures" ]
    yes_if "  ... the command (a link)" [ -L "$H/.local/bin/deva-adventures" ]
    yes_if "  ... the icon" [ -f "$ICON" ]
    has "  ... the menu entry runs the program by its full path" "^Exec=$L/deva-adventures --fullscreen\$" "$MENU"
    has "  ... in a window too" "^Exec=$L/deva-adventures --window\$" "$MENU"
    has "  ... with the icon's full path" "^Icon=$ICON\$" "$MENU"
    if command -v desktop-file-validate >/dev/null 2>&1; then
        expect "  ... a valid desktop entry (desktop-file-validate)" 0 desktop-file-validate "$MENU"
    fi
    expect "the installed command runs" 0 env HOME="$H" "$H/.local/bin/deva-adventures" --check
    has "  ... with the installed data" "dati in $L/deva_adventures" "$OUT/last.txt"

    # a game played (its saves), the grown-ups' settings changed, then the same version again
    mkdir -p "$S"
    printf 'nome = DEVA\nsessioni = 3\n# fine\n' >"$S/deva_adventures.sav"
    printf 'data;profilo\n' >"$S/deva_adventures_diario.csv"
    sed -i 's/^sessione_minuti = .*/sessione_minuti = 25/' "$L/deva_adventures/deva_adventures.cfg"
    expect "update over the installed version" 0 inst sh --yes
    has "  ... knows the version installed" "già installata   : versione $V" "$OUT/last.txt"
    has "  ... keeps the grown-ups' settings" "^sessione_minuti = 25" "$L/deva_adventures/deva_adventures.cfg"
    yes_if "  ... and puts the new defaults beside them" [ -f "$L/deva_adventures/deva_adventures.cfg.default" ]
    bk=
    for d in "$S"/copie/*; do [ -d "$d" ] && bk=$d && break; done
    yes_if "  ... copies the saves first (${bk#"$S/"})" present "$bk/deva_adventures.sav" "$bk/deva_adventures_diario.csv"
    yes_if "  ... leaves the saves where they are" [ -f "$S/deva_adventures.sav" ]
    yes_if "  ... and nothing half-done behind" absent "$H/.local/lib/.deva-adventures.new" "$H/.local/lib/.deva-adventures.old"

    # a broken package changes nothing
    cp -R "$P" "$OUT/broken"
    printf 'x' >>"$OUT/broken/deva_adventures/gfx/atlas.txt"
    expect "a broken package: refused" 1 env HOME="$H" XDG_DATA_HOME= DEVA_INSTALL_AS_ROOT=1 sh "$OUT/broken/install.sh" --yes
    has "  ... says so" "non corrisponde a SHA256SUMS" "$OUT/last.txt"

    # a failure halfway (the copy does not check out): everything back as it was
    cat >"$OUT/fakebin/sha256sum" <<EOF
#!/bin/sh
n=\$(cat "$ABS/fakebin/calls" 2>/dev/null || echo 0)
echo \$((n + 1)) >"$ABS/fakebin/calls"
[ "\$n" = 0 ] || exit 1
exec $(command -v sha256sum) "\$@"
EOF
    chmod +x "$OUT/fakebin/sha256sum"
    expect "a failure halfway (the copy does not check out)" 1 env PATH="$ABS/fakebin:$PATH" HOME="$H" XDG_DATA_HOME= \
        DEVA_INSTALL_AS_ROOT=1 sh "$P/install.sh" --yes
    has "  ... puts back what was there" "rimetto com'era" "$OUT/last.txt"
    expect "  ... the installed game still works" 0 env HOME="$H" "$H/.local/bin/deva-adventures" --check
    has "  ... with its settings" "^sessione_minuti = 25" "$L/deva_adventures/deva_adventures.cfg"
    yes_if "  ... and its menu entry" [ -f "$MENU" ]
    yes_if "  ... nothing half-done behind" absent "$H/.local/lib/.deva-adventures.new" "$H/.local/lib/.deva-adventures.old"

    # a failure after the new version took the place of the old one (writing the menu entry)
    mkdir -p "$OUT/fakeawk"
    printf '#!/bin/sh\nexit 1\n' >"$OUT/fakeawk/awk"
    chmod +x "$OUT/fakeawk/awk"
    sed -i 's/^sessione_minuti = .*/sessione_minuti = 26/' "$L/deva_adventures/deva_adventures.cfg"
    expect "a failure after the swap (the menu entry cannot be written)" 1 env PATH="$ABS/fakeawk:$PATH" HOME="$H" \
        XDG_DATA_HOME= DEVA_INSTALL_AS_ROOT=1 sh "$P/install.sh" --yes
    has "  ... puts back what was there" "rimetto com'era" "$OUT/last.txt"
    has "  ... the old folder back, settings and all" "^sessione_minuti = 26" "$L/deva_adventures/deva_adventures.cfg"
    expect "  ... the game still works" 0 env HOME="$H" "$H/.local/bin/deva-adventures" --check
    yes_if "  ... the menu entry as it was" [ -f "$MENU" ]
    yes_if "  ... nothing half-done behind" absent "$H/.local/lib/.deva-adventures.new" "$H/.local/lib/.deva-adventures.old" \
        "$MENU.new"

    # the game open: no update under its feet
    cp "$(command -v sleep)" "$OUT/fakebin/deva-adventures"
    "$OUT/fakebin/deva-adventures" 30 &
    SPID=$!
    sleep 0.3
    expect "the game open: wait" 1 inst sh --yes
    has "  ... says to close it" "il gioco è aperto" "$OUT/last.txt"
    kill "$SPID" 2>/dev/null
    wait "$SPID" 2>/dev/null

    # elsewhere, in a folder with a space; one with a % is refused
    PX="$ABS/con spazio"
    expect "--prefix with a space in it" 0 inst sh --yes --prefix "$PX"
    has "  ... the menu entry quotes the path" "^Exec=\"$PX/lib/deva-adventures/deva-adventures\" --fullscreen\$" \
        "$PX/share/applications/deva-adventures.desktop"
    if command -v desktop-file-validate >/dev/null 2>&1; then
        expect "  ... still a valid desktop entry" 0 desktop-file-validate "$PX/share/applications/deva-adventures.desktop"
    fi
    expect "  ... the program runs from there" 0 "$PX/bin/deva-adventures" --check
    expect "  ... uninstall from the installed copy (it knows its prefix)" 0 env HOME="$H" DEVA_INSTALL_AS_ROOT=1 \
        sh "$PX/lib/deva-adventures/install.sh" --uninstall --yes
    yes_if "  ... all gone" absent "$PX/lib/deva-adventures" "$PX/bin/deva-adventures" \
        "$PX/share/applications/deva-adventures.desktop"
    expect "--prefix with a % in it: refused" 1 inst sh --yes --prefix "$ABS/100%"
    yes_if "  ... before writing anything" [ ! -e "$ABS/100%" ]

    # uninstall: the saves stay, the changed settings go with them
    expect "uninstall, with the installed copy" 0 env HOME="$H" XDG_DATA_HOME= DEVA_INSTALL_AS_ROOT=1 \
        sh "$L/install.sh" --uninstall --yes
    yes_if "  ... program, command, menu and icon gone" absent "$L" "$H/.local/bin/deva-adventures" "$MENU" "$ICON"
    yes_if "  ... the saves stay" [ -f "$S/deva_adventures.sav" ]
    kept=
    for f in "$S"/deva_adventures.cfg.*; do [ -f "$f" ] && kept=$f && break; done
    yes_if "  ... the changed settings kept with them (${kept#"$S/"})" [ -n "$kept" ]
    expect "uninstall again: nothing to do" 0 inst sh --uninstall --yes
    has "  ... says so" "Non è installato qui" "$OUT/last.txt"

    # the same with the other shells a PC may have as sh
    for shell in dash "busybox sh"; do
        command -v "${shell%% *}" >/dev/null 2>&1 || { skip "install.sh with $shell (not here)"; continue; }
        expect "install with $shell" 0 inst "$shell" --yes
        expect "  ... the program runs" 0 env HOME="$H" "$L/deva-adventures" --check
        expect "  ... uninstall with $shell" 0 inst "$shell" --uninstall --yes
        yes_if "  ... all gone" absent "$L" "$MENU"
    done
fi

echo "== PC program: $PASS ok, $FAIL failed, $SKIP skipped"
[ "$FAIL" = 0 ]
