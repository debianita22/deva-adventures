#!/bin/sh
# The Windows package under Wine, on a virtual X screen (1.2): the program of the zip and the installer.
#   tools/harness/test_windows.sh               (make test-windows; needs make dist first)
#
# 1. the program without a window: options, exit codes, SDL2.dll and the data missing;
# 2. its window (Wine draws it on Xvfb, openbox for full screen and focus): picture, keys through the
#    first screens, the pause held, Alt+Enter and F11, one copy at a time, Alt+F4 with the saves in
#    %APPDATA% complete and in the same bytes as on Linux (LF);
# 3. setup.exe /S: files, shortcuts, the entry in "App installate", the program installed; installed
#    again over a changed deva_adventures.cfg (kept, the new one beside it); uninstall.exe /S (the
#    saves stay).
# Needs wine (64 bit), Xvfb, openbox, xdotool, xwininfo, ImageMagick. A fresh Wine prefix in
# build/test/windows/wine (about a minute the first time). The real Windows is tried by the CI.
set -u
cd "$(dirname "$0")/../.." || exit 1
OUT=${OUT:-build/test}/windows
V=$(sed -n 's/^#define GAME_VERSION "\(.*\)"/\1/p' src/common.h)
ZIP=release/deva-adventures-$V-windows-x64.zip
SETUP=release/deva-adventures-$V-windows-x64-setup.exe
TITLE="Deva's Awesome Adventures"
PASS=0 FAIL=0 SKIP=0
ok() { PASS=$((PASS + 1)); printf '  ok    %s\n' "$*"; }
ko() { FAIL=$((FAIL + 1)); printf '  FAIL  %s\n' "$*"; }
skip() { SKIP=$((SKIP + 1)); printf '  skip  %s\n' "$*"; }
for t in wine Xvfb xdotool xwininfo import; do
    command -v "$t" >/dev/null 2>&1 || { echo "== Windows package: skipped (no $t)"; exit 0; }
done
[ -f "$ZIP" ] || { echo "no $ZIP: make dist first"; exit 1; }
rm -rf "$OUT"
mkdir -p "$OUT"
ABS=$(cd "$OUT" && pwd)
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
yes_if() {
    d=$1
    shift
    if "$@"; then ok "$d"; else ko "$d"; fi
}
present() {
    for f; do [ -e "$f" ] || return 1; done
}
absent() {
    for f; do
        if [ -e "$f" ] || [ -L "$f" ]; then return 1; fi
    done
    return 0
}

# a virtual screen and a window manager, then a fresh Wine
n=96
while [ -e "/tmp/.X11-unix/X$n" ] || [ -e "/tmp/.X$n-lock" ]; do n=$((n - 1)); done
Xvfb ":$n" -screen 0 1280x1024x24 -nolisten tcp >"$OUT/xvfb.txt" 2>&1 &
XPID=$!
export DISPLAY=":$n" WINEPREFIX="$ABS/wine" WINEDEBUG=-all WINEDLLOVERRIDES="mscoree,mshtml=" DEVA_NO_DIALOG=1 \
    SDL_AUDIODRIVER=dummy
sleep 1
WMPID=
if command -v openbox >/dev/null 2>&1; then
    openbox >"$OUT/openbox.txt" 2>&1 &
    WMPID=$!
fi
echo "== a fresh Wine prefix"
timeout 300 wineboot -i >"$OUT/wineboot.txt" 2>&1
U=
for d in "$WINEPREFIX"/drive_c/users/*; do
    case "${d##*/}" in Public | '*') ;; *) U=${d##*/}; break ;; esac
done
APPDATA_U="$WINEPREFIX/drive_c/users/$U/AppData/Roaming"
LOCAL_U="$WINEPREFIX/drive_c/users/$U/AppData/Local"
SAVES="$APPDATA_U/deva-adventures"

