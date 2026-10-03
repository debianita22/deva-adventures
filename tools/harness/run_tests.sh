#!/bin/sh
# Automated playthroughs of Deva's Awesome Adventures with the headless harness.
#   demo    : every game open, one short round each (2 questions) with right,
#             wrong and guided answers, rewards and the goodnight ending;
#             screenshots and an MP4 video
#   sblocco : default gradual unlock (one new game per round here): the "?"
#             cards turn into games one by one, the bot skips the locked ones
#   storia  : the whole tale, fast (one round charges the wand): prologue,
#             five duels, the colours coming back, the witch, the party
#   storia2 : the second adventure, from a save where the first one is over:
#             prologue, five duels with the questions of every game, the
#             stars coming back, the wizard afraid of the dark, the party
#   (0.12: in every ending the bot does the kind act - the dance with the cross,
#   the lantern, the lullaby note by note, the game of turns with one press out of
#   turn - except in storia3, where it waits and the lullaby goes on by itself)
#   storia3 : the third adventure (0.10), from a save where the second one is
#             over: the ogre blows the music away, five duels with the new
#             games' questions, the notes coming back, the lullaby, the party
#   storia4 : the fourth adventure (0.11), from a save where the third one is
#             over: the king stops the toys, five duels (the shop and the
#             measures among the questions), the keys, "un po' tu e un po' io",
#             the party at the merry-go-round
#   rivedi  : all four adventures told: the album tells the fourth one again
#             (beginning and ending, with the game of turns), the story progress
#             does not change; then "Gioca": the epilogue, the party for Deva (0.12)
#   ritorno : a new day in the middle of the second adventure: "Bentornata!
#             Contiamo le stelle..." on the map, the stars found hop as they are
#             counted (0.12)
#   soak    : long session answering (almost) always right, all eighteen games
#             in turn: levels climb to 5, every make-up item is won, the session ends
#   levels  : games already at level 4, the three of 0.6 at 5 and played first
#             (a saved file);
#             screenshots
#   nuovi   : the three games of 0.6 (ginnastica, trucca i mostri, forme e
#             colori) in turn from level 1, a guided answer, the first duel;
#             screenshots and a short MP4 video
#   nuovi2  : the three games of 0.10 (le lettere, le ombre, il sentiero) in
#             turn from level 1: right, wrong and guided answers; screenshots
#             and a short MP4 video
#   nuovi3  : the two games of 0.11 (il negozio, le misure) in turn from level 1
#             (their levels 4 and 5 are in "levels"): right, wrong and guided
#             answers; screenshots and a short MP4 video
#   menu    : the main menu (a saved profile with some make-up and two places
#             freed): dressing room, album, saves with a new profile typed on
#             the keyboard, options held open (levels, progress, credits), a
#             game paused once, and at the end out through "Esci"
#   monkey1, monkey2 (0.14): a small child with the console, a fresh install with
#             the shipped options: random moods (trying, mashing the red button,
#             holding it, the cross, any button, START, squeezing them all, putting
#             it down) for 25 minutes; the grown-ups' doors (options, saves, exit,
#             L + R) must stay shut
#   then (0.15) the playtest report of the soak and of monkey1 (report.html next to
#   their saves); tools/harness/test_saves.sh tries the saves cut by a power cut
set -e
cd "$(dirname "$0")/../.."
CORE=${CORE:-build/host/deva_adventures_libretro.so}
HARNESS=${HARNESS:-./build/harness}
VIDEO="--video"
[ -n "$NOVIDEO" ] && VIDEO=""
ALL=cpsbnmrdeogtflhwqu
OUT=${OUT:-build/test}
rm -rf "$OUT"
for t in demo sblocco storia storia2 storia3 storia4 rivedi ritorno soak levels nuovi nuovi2 nuovi3 menu monkey1 monkey2; do
    mkdir -p "$OUT/$t/system/deva_adventures" "$OUT/$t/save" "$OUT/$t/out"
    for d in gfx voce sfx musica; do
        ln -s "$PWD/data/deva_adventures/$d" "$OUT/$t/system/deva_adventures/$d"
    done
done
printf 'sessione_minuti = 21\ndomande_per_round = 2\nlivello_iniziale = 1\nsblocco_giochi = 0\n' \
    > "$OUT/demo/system/deva_adventures/deva_adventures.cfg"
printf 'sessione_minuti = 12\ndomande_per_round = 2\nlivello_iniziale = 1\nround_per_sblocco = 1\n' \
    > "$OUT/sblocco/system/deva_adventures/deva_adventures.cfg"
