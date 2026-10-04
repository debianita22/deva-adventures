#!/bin/sh
# Deva's Awesome Adventures - the macOS app of the disk image, tried (1.2). On a Mac, after mkapp.sh
# (the CI does it on a macOS runner):
#   tools/release/test_app.sh [release/deva-adventures-<v>-macos.dmg]
#
# The app copied out of the disk image as a user would, its signature and its two architectures,
# --version and --check, then a short game with SDL's own video and sound and no screen (600 frames,
# then out in order): the save whole, the log with its frames. Natively, and as x86_64 under Rosetta
# when the Mac has it.
set -eu
cd "$(dirname "$0")/../.."
V=$(sed -n 's/^#define GAME_VERSION "\(.*\)"/\1/p' src/common.h)
DMG=${1:-release/deva-adventures-$V-macos.dmg}
NAME="Deva's Awesome Adventures"

detach() { # (a scan of the new volume can keep it busy for a moment)
    for _ in 1 2 3 4 5; do
        hdiutil detach "$1" >/dev/null 2>&1 && return 0
        sleep 2
    done
    hdiutil detach -force "$1"
}

W=$(mktemp -d)
MNT=$W/mnt
mkdir -p "$MNT"
hdiutil attach -nobrowse -readonly -mountpoint "$MNT" "$DMG" >/dev/null
ls -l "$MNT"
for f in "$NAME.app" Applicazioni LEGGIMI.txt docs/manuale.pdf LICENSE-SDL2.txt; do
    [ -e "$MNT/$f" ] || { echo "the disk image has no $f" >&2; exit 1; }
done
ditto "$MNT/$NAME.app" "$W/$NAME.app"
detach "$MNT"

APP=$W/$NAME.app
EXE=$APP/Contents/MacOS/deva-adventures
codesign --verify --deep --strict "$APP"
echo "signature ok; $(lipo -archs "$EXE")"

try() { # native or x86_64
    run=""
    [ "$1" = native ] || run="arch -$1"
    $run "$EXE" --version
    $run "$EXE" --check >"$W/check.txt" 2>&1 || { cat "$W/check.txt"; return 1; }
    cat "$W/check.txt"
    grep -q 'tutto pronto' "$W/check.txt"
    S=$W/saves-$1
    SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy DEVA_TEST_FRAMES=600 DEVA_NO_DIALOG=1 \
        $run "$EXE" --verbose --saves "$S" >"$W/run-$1.txt" 2>&1 || { tail -n 30 "$W/run-$1.txt"; return 1; }
    tail -n 2 "$S/deva-adventures.log"
    grep -Eq '[0-9]+ frames in' "$S/deva-adventures.log"
    [ "$(tail -n 1 "$S/deva_adventures.sav")" = "# fine" ]
    echo "== $1: ok"
}
try native
said="macOS $(sw_vers -productVersion) on $(uname -m): the app runs natively"
if arch -x86_64 /usr/bin/true 2>/dev/null; then
    try x86_64
    said="$said and as x86_64 under Rosetta"
else
    echo "== x86_64: no Rosetta on this Mac, not tried"
    said="$said (no Rosetta: x86_64 not tried)"
fi
rm -rf "$W"
echo "::notice title=macOS app::$said" # (shown by GitHub Actions; elsewhere just a line)