# ------------------------------------------------------------------ 1. without a window
echo "== the program of the zip ($ZIP)"
python3 -c "import sys, zipfile; zipfile.ZipFile(sys.argv[1]).extractall(sys.argv[2])" "$ZIP" "$OUT/zip"
P="$ABS/zip/deva-adventures-$V-windows-x64"
expect "the package as unpacked: every file as in SHA256SUMS" 0 sh -c "cd '$P' && sha256sum -c SHA256SUMS"
expect "--version" 0 wine "$P/deva-adventures.exe" --version
has "  ... says $V" "deva-adventures $V" "$OUT/last.txt"
expect "an unknown option: exit 2" 2 wine "$P/deva-adventures.exe" --nope
expect "--check" 0 wine "$P/deva-adventures.exe" --check
has "  ... finds the data and SDL2.dll" "SDL 2\.[0-9.]*: tutto pronto" "$OUT/last.txt"
mv "$P/SDL2.dll" "$P/SDL2.dll.off"
expect "SDL2.dll missing: exit 4" 4 wine "$P/deva-adventures.exe" --check
has "  ... says what to do" "Manca SDL2.dll accanto al programma" "$OUT/last.txt"
mv "$P/SDL2.dll.off" "$P/SDL2.dll"
mv "$P/deva_adventures" "$P/deva_adventures.off"
expect "data missing: exit 3" 3 wine "$P/deva-adventures.exe" --check
has "  ... says where they go" "accanto al programma" "$OUT/last.txt"
mv "$P/deva_adventures.off" "$P/deva_adventures"

# ------------------------------------------------------------------ 2. the window
MARK=0
mark() { MARK=$(wc -l <"$OUT/run.txt" 2>/dev/null || echo 0); }
step() { # description, pattern, seconds
    k=0
    while ! tail -n "+$((MARK + 1))" "$OUT/run.txt" 2>/dev/null | grep -q -- "$2"; do
        k=$((k + 1))
        if [ "$k" -gt $((${3:-8} * 10)) ]; then
            ko "$1 (no \"$2\")"
            return
        fi
        sleep 0.1
    done
    ok "$1"
    sleep 0.4
}
key() {
    mark
    xdotool key --window "$WID" "$@"
}
size() { xwininfo -id "$WID" | awk '/Width:/ { w = $2 } /Height:/ { h = $2 } END { print w "x" h }'; }
mean() { convert "$1" -format '%[fx:mean]' info: 2>/dev/null; }
above() { awk -v a="$1" -v b="$2" 'BEGIN { exit !(a > b) }'; }

echo "== its window (Wine on Xvfb)"
wine "$P/deva-adventures.exe" --scale 2 --verbose >"$OUT/run.txt" 2>&1 &
PID=$!
MARK=0
WID=$(timeout 60 xdotool search --sync --onlyvisible --name "$TITLE" 2>/dev/null | head -n 1)
if [ -z "$WID" ]; then
    ko "a window opens"