printf 'sessione_minuti = 30\ndomande_per_round = 2\nlivello_iniziale = 1\nround_per_sfida = 1\n' \
    > "$OUT/storia/system/deva_adventures/deva_adventures.cfg"
printf 'sessione_minuti = 30\ndomande_per_round = 2\nlivello_iniziale = 1\nround_per_sfida = 1\nsblocco_giochi = 0\n' \
    > "$OUT/storia2/system/deva_adventures/deva_adventures.cfg"
printf 'nome = DEVA\ncapitolo = 5\ncarica = 0\nracconti = 255\nsessioni = 8\n' > "$OUT/storia2/save/deva_adventures.sav"
printf 'sessione_minuti = 40\ndomande_per_round = 2\nlivello_iniziale = 1\nround_per_sfida = 1\nsblocco_giochi = 0\n' \
    > "$OUT/storia3/system/deva_adventures/deva_adventures.cfg"
printf 'nome = DEVA\navventura = 2\ncapitolo = 5\ncarica = 0\nracconti = 65535\nsessioni = 14\n' \
    > "$OUT/storia3/save/deva_adventures.sav"
printf 'sessione_minuti = 40\ndomande_per_round = 2\nlivello_iniziale = 1\nround_per_sfida = 1\nsblocco_giochi = 0\n' \
    > "$OUT/storia4/system/deva_adventures/deva_adventures.cfg"
printf 'nome = DEVA\navventura = 3\ncapitolo = 5\ncarica = 0\nracconti = 16777215\nsessioni = 20\n' \
    > "$OUT/storia4/save/deva_adventures.sav"
printf 'sessione_minuti = 20\ndomande_per_round = 2\nlivello_iniziale = 1\nsblocco_giochi = 0\n' \
    > "$OUT/rivedi/system/deva_adventures/deva_adventures.cfg"
printf 'nome = DEVA\navventura = 4\ncapitolo = 5\ncarica = 0\nracconti = 4294967295\nsessioni = 26\n' \
    > "$OUT/rivedi/save/deva_adventures.sav"
printf 'sessione_minuti = 10\ndomande_per_round = 2\nlivello_iniziale = 1\nsblocco_giochi = 0\n' \
    > "$OUT/ritorno/system/deva_adventures/deva_adventures.cfg"
printf 'nome = DEVA\navventura = 2\ncapitolo = 2\ncarica = 0\nracconti = 20479\nsessioni = 9\n' \
    > "$OUT/ritorno/save/deva_adventures.sav"
printf 'sessione_minuti = 30\ndomande_per_round = 5\nlivello_iniziale = 1\nsblocco_giochi = 0\n' \
    > "$OUT/soak/system/deva_adventures/deva_adventures.cfg"
printf 'sessione_minuti = 16\ndomande_per_round = 3\nlivello_iniziale = 4\nsblocco_giochi = 0\n' \
    > "$OUT/levels/system/deva_adventures/deva_adventures.cfg"
: > "$OUT/levels/save/deva_adventures.sav"
for g in conta parole sequenze balla nome memory ritmo dove emozioni storie; do
    echo "livello_$g = 4" >> "$OUT/levels/save/deva_adventures.sav"
done
for g in ginnastica trucco forme lettere ombre sentiero negozio misure; do # 0.6, 0.10, 0.11 at the top
    echo "livello_$g = 5" >> "$OUT/levels/save/deva_adventures.sav"
done
printf 'sessione_minuti = 8\ndomande_per_round = 3\nlivello_iniziale = 1\nsblocco_giochi = 0\nround_per_sfida = 3\n' \
    > "$OUT/nuovi/system/deva_adventures/deva_adventures.cfg"
printf 'sessione_minuti = 10\ndomande_per_round = 3\nlivello_iniziale = 1\nsblocco_giochi = 0\nstoria = 0\n' \
    > "$OUT/nuovi2/system/deva_adventures/deva_adventures.cfg"
printf 'sessione_minuti = 14\ndomande_per_round = 4\nlivello_iniziale = 1\nsblocco_giochi = 0\nstoria = 0\n' \
    > "$OUT/nuovi3/system/deva_adventures/deva_adventures.cfg"
printf 'sessione_minuti = 30\ndomande_per_round = 2\nlivello_iniziale = 1\nround_per_sfida = 3\n' \
    > "$OUT/menu/system/deva_adventures/deva_adventures.cfg"
