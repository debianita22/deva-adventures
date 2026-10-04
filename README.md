# Deva's Awesome Adventures

An educational game for five-year-olds, in Italian and fully voiced, for **RetroArch**: a contentless
libretro core in C99, designed for the XiFan RF35H handheld (RK3326) and for any RetroArch on Linux
arm64 or x86_64. The same game also runs **on a PC without RetroArch**, as a program of its own on
**Windows**, **macOS** and **Linux**, in a window or full screen.

*Italiano: [README.it.md](README.it.md).*

<p>
<img src="docs/img/menu.png" width="32%" alt="Main menu">
<img src="docs/img/sfida.png" width="32%" alt="A challenge with the monster">
<img src="docs/img/sentiero.png" width="32%" alt="The path, a first coding game">
</p>

- **Eighteen games** with levels 1 to 5 that adapt on their own, and guided help after two mistakes:
  numbers, coins, measures, words, letters, shapes, shadows, sequences, rhythm, memory, positions,
  emotions, dance, gymnastics, make-up, first coding.
- **Four adventures** with a map, challenges and a kind gesture that turns the villain good, then a
  final party; a make-up room, an album, three profiles.
- **Fully voiced** (923 phrases): no reading needed. Animated characters, original music and sound
  effects.
- **Child-proof**: options, saves and exit open only while holding L and R; timed sessions that end
  with a goodnight; saves that survive a power cut halfway through.
- **For grown-ups**: a play report (`tools/report/report.py`) and a kit for the first playtest.
- No network, ads or purchases: everything stays on the console or the PC.

## Install

The packages are on the [releases page](https://github.com/debianita22/deva-adventures/releases),
with `SHA256SUMS` to check them:

| Package | For |
| --- | --- |
| `deva-adventures-1.2.0-windows-x64-setup.exe` | **Windows 10 and 11** (64-bit): per-user installer, no administrator needed |
| `deva-adventures-1.2.0-windows-x64.zip` | Windows 10 and 11 (64-bit), portable |
| `deva-adventures-1.2.0-macos.dmg` | **macOS** 10.13 or later, Apple Silicon and Intel |
| `deva-adventures-1.2.0-linux-x86_64.tar.gz` | **Linux PC without RetroArch** (x86_64, glibc ≥ 2.17, SDL2 ≥ 2.0.9) |
| `deva-adventures-1.2.0-aarch64.zip` | arm64 Linux handhelds with RetroArch (glibc ≥ 2.17) |
| `deva-adventures-1.2.0-x86_64.zip` | Linux PC with RetroArch (glibc ≥ 2.17) |
| `deva-adventures-1.2.0-src.tar.gz` | source code: devaOS (Buildroot), Lakka, other architectures |

**Windows**: run the `setup.exe` (or unzip the zip and open `deva-adventures.exe`). The program is not
signed with a paid certificate, so the first time Windows shows "Windows protected your PC": *More
info* → *Run anyway*.

**macOS**: open the `.dmg` and drag the app to Applications. The app is signed ad hoc, not by an
Apple-registered developer, so macOS blocks the first launch: *System Settings* → *Privacy &
Security* → *Open Anyway* (details in the disk's `LEGGIMI.txt`).

**Linux PC**, without RetroArch:

```sh
tar xzf deva-adventures-1.2.0-linux-x86_64.tar.gz
cd deva-adventures-1.2.0-linux-x86_64
./deva-adventures          # right away, in a window (F11: full screen)
./install.sh               # or installed: application menu, icon, deva-adventures command
```

**Handheld**, with RetroArch:

```sh
unzip deva-adventures-1.2.0-aarch64.zip
cd deva-adventures-1.2.0-aarch64
sh install.sh --dry-run    # shows the folders and what it would do
sh install.sh              # backup, install, check of the copied files
```

Then in RetroArch: *Contentless Cores* → **Deva's Awesome Adventures**. Manual installation, devaOS,
Lakka, the PC without RetroArch, updates and rollback: chapters 1, 2 and 13 of the manual. The saves
are the same files on every system: copy them from the handheld to the PC and back.

## Build

```sh
make              # the core for the PC: build/host/deva_adventures_libretro.so
make linux        # the game without RetroArch for this Linux PC: build/host/deva-adventures
make test         # 16 automated playthroughs and interrupted saves
make test-linux   # the PC program on a virtual screen (Xvfb), and its install.sh
make dist         # release packages in release/ (Zig: aarch64 and x86_64 cores, Linux and Windows programs)
make test-windows # the Windows package under Wine
make mac          # on a Mac: one program for Apple Silicon and Intel (then tools/release/mkapp.sh)
```

GitHub Actions (`.github/workflows/ci.yml`) runs all of this on every push, tries the packages on real
Windows and macOS runners, and publishes the release when a `vX.Y.Z` tag is pushed. Requirements, ARM
tests under qemu, sanitizers and the rest: [`docs/SVILUPPO.md`](docs/SVILUPPO.md).

Packaging for other systems is in [`packaging/`](packaging): the RetroArch, Linux, Windows and macOS
installers, a Buildroot package (devaOS) and a Lakka package.

## Documentation (Italian)

| | |
| --- | --- |
| [`docs/manuale.pdf`](docs/manuale.pdf) ([HTML](docs/manuale.html)) | the parents' manual: install, games, adventures, options, files, troubleshooting |
| [`docs/playtest/scheda_osservazione.pdf`](docs/playtest/scheda_osservazione.pdf) | the first playtest: a guide for the observer and a sheet to print |
| [`docs/SVILUPPO.md`](docs/SVILUPPO.md) | build, test, harness, art, audio, voice, architecture, release |
| [`CHANGELOG.md`](CHANGELOG.md) | what changed in each version |
| [`THIRD_PARTY.md`](THIRD_PARTY.md) | third-party components and licenses, including the voice |

## License

The code is under the MIT license ([`LICENSE`](LICENSE)). Art, music, stories and characters are
original, generated by the scripts in this repository. The Windows and macOS packages include SDL2
(zlib license, `LICENSE-SDL2.txt`).

**The voice** is synthesized offline with the Piper model *it_IT-paola-medium*. Its dataset is CC0, but
the model was fine-tuned from *en_US-lessac-medium*, whose training corpus is licensed for research only,
without redistribution. Whether the generated phrases are a derivative work of that corpus is debatable.
To replace the voice, put recordings in `data/voce/registrate/<id>.wav` (they win over the synthesis)
or regenerate the phrases with another model: see [`THIRD_PARTY.md`](THIRD_PARTY.md).
