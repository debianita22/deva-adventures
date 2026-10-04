#!/bin/sh
# platform/sdl/sdl2_api.h against the real SDL2 headers (1.1): the same values, the same layouts and
# prototypes that compile against the real ones. Needs the SDL2 development headers (libsdl2-dev).
#   tools/harness/check_sdl2_api.sh          (inside make test-linux)
cd "$(dirname "$0")/../.." || exit 1
OUT=${OUT:-build/test}/sdl2-api
mkdir -p "$OUT"
CC=${CC:-cc}
if ! SDL_CFLAGS=$(sdl2-config --cflags 2>/dev/null) || ! SDL_LIBS=$(sdl2-config --libs 2>/dev/null); then
    echo "== SDL2 API: skipped (no sdl2-config: install libsdl2-dev)"
    exit 0
fi
# shellcheck disable=SC2086 # (the flags of sdl2-config are several words)
$CC -std=c99 -DREAL_SDL $SDL_CFLAGS -Werror=incompatible-pointer-types -Werror=int-conversion \
    -o "$OUT/real" tools/harness/check_sdl2_api.c $SDL_LIBS || { echo "== SDL2 API: prototypes differ"; exit 1; }
$CC -std=c99 -o "$OUT/ours" tools/harness/check_sdl2_api.c || exit 1
"$OUT/real" >"$OUT/real.txt" && "$OUT/ours" >"$OUT/ours.txt" || exit 1
if cmp -s "$OUT/real.txt" "$OUT/ours.txt"; then
    echo "== SDL2 API: $(wc -l <"$OUT/ours.txt") values and layouts and every prototype as in SDL $(sdl2-config --version)"
else
    echo "== SDL2 API: DIFFERENT from SDL $(sdl2-config --version)"
    diff "$OUT/real.txt" "$OUT/ours.txt"
    exit 1
fi