cat > "$OUT/menu/save/deva_adventures.sav" <<'END'
livello_conta = 2
livello_parole = 3
livello_balla = 2
trucchi = ombretto_rosa,ombretto_azzurro,rossetto_rosso,guance,adesivo_cuore,coroncina
occhi = ombretto_rosa
labbra = rossetto_rosso
testa = coroncina
stelle_totali = 124
round_totali = 31
sessioni = 6
capitolo = 2
racconti = 79
giochi = conta,parole,sequenze,balla,nome,memory
END

echo "== demo"
DEVA_SEED=7 "$HARNESS" "$CORE" "$OUT/demo/system" "$OUT/demo/save" "$OUT/demo/out" \
    --plan cwh --games $ALL --frames 84000 --shots ${VIDEO:+$VIDEO "$OUT/demo/deva_adventures_demo.mp4"}
echo "== sblocco (gradual unlock, one game per round)"
DEVA_SEED=9 "$HARNESS" "$CORE" "$OUT/sblocco/system" "$OUT/sblocco/save" "$OUT/sblocco/out" \
    --plan c --games $ALL --frames 50000 --shots
grep "^giochi" "$OUT/sblocco/save/deva_adventures.sav"
echo "== storia (the whole tale)"
DEVA_SEED=4 "$HARNESS" "$CORE" "$OUT/storia/system" "$OUT/storia/save" "$OUT/storia/out" \
    --plan ccw --games cpse --frames 60000 --shots
grep "^capitolo\|^racconti" "$OUT/storia/save/deva_adventures.sav"
echo "== storia2 (the second adventure)"
DEVA_SEED=2 "$HARNESS" "$CORE" "$OUT/storia2/system" "$OUT/storia2/save" "$OUT/storia2/out" \
    --plan ccw --games cpsbnmrdeogtf --frames 70000 --shots
grep "^avventura\|^capitolo\|^racconti" "$OUT/storia2/save/deva_adventures.sav"
echo "duel questions: $(grep 'BOT duel question' "$OUT/storia2/out/log.txt" | sed 's/.*game=//' | tr '\n' ' ')"
echo "== storia3 (the third adventure)"
DEVA_SEED=6 "$HARNESS" "$CORE" "$OUT/storia3/system" "$OUT/storia3/save" "$OUT/storia3/out" \
    --plan ccw --games lhwcpsbnmrdeogtf --frames 90000 --shots --idle-acts
grep "^avventura\|^capitolo\|^racconti" "$OUT/storia3/save/deva_adventures.sav"
grep "act=lullaby" "$OUT/storia3/out/log.txt" | grep -v " n=" || true
echo "duel questions: $(grep 'BOT duel question' "$OUT/storia3/out/log.txt" | sed 's/.*game=//' | tr '\n' ' ')"
echo "== storia4 (the fourth adventure)"
DEVA_SEED=8 "$HARNESS" "$CORE" "$OUT/storia4/system" "$OUT/storia4/save" "$OUT/storia4/out" \
    --plan ccw --games quhwlcpsbnmrdeogtf --frames 100000 --shots
grep "^avventura\|^capitolo\|^racconti" "$OUT/storia4/save/deva_adventures.sav"
echo "duel questions: $(grep 'BOT duel question' "$OUT/storia4/out/log.txt" | sed 's/.*game=//' | tr '\n' ' ')"
echo "== rivedi (the album tells the fourth adventure again)"
cp "$OUT/rivedi/save/deva_adventures.sav" "$OUT/rivedi/sav_before"
DEVA_SEED=12 "$HARNESS" "$CORE" "$OUT/rivedi/system" "$OUT/rivedi/save" "$OUT/rivedi/out" \
    --plan c --games qu --frames 26000 --shots --rivedi
grep "racconto replay\|racconto_end\|act=ball \(auto\|done\)\|epilogue" "$OUT/rivedi/out/log.txt" || true
grep "^avventura\|^capitolo\|^racconti\|^epilogo" "$OUT/rivedi/save/deva_adventures.sav"
[ "$(grep "^racconti\|^capitolo\|^avventura" "$OUT/rivedi/sav_before")" = \
  "$(grep "^racconti\|^capitolo\|^avventura" "$OUT/rivedi/save/deva_adventures.sav")" ] &&
    echo "story progress unchanged by the replay"
echo "== ritorno (a new day in the middle of the second adventure)"
DEVA_SEED=3 "$HARNESS" "$CORE" "$OUT/ritorno/system" "$OUT/ritorno/save" "$OUT/ritorno/out" \
    --plan cw --games cpq --frames 9000 --shots
