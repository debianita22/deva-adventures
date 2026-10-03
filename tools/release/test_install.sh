#!/bin/bash
# shellcheck disable=SC2016,SC2034,SC2012 # (the checks are strings run by eval; ls of our own folders)
# Deva's Awesome Adventures - the installer of the RetroArch packages (install.sh, uninstall.sh), tried
# on fake RetroArch folders: fresh install, upgrade with the grown-ups' settings and saves, backups and
# restore, rollback, uninstall. With busybox when there is one (its sh and its applets first in PATH,
# as on the handhelds). The installed x86_64 core is also run by the harness.
#
#   make dist harness && ./tools/release/test_install.sh        (or: make test-dist)
#   NO_BUSYBOX=1 ./tools/release/test_install.sh                  (the system sh and tools instead)
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
V=$(sed -n 's/^#define GAME_VERSION "\(.*\)"/\1/p' "$ROOT/src/common.h")
W=${OUT:-$ROOT/build/test-install}
PKGZIP=$ROOT/release/deva-adventures-$V-x86_64.zip
[ -f "$PKGZIP" ] || { echo "missing $PKGZIP: run make dist first"; exit 2; }
rm -rf "$W"; mkdir -p "$W/bb" "$W/old"
if [ -z "${NO_BUSYBOX:-}" ] && command -v busybox >/dev/null 2>&1; then
    busybox --install -s "$W/bb"
    export PATH="$W/bb:$PATH"
    SH="busybox sh"
    echo "(busybox $(busybox | head -1 | cut -d' ' -f2): its sh and its applets)"
else
    SH="sh"
    echo "(no busybox: $(command -v sh) and the system tools)"
fi
REAL_SHA=$(command -v sha256sum)
# the version "already installed": a stand-in core, an older .info, the data with older comments
OLD_CORE=$W/old/deva_adventures_libretro.so
printf 'a core of 0.15 (stand-in)\n' >"$OLD_CORE"
OLD_INFO=$W/old/deva_adventures_libretro.info
sed 's/^display_version = .*/display_version = "0.15.0"/' "$ROOT/deva_adventures_libretro.info" >"$OLD_INFO"
OLD_DATA=$W/old/deva_adventures
cp -R "$ROOT/data/deva_adventures" "$OLD_DATA"
sed -i '1s/^/# (i commenti di una versione vecchia)\n/' "$OLD_DATA/deva_adventures.cfg"
pass=0 fail=0
ok() { if eval "$2"; then pass=$((pass + 1)); echo "   ok   $1"; else fail=$((fail + 1)); echo "   FAIL $1   [$2]"; fi; }
h() { sha256sum "$1" 2>/dev/null | cut -d' ' -f1; }
fresh_pkg() { # -> $P: the unpacked package, $N: its data files
    rm -rf "$W/pkg"; mkdir -p "$W/pkg"; (cd "$W/pkg" && unzip -q "$PKGZIP"); P="$W/pkg/deva-adventures-$V-x86_64"
    N=$(grep -c "  system/deva_adventures/" "$P/SHA256SUMS"); }
