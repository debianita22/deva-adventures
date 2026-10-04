#!/bin/sh
# Deva's Awesome Adventures - the macOS app and its disk image (1.2). On a Mac (the CI: macos-14),
# after make mac:
#   tools/release/mkapp.sh [--out release]
#
# "Deva's Awesome Adventures.app": the program for arm64 and x86_64 in one, the data, the icon and
# SDL2.framework as the SDL project builds and signs it (pinned in packaging/sdl2/README.md). The app
# is signed ad hoc (there is no Apple Developer ID: the first time macOS asks to allow it, see
# packaging/macos/LEGGIMI.txt), checked, asked whether it finds its data and SDL2, then put with a
# link to Applicazioni, LEGGIMI.txt and the manual into release/deva-adventures-<v>-macos.dmg.
set -eu
cd "$(dirname "$0")/../.."
OUT=release
if [ "${1:-}" = --out ]; then OUT=$2; fi
V=$(sed -n 's/^#define GAME_VERSION "\(.*\)"/\1/p' src/common.h)
SDL2_VERSION=2.32.10
SDL2_DMG_SHA256=4a7ac31640d70214e848f994be8a12849c0f97918a7e6c2e27a40036166d1a7f
NAME="Deva's Awesome Adventures"
BIN=build/mac/deva-adventures

detach() { # (a scan of the volume just mounted can keep it busy for a moment)
    for _ in 1 2 3 4 5; do
        hdiutil detach "$1" >/dev/null 2>&1 && return 0
        sleep 2
    done
    hdiutil detach -force "$1"
}

[ -x "$BIN" ] || { echo "no $BIN: make mac first" >&2; exit 1; }
for arch in arm64 x86_64; do
    lipo "$BIN" -verify_arch "$arch" || { echo "$BIN has no $arch" >&2; exit 1; }
done

W=build/mac/pkg
APP="$W/$NAME.app"
rm -rf "$W"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources" "$APP/Contents/Frameworks" build/sdl2 "$OUT"

# SDL2.framework of the SDL project: these bytes only, its own signature kept
SDL_DMG=build/sdl2/SDL2-$SDL2_VERSION.dmg
if [ ! -f "$SDL_DMG" ]; then
    curl -fsSL -o "$SDL_DMG.part" \
        "https://github.com/libsdl-org/SDL/releases/download/release-$SDL2_VERSION/SDL2-$SDL2_VERSION.dmg"
    mv "$SDL_DMG.part" "$SDL_DMG"
fi
echo "$SDL2_DMG_SHA256  $SDL_DMG" | shasum -a 256 -c -
MNT=$(mktemp -d)
hdiutil attach -nobrowse -readonly -mountpoint "$MNT" "$SDL_DMG" >/dev/null
ditto "$MNT/SDL2.framework" "$APP/Contents/Frameworks/SDL2.framework"
detach "$MNT"
codesign --verify --strict --verbose=2 "$APP/Contents/Frameworks/SDL2.framework"

# the program, the data, the icons, the Info.plist
cp "$BIN" "$APP/Contents/MacOS/deva-adventures"
ditto data/deva_adventures "$APP/Contents/Resources/deva_adventures"
cp packaging/macos/deva-adventures.icns packaging/linux/deva-adventures.png "$APP/Contents/Resources/"
sed "s/@VERSION@/$V/g" packaging/macos/Info.plist.in >"$APP/Contents/Info.plist"
plutil -lint "$APP/Contents/Info.plist"

# signed ad hoc: the bundle sealed, the framework inside keeps the SDL project's signature (no extended
# attributes in the files: codesign refuses Finder information and resource forks)
xattr -cr "$APP/Contents/MacOS" "$APP/Contents/Resources" "$APP/Contents/Info.plist"
codesign --force --sign - --timestamp=none "$APP"
codesign --verify --deep --strict --verbose=2 "$APP"
DEVA_NO_DIALOG=1 "$APP/Contents/MacOS/deva-adventures" --check

# the disk image
STAGE=$W/dmg
mkdir -p "$STAGE/docs" "$STAGE/tools/report"
ditto "$APP" "$STAGE/$NAME.app"
ln -s /Applications "$STAGE/Applicazioni"
sed "s/@VERSION@/$V/g" packaging/macos/LEGGIMI.txt >"$STAGE/LEGGIMI.txt"
cp docs/manuale.pdf docs/playtest/scheda_osservazione.pdf "$STAGE/docs/"
cp LICENSE THIRD_PARTY.md CHANGELOG.md "$STAGE/"
cp packaging/sdl2/LICENSE.txt "$STAGE/LICENSE-SDL2.txt"
cp tools/report/report.py "$STAGE/tools/report/"
DMG="$OUT/deva-adventures-$V-macos.dmg"
# (hdiutil create sometimes fails with "Resource busy" while the system scans the new image: try again)
tries=0
until rm -f "$DMG" &&
    hdiutil create -volname "$NAME $V" -srcfolder "$STAGE" -fs HFS+ -format UDZO -imagekey zlib-level=9 "$DMG" >/dev/null; do
    tries=$((tries + 1))
    [ "$tries" -lt 4 ] || { echo "hdiutil create failed $tries times" >&2; exit 1; }
    sleep 5
done
hdiutil verify "$DMG" >/dev/null
ls -l "$DMG"