grep "mappa recap\|voice \(rit_\|n0\|mancano\)" "$OUT/ritorno/out/log.txt" || true
echo "== soak"
DEVA_SEED=11 "$HARNESS" "$CORE" "$OUT/soak/system" "$OUT/soak/save" "$OUT/soak/out" \
    --plan cccccccccw --games $ALL --frames 150000
echo "== levels (4 and up)"
DEVA_SEED=5 "$HARNESS" "$CORE" "$OUT/levels/system" "$OUT/levels/save" "$OUT/levels/out" \
    --plan ccch --games quhwlgtfcpsbnmrdeo --frames 96000 --shots
echo "== nuovi (ginnastica, trucca i mostri, forme e colori)"
DEVA_SEED=21 "$HARNESS" "$CORE" "$OUT/nuovi/system" "$OUT/nuovi/save" "$OUT/nuovi/out" \
    --plan ccchcccw --games gtf --frames 34000 --shots ${VIDEO:+$VIDEO "$OUT/nuovi/deva_adventures_nuovi.mp4"}
echo "== nuovi2 (le lettere, le ombre, il sentiero)"
DEVA_SEED=23 "$HARNESS" "$CORE" "$OUT/nuovi2/system" "$OUT/nuovi2/save" "$OUT/nuovi2/out" \
    --plan cwchccwc --games lhw --frames 36000 --shots ${VIDEO:+$VIDEO "$OUT/nuovi2/deva_adventures_nuovi2.mp4"}
echo "== nuovi3 (il negozio, le misure)"
DEVA_SEED=29 "$HARNESS" "$CORE" "$OUT/nuovi3/system" "$OUT/nuovi3/save" "$OUT/nuovi3/out" \
    --plan cwchccwccc --games qu --frames 40000 --shots ${VIDEO:+$VIDEO "$OUT/nuovi3/deva_adventures_nuovi3.mp4"}
echo "== menu (main menu, dressing room, album, saves, options, pause, exit)"
DEVA_SEED=3 "$HARNESS" "$CORE" "$OUT/menu/system" "$OUT/menu/save" "$OUT/menu/out" \
    --plan cw --games cps --frames 30000 --shots --tour ${VIDEO:+$VIDEO "$OUT/menu/deva_adventures_menu.mp4"}
ls "$OUT/menu/save"
cat "$OUT/menu/save/deva_adventures_opzioni.cfg"
grep -H "^nome" "$OUT"/menu/save/deva_adventures*.sav* || true  # profile 2 is typed, then deleted (.bak)
for s in 1 2; do
    sed 's/^sessione_minuti = .*/sessione_minuti = 30/' data/deva_adventures/deva_adventures.cfg \
        > "$OUT/monkey$s/system/deva_adventures/deva_adventures.cfg"
    echo "== monkey$s (a small child mashing, holding and squeezing the buttons for 25 minutes)"
    DEVA_SEED=$s "$HARNESS" "$CORE" "$OUT/monkey$s/system" "$OUT/monkey$s/save" "$OUT/monkey$s/out" \
        --monkey $s --frames 90000
    echo "grown-ups' doors opened by chance: $(grep -c "title hold done\|title confirm done\|profile new\|profile rename\|profile delete\|opzioni saved\|pause open" "$OUT/monkey$s/out/log.txt" || true)"
done
echo "== save file after soak"
cat "$OUT/soak/save/deva_adventures.sav"
echo "== parent log (a few lines per game)"
for g in conta parole sequenze balla nome memory ritmo dove emozioni storie ginnastica trucco forme lettere ombre \
    sentiero negozio misure; do
    grep ";$g;" "$OUT/soak/save/deva_adventures_log.csv" | head -2
done
echo "== playtest report (tools/report/report.py) and session diary"
python3 tools/report/report.py "$OUT/soak/save" -o "$OUT/soak/report.html" --text > "$OUT/soak/report.txt"
head -6 "$OUT/soak/report.txt"
python3 tools/report/report.py "$OUT/monkey1/save" -o "$OUT/monkey1/report.html"
echo "diary of monkey1: $(cut -d';' -f4 "$OUT/monkey1/save/deva_adventures_diario.csv" | sort | uniq -c | tr -s ' \n' ' ')"
echo "answer rows without a time (soak): $(awk -F';' 'NR > 1 && $10 == ""' "$OUT/soak/save/deva_adventures_log.csv" | wc -l)"
echo "== warnings in the logs"
grep -h "missing\|cannot\|WARN" "$OUT"/*/out/log.txt | sort | uniq -c | head -20 || true