inst() { (cd "$P" && $SH install.sh "$@") > "$W/out.txt" 2>&1; echo $? > "$W/rc.txt"; }
rc() { cat "$W/rc.txt"; }
saw() { grep -q -- "$1" "$W/out.txt"; }
home_cfg() { # $1 = sandbox: a RetroArch config in $1/home/.config/retroarch with ~ paths
    mkdir -p "$1/home/.config/retroarch"
    cat > "$1/home/.config/retroarch/retroarch.cfg" <<EOF
libretro_directory = "~/.config/retroarch/cores"
libretro_info_path = "~/.config/retroarch/cores"
system_directory = "~/.config/retroarch/system"
savefile_directory = "~/.config/retroarch/saves"
video_driver = "gl"
EOF
}
old_install() { # $1 = sandbox: version 0.15 installed, settings changed by the grown-ups, saves
    R="$1/home/.config/retroarch"
    mkdir -p "$R/cores" "$R/system" "$R/saves"
    cp "$OLD_CORE" "$R/cores/"; cp "$OLD_INFO" "$R/cores/"
    cp -R "$OLD_DATA" "$R/system/deva_adventures"
    sed -i 's/^sessione_minuti = .*/sessione_minuti = 30/' "$R/system/deva_adventures/deva_adventures.cfg"
    printf 'nome = DEVA\nsessioni = 4\n# fine\n' > "$R/saves/deva_adventures.sav"
    printf 'nome = DEVA\nsessioni = 3\n# fine\n' > "$R/saves/deva_adventures.sav.prev"
    printf 'nome = LUCA\n# fine\n' > "$R/saves/deva_adventures_2.sav"
    printf '2026-10-01;1;conta;1;conta:stelle;3;3;giusta;1;2.1;0;35\n' > "$R/saves/deva_adventures_log.csv"
    printf '2026-10-01;1;0;inizio;0.15.0 profilo 1\n' > "$R/saves/deva_adventures_diario.csv"
    printf 'profilo = 1\n# fine\n' > "$R/saves/deva_adventures_opzioni.cfg"
}
saves_sum() { (cd "$1" && cat deva_adventures* | sha256sum | cut -d' ' -f1); }

echo "== A: fresh install, ~ paths in retroarch.cfg; --dry-run first; no terminal without --yes"
T=$W/a; home_cfg "$T"; fresh_pkg; export HOME=$T/home; unset XDG_CONFIG_HOME
R=$T/home/.config/retroarch
inst --help
ok "--help in Italian, with the version" '[ "$(rc)" = 0 ] && saw "Deva.s Awesome Adventures $V - installazione in RetroArch"'
inst --boh
ok "an unknown option stops, with the help" '[ "$(rc)" = 2 ] && saw "Opzione sconosciuta: --boh" && saw "sh install.sh --dry-run"'
inst --dry-run
ok "dry-run ends well" '[ "$(rc)" = 0 ]'
ok "dry-run shows the folders" 'saw "core          : $R/cores" && saw "dati          : $R/system/deva_adventures"'
ok "dry-run changes nothing" '[ ! -e "$R/cores" ] && [ ! -e "$R/system" ]'
inst </dev/null
ok "without a terminal and --yes it stops" '[ "$(rc)" != 0 ] && saw "rilancia con --yes" && [ ! -e "$R/cores/deva_adventures_libretro.so" ]'
inst --yes
ok "install ends well" '[ "$(rc)" = 0 ]'
ok "package verified, then the copies" 'saw "Pacchetto verificato" && saw "Installazione verificata: $((N + 2)) file"'
ok "nothing before: no backup" 'saw "Nessuna versione precedente" && [ ! -e "$R/system/deva_adventures_backup" ]'
ok "core and .info in the cores folder" '[ "$(h $R/cores/deva_adventures_libretro.so)" = "$(h $P/cores/deva_adventures_libretro.so)" ] && [ -f "$R/cores/deva_adventures_libretro.info" ]'
ok "data in system/deva_adventures, INSTALLATO.txt" '[ -f "$R/system/deva_adventures/gfx/atlas.png" ] && grep -q "salvataggi = $R/saves" "$R/system/deva_adventures/INSTALLATO.txt"'
ok "no leftovers" '[ -z "$(ls -A $R/cores | grep "^\.")" ] && [ ! -e "$R/system/.deva_adventures.new" ] && [ ! -e "$R/system/deva_adventures/.verifica" ]'
ok "the start hint names both menus" 'saw "Contentless Cores (Nuclei senza contenuto)" && saw "Start Core (Avvia core)"'
mkdir -p "$R/saves" "$T/run"
DEVA_SEED=5 "$ROOT/build/harness" "$R/cores/deva_adventures_libretro.so" "$R/system" "$R/saves" "$T/run" --frames 2400 --plan c --games c > "$T/run.txt" 2>&1
ok "the installed core runs from there (harness, 40 s)" 'grep -q "warnings=0" "$T/run.txt" && grep -q "data $R/system/deva_adventures, saves $R/saves" "$T/run/log.txt"'
ok "and saves in the saves folder" '[ -f "$R/saves/deva_adventures.sav" ] && tail -1 "$R/saves/deva_adventures.sav" | grep -q "^# fine"'