else
    ok "a window opens"
    yes_if "  ... 640x480 with --scale 2" [ "$(size)" = 640x480 ]
    step "the title screen" "BOT scene=title" 30
    [ -z "$WMPID" ] || xdotool windowactivate --sync "$WID" >/dev/null 2>&1 || true
    sleep 1
    import -window "$WID" "$OUT/title.png" 2>/dev/null
    m=$(mean "$OUT/title.png")
    yes_if "  ... is on the screen (mean brightness ${m:-?})" above "${m:-0}" 0.1
    has "  ... drawn by Direct3D" "renderer direct3d" "$OUT/run.txt"
    yes_if "saves in %APPDATA%\\deva-adventures" [ -f "$SAVES/.deva-adventures.lock" ]
    expect "a second copy: exit 5" 5 wine "$P/deva-adventures.exe"
    has "  ... says it is already open" "Il gioco è già aperto" "$OUT/last.txt"
    key Right
    step "Right: the next card" "BOT title sel=1 item=camerino"
    key Left
    step "Left: back to Gioca" "BOT title sel=0 item=gioca"
    key Return
    step "Return (red): Gioca" "BOT title_pick item=gioca"
    step "  ... the controls tutorial" "BOT prova step=croce" 10
    sleep 2
    key Right
    step "an arrow: the cross done" "BOT prova step=rosso" 10
    sleep 2
    key space
    step "Space (red): done" "BOT prova step=fatto" 10
    step "  ... and the first tale" "BOT racconto prologo step=1" 15
    mark
    xdotool keydown --window "$WID" Escape
    sleep 1.3
    xdotool keyup --window "$WID" Escape
    step "Esc held: the pause" "BOT pause open" 5
    key BackSpace
    step "Backspace (yellow): on with the game" "BOT pause choice=0" 5
    if [ -n "$WMPID" ]; then
        key alt+Return
        sleep 2
        yes_if "Alt+Enter: full screen ($(size))" [ "$(size)" = 1280x1024 ]
        key F11
        sleep 2
        yes_if "F11: the window again ($(size))" [ "$(size)" = 640x480 ]
        key alt+F4
    else
        kill -TERM "$PID"
    fi
    (sleep 20 && kill -9 "$PID" 2>/dev/null) >/dev/null 2>&1 &
    dog=$!
    wait "$PID"
    RC=$?
    kill "$dog" 2>/dev/null
    yes_if "Alt+F4 closes it (exit $RC)" [ "$RC" = 0 ]
    last=$(tail -n 1 "$SAVES/deva_adventures.sav" 2>/dev/null)
    yes_if "  ... the save is complete (# fine)" [ "$last" = "# fine" ]
    if grep -q "$(printf '\r')" "$SAVES/deva_adventures.sav"; then
        ko "  ... the save has CR-LF: not the bytes of the other systems"
    else
        ok "  ... in the same bytes as on Linux and the console (LF)"
    fi
    has "  ... the diary closes the session" ";chiusa;" "$SAVES/deva_adventures_diario.csv"
    has "  ... the log tells the frames" "frames in" "$SAVES/deva-adventures.log"
fi

# ------------------------------------------------------------------ 3. the installer
echo "== the installer ($SETUP)"
if [ ! -f "$SETUP" ]; then
    skip "the installer (no $SETUP: makensis missing at make dist)"
else
    I="$LOCAL_U/Programs/$TITLE"
    REGKEY='HKCU\Software\Microsoft\Windows\CurrentVersion\Uninstall\DevaAdventures'
    expect "setup.exe /S" 0 wine "$SETUP" /S
    yes_if "  ... the program in %LOCALAPPDATA%\\Programs" [ -f "$I/deva-adventures.exe" ]
    yes_if "  ... with SDL2.dll and the data" present "$I/SDL2.dll" "$I/deva_adventures/gfx/atlas.txt"
    lnk=$(find "$WINEPREFIX/drive_c/users/$U" -name "$TITLE.lnk" 2>/dev/null | grep -c 'Start Menu')
    yes_if "  ... a shortcut in the Start menu" [ "$lnk" -ge 1 ]
    expect "  ... an entry in App installate" 0 wine reg query "$REGKEY" /v DisplayVersion
    has "  ... version $V" "$V" "$OUT/last.txt"
    expect "the installed program runs" 0 wine "$I/deva-adventures.exe" --check
    sed -i 's/^sessione_minuti = .*/sessione_minuti = 25/' "$I/deva_adventures/deva_adventures.cfg"
    expect "installed again (an update)" 0 wine "$SETUP" /S
    has "  ... the grown-ups' settings stay" "^sessione_minuti = 25" "$I/deva_adventures/deva_adventures.cfg"
    yes_if "  ... the new ones beside them" [ -f "$I/deva_adventures/deva_adventures.cfg.default" ]
    expect "uninstall.exe /S" 0 wine "$I/uninstall.exe" /S
    sleep 3 # (the uninstaller works on from a copy of itself)
    yes_if "  ... the program is gone" absent "$I/deva-adventures.exe" "$I/deva_adventures"
    expect "  ... and its entry" 1 wine reg query "$REGKEY"
    yes_if "  ... the saves stay" [ -f "$SAVES/deva_adventures.sav" ]
fi

[ -z "$WMPID" ] || kill "$WMPID" 2>/dev/null
wineserver -k 2>/dev/null
kill "$XPID" 2>/dev/null
wait 2>/dev/null
echo "== Windows package: $PASS ok, $FAIL failed, $SKIP skipped"
[ "$FAIL" = 0 ]
