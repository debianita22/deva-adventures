#!/bin/sh
# Saves that survive a power cut (0.15). Each case prepares a save folder as a power cut, a card
# that did not finish writing or a grown-up with a text editor would leave it, starts the core for
# two seconds (it loads the profile and saves it again) and checks what came back.
#   CORE=... HARNESS=... tools/harness/test_saves.sh       (HARNESS may be "qemu-aarch64 -L ... harness")
cd "$(dirname "$0")/../.."
CORE=${CORE:-build/host/deva_adventures_libretro.so}
HARNESS=${HARNESS:-./build/harness}
OUT=${OUT:-build/test}/salvataggi
rm -rf "$OUT"
fails=0
checks=0

good() { # name sessions: a whole profile, as 0.15 writes it
    printf 'nome = %s\nlivello_conta = 3\nsessioni = %s\ngiochi = conta,parole,sequenze\nsecondi_giocati = 600\n# fine\n' \
        "$1" "$2"
}
setup() {
    d="$OUT/$1"
    S="$d/save"
    mkdir -p "$d/system/deva_adventures" "$S" "$d/out"
    for x in gfx voce sfx musica; do ln -s "$PWD/data/deva_adventures/$x" "$d/system/deva_adventures/$x"; done
    echo "== $1: $2"
}
run() {
    DEVA_SEED=1 $HARNESS "$CORE" "$d/system" "$S" "$d/out" --frames 120 > "$d/run.txt" 2>&1 ||
        echo "   (the harness stopped with an error, see $d/run.txt)"
}
check() { # what command...
    what=$1
    shift
    checks=$((checks + 1))
    if "$@" > /dev/null 2>&1; then echo "   ok   $what"; else echo "   FAIL $what" && fails=$((fails + 1)); fi
}
warned() { grep -q "cut short" "$d/out/log.txt"; }
quiet() { ! grep -q "cut short" "$d/out/log.txt"; }
last_line_mark() { [ "$(tail -n 1 "$1")" = "# fine" ]; }

setup troncato "cut in the middle of a line, the copy before is there"
good PREV 7 > "$S/deva_adventures.sav.prev"
good NUOVO 8 | head -c 40 > "$S/deva_adventures.sav"
run
check "the copy before is back" grep -q "^nome = PREV" "$S/deva_adventures.sav"
check "and the sessions go on from it" grep -q "^sessioni = 8" "$S/deva_adventures.sav"
check "the cut file is kept aside" test -f "$S/deva_adventures.sav.rotto"
check "a warning in the log" warned

setup sparito "the file is gone (the renaming was cut), the copy before is there"
good PREV 7 > "$S/deva_adventures.sav.prev"
run
check "the copy before is back" grep -q "^nome = PREV" "$S/deva_adventures.sav"
check "a warning in the log" warned

setup tmp "the file is gone, the save being written got to its end"
good TMP 9 > "$S/deva_adventures.sav.tmp"
good PREV 7 > "$S/deva_adventures.sav.prev"
run
check "the newest one wins" grep -q "^nome = TMP" "$S/deva_adventures.sav"
check "and the sessions go on from it" grep -q "^sessioni = 10" "$S/deva_adventures.sav"
check "it is the copy before now" grep -q "^nome = TMP" "$S/deva_adventures.sav.prev"
check "no .tmp left" test ! -e "$S/deva_adventures.sav.tmp"

setup buco "a hole of zeros (a card that did not finish writing)"
printf 'nome = NUOVO\nlivello_conta = 3\n\000\000\000\000\000\000\nsessioni = 8\n# fine\n' > "$S/deva_adventures.sav"
good PREV 7 > "$S/deva_adventures.sav.prev"
run
check "the copy before is back" grep -q "^nome = PREV" "$S/deva_adventures.sav"
check "a warning in the log" warned

setup tmp_a_meta "a save stopped halfway, the file is whole"
good OK 5 > "$S/deva_adventures.sav"
printf 'nome = TMP\nsessi' > "$S/deva_adventures.sav.tmp"
run
check "the file stays" grep -q "^nome = OK" "$S/deva_adventures.sav"
check "no warning" quiet
check "no .tmp left" test ! -e "$S/deva_adventures.sav.tmp"

setup vecchio "a file of 0.14 (no last line, no copy before)"
printf 'nome = VECCHIO\nlivello_conta = 2\nsessioni = 4\n' > "$S/deva_adventures.sav"
run
check "it loads as it is" grep -q "^nome = VECCHIO" "$S/deva_adventures.sav"
check "the sessions go on" grep -q "^sessioni = 5" "$S/deva_adventures.sav"
check "no warning" quiet
check "saved again with its last line" last_line_mark "$S/deva_adventures.sav"

setup a_mano "edited by hand: CR-LF, a line added after the last one, no newline at the end"
printf 'nome = MANO\r\nsessioni = 3\r\n# fine\r\nlivello_parole = 4' > "$S/deva_adventures.sav"
good PREV 7 > "$S/deva_adventures.sav.prev"
run
check "the edit stays" grep -q "^nome = MANO" "$S/deva_adventures.sav"
check "even the added line" grep -q "^livello_parole = 4" "$S/deva_adventures.sav"
check "no warning" quiet

setup opzioni "the in-game options cut short: the chosen profile is not lost"
good UNO 9 > "$S/deva_adventures.sav"
good DUE 3 > "$S/deva_adventures_2.sav"
printf 'profilo = 2\n# fine\n' > "$S/deva_adventures_opzioni.cfg.prev"
printf 'profilo = 2\nvolume_mu' > "$S/deva_adventures_opzioni.cfg"
run
check "profile 2 is the one in use" grep -q "BOT profile slot=2 name=DUE" "$d/out/log.txt"
check "the options file is whole again" last_line_mark "$S/deva_adventures_opzioni.cfg"

echo "== $((checks - fails)) of $checks checks passed"
[ "$fails" -eq 0 ]