echo "== A2: run by another user (root installing for ark): ~ is the home of retroarch.cfg"
T=$W/a2; home_cfg "$T"; fresh_pkg; export HOME=$T/root-home; mkdir -p "$HOME"; R=$T/home/.config/retroarch
inst --yes --cfg "$R/retroarch.cfg"
ok "installed in the config's home, not in \$HOME" '[ "$(rc)" = 0 ] && [ -f "$R/cores/deva_adventures_libretro.so" ] && [ -d "$R/system/deva_adventures" ] && [ ! -e "$HOME/.config" ]'

echo "== B: \"default\" folders in retroarch.cfg and : for RetroArch's own folder"
T=$W/b; fresh_pkg; mkdir -p "$T/cfg"; export HOME=$T/home
printf 'libretro_directory = "default"\nlibretro_info_path = ":/info"\nsystem_directory = "default"\nsavefile_directory = "default"\n' > "$T/cfg/retroarch.cfg"
inst --yes --cfg "$T/cfg/retroarch.cfg"
ok "install ends well" '[ "$(rc)" = 0 ]'
ok "cores and system next to retroarch.cfg, info from :/info" '[ -f "$T/cfg/cores/deva_adventures_libretro.so" ] && [ -f "$T/cfg/info/deva_adventures_libretro.info" ] && [ -d "$T/cfg/system/deva_adventures/voce" ]'
ok "no saves folder: the saves in the system folder" 'saw "salvataggi    : $T/cfg/system"'

echo "== C: Lakka-like absolute paths, saves sorted by core name, a folder in RAM, spaces in the paths"
T="$W/c with spaces"; fresh_pkg; S="$T/storage"; mkdir -p "$S/.config/retroarch" "$S/savefiles"
cat > "$S/.config/retroarch/retroarch.cfg" <<EOF
libretro_directory = "$S/cores"
libretro_info_path = "$S/cores"
system_directory = "$S/system"
savefile_directory = "$S/savefiles"
sort_savefiles_enable = "true"
EOF
mkdir -p "$S/savefiles/Deva's Awesome Adventures"
printf 'nome = DEVA\n# fine\n' > "$S/savefiles/Deva's Awesome Adventures/deva_adventures.sav"
inst --yes --cfg "$S/.config/retroarch/retroarch.cfg"
ok "install ends well" '[ "$(rc)" = 0 ]'
ok "saves: the folder named after the core" "saw \"salvataggi    : \$S/savefiles/Deva's Awesome Adventures\""
ok "files in place despite the spaces" '[ -f "$S/cores/deva_adventures_libretro.so" ] && [ -f "$S/system/deva_adventures/deva_adventures.cfg" ]'
BKC=$(ls -d "$S/system/deva_adventures_backup/"*/ | head -1)
ok "the save there went into the backup" '[ -f "${BKC}saves/deva_adventures.sav" ]'
SHM=/dev/shm/deva_inst_test; rm -rf "$SHM"; mkdir -p "$SHM/cores"
inst --yes --cfg "$S/.config/retroarch/retroarch.cfg" --cores "$SHM/cores"
ok "a cores folder in RAM is pointed out" 'saw "ATTENZIONE: $SHM/cores è in RAM"'
rm -rf "$SHM"
printf 'savefiles_in_content_dir = "true"\n' >> "$S/.config/retroarch/retroarch.cfg"
inst --dry-run --cfg "$S/.config/retroarch/retroarch.cfg"
ok "saves in the content folder (none for this core): the system folder" 'saw "salvataggi    : $S/system"'

echo "== D: upgrade over 0.15, settings changed by the grown-ups, six save files"
T=$W/d; home_cfg "$T"; old_install "$T"; fresh_pkg; export HOME=$T/home; R=$T/home/.config/retroarch
before=$(saves_sum "$R/saves")
inst --yes
ok "install ends well" '[ "$(rc)" = 0 ]'
ok "it saw 0.15 installed" 'saw "già installata: versione 0.15.0"'
BK=$(ls -d "$R/system/deva_adventures_backup/"*/ | head -1)
ok "backup: the 0.15 core, .info, data and saves" '[ "$(h ${BK}deva_adventures_libretro.so)" = "$(h $OLD_CORE)" ] && grep -q "0.15.0" ${BK}deva_adventures_libretro.info && [ -d ${BK}deva_adventures/voce ] && [ $(ls ${BK}saves | wc -l) = 6 ]'
ok "BACKUP.txt says which version" 'grep -q "versione_tolta = 0.15.0" ${BK}BACKUP.txt'
ok "the new core and .info" '[ "$(h $R/cores/deva_adventures_libretro.so)" = "$(h $P/cores/deva_adventures_libretro.so)" ] && grep -q "\"$V\"" $R/cores/deva_adventures_libretro.info'
ok "the grown-ups' settings stay, the new ones beside" 'grep -q "^sessione_minuti = 30" $R/system/deva_adventures/deva_adventures.cfg && cmp -s $R/system/deva_adventures/deva_adventures.cfg.default $P/system/deva_adventures/deva_adventures.cfg && saw "Le tue impostazioni"'
ok "verified, the kept settings left out" 'saw "Installazione verificata: $((N + 1)) file"'
ok "saves untouched" '[ "$(saves_sum $R/saves)" = "$before" ]'
ok "the way back is printed" 'saw "sh install.sh --restore"'

echo "== E: upgrade over 0.15 with its settings untouched (only the comments older)"
T=$W/e; home_cfg "$T"; old_install "$T"; fresh_pkg; export HOME=$T/home; R=$T/home/.config/retroarch
cp "$OLD_DATA/deva_adventures.cfg" "$R/system/deva_adventures/deva_adventures.cfg"
inst --yes
ok "install ends well" '[ "$(rc)" = 0 ]'
ok "the new file with the new comments, no .default" 'cmp -s $R/system/deva_adventures/deva_adventures.cfg $P/system/deva_adventures/deva_adventures.cfg && [ ! -e $R/system/deva_adventures/deva_adventures.cfg.default ] && ! saw "Le tue impostazioni"'
ok "every file verified" 'saw "Installazione verificata: $((N + 2)) file"'

echo "== F/G/H: --list-backups, --restore, --with-saves, back to before 0.15 (D's sandbox)"
T=$W/d; export HOME=$T/home; R=$T/home/.config/retroarch; fresh_pkg
inst --list-backups
ok "the backup is listed with its version" 'saw "(versione 0.15.0)"'
printf 'nome = DEVA\nsessioni = 9\n# fine\n' > "$R/saves/deva_adventures.sav"
inst --yes --restore "$BK"
ok "restore ends well" '[ "$(rc)" = 0 ]'
ok "core, .info and data of 0.15 again" '[ "$(h $R/cores/deva_adventures_libretro.so)" = "$(h $OLD_CORE)" ] && grep -q "0.15.0" $R/cores/deva_adventures_libretro.info && grep -q "^sessione_minuti = 30" $R/system/deva_adventures/deva_adventures.cfg && [ ! -e $R/system/deva_adventures/INSTALLATO.txt ]'
ok "the saves stay as they are without --with-saves" 'grep -q "sessioni = 9" $R/saves/deva_adventures.sav && [ -f $R/saves/deva_adventures.sav.prev ]'
inst --yes --restore "$BK" --with-saves
ok "--with-saves puts back the saves of then" 'grep -q "sessioni = 4" $R/saves/deva_adventures.sav'
ok "no leftovers of the swap" '[ ! -e $R/system/.deva_adventures.new ] && [ ! -e $R/system/.deva_adventures.old ]'
OLD014="$T/bk014"; cp -R "$BK" "$OLD014"; sed -i 's/^versione_tolta = .*/versione_tolta = 0.14.0/' "$OLD014/BACKUP.txt"
printf 'x\n' > "$R/saves/deva_adventures.sav.tmp"
inst --yes --restore "$OLD014"
ok "back to 0.14: its saves lose .prev and .tmp" '[ ! -e $R/saves/deva_adventures.sav.prev ] && [ ! -e $R/saves/deva_adventures.sav.tmp ] && [ -f $R/saves/deva_adventures.sav ]'
inst --yes --restore "$T/nonexistent"
ok "a folder that is not a backup is refused" '[ "$(rc)" != 0 ] && saw "manca BACKUP.txt"'

echo "== I: rollback, the copies do not match (a fake sha256sum fails on the installed files)"
T=$W/i; home_cfg "$T"; old_install "$T"; fresh_pkg; export HOME=$T/home; R=$T/home/.config/retroarch
mkdir -p "$W/fake"; cat > "$W/fake/sha256sum" <<EOF
#!/bin/sh
case "\$*" in *.verifica*) exit 1 ;; esac
exec "$REAL_SHA" "\$@"
EOF
chmod +x "$W/fake/sha256sum"
data_before=$(cd "$R/system/deva_adventures" && find . -type f | sort | xargs sha256sum | sha256sum)
PATH="$W/fake:$PATH" inst --yes
ok "the install fails" '[ "$(rc)" != 0 ] && saw "non corrispondono al pacchetto" && saw "rimetto com"'
ok "the 0.15 core and .info are back" '[ "$(h $R/cores/deva_adventures_libretro.so)" = "$(h $OLD_CORE)" ] && grep -q "0.15.0" $R/cores/deva_adventures_libretro.info'
ok "the 0.15 data are back, settings included" '[ "$(cd $R/system/deva_adventures && find . -type f | sort | xargs sha256sum | sha256sum)" = "$data_before" ]'
ok "no leftovers" '[ ! -e $R/system/.deva_adventures.new ] && [ -z "$(ls -A $R/cores | grep "^\.")" ]'
T=$W/i2; home_cfg "$T"; fresh_pkg; export HOME=$T/home; R=$T/home/.config/retroarch
PATH="$W/fake:$PATH" inst --yes
ok "a first install that fails leaves nothing" '[ "$(rc)" != 0 ] && [ ! -e $R/cores/deva_adventures_libretro.so ] && [ ! -e $R/cores/deva_adventures_libretro.info ] && [ ! -e $R/system/deva_adventures ]'

echo "== J: a damaged package is refused before anything changes"
T=$W/j; home_cfg "$T"; old_install "$T"; fresh_pkg; export HOME=$T/home; R=$T/home/.config/retroarch
printf 'x' >> "$P/system/deva_adventures/voce/saluto.ogg"
inst --yes
ok "refused" '[ "$(rc)" != 0 ] && saw "non corrisponde a SHA256SUMS"'
ok "0.15 still there, no backup" '[ "$(h $R/cores/deva_adventures_libretro.so)" = "$(h $OLD_CORE)" ] && [ ! -e $R/system/deva_adventures_backup ]'

echo "== K: uninstall (D's sandbox, after a new install)"
T=$W/d; export HOME=$T/home; R=$T/home/.config/retroarch; fresh_pkg
inst --yes
nbk=$(ls -d "$R"/system/deva_adventures_backup/*/ | wc -l)
(cd "$P" && $SH uninstall.sh --yes) > "$W/out.txt" 2>&1; echo $? > "$W/rc.txt"
ok "uninstall ends well" '[ "$(rc)" = 0 ]'
ok "core, .info and data gone" '[ ! -e $R/cores/deva_adventures_libretro.so ] && [ ! -e $R/cores/deva_adventures_libretro.info ] && [ ! -e $R/system/deva_adventures ]'
ok "saves and backups stay" '[ -f $R/saves/deva_adventures.sav ] && [ $(ls -d $R/system/deva_adventures_backup/*/ | wc -l) = "$nbk" ]'

echo "== $pass passed, $fail failed"
[ "$fail" = 0 ]
