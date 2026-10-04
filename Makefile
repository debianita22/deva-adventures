# Deva's Awesome Adventures - libretro core
#
#   make                  host build (build/host, for testing on this PC)
#   make aarch64          release build for arm64 handhelds (RK3326), Zig, glibc >= 2.17
#   make x86_64           release build for a Linux PC, Zig, glibc >= 2.17
#   make linux            the game on its own for a Linux PC, no RetroArch (platform/sdl, SDL2 at run time)
#   make linux-x86_64     the same, release build (Zig, glibc >= 2.17)
#   make windows          the same for Windows 10 and 11, x86_64 (Zig; SDL2.dll goes beside it)
#   make mac              the same for macOS, arm64 + x86_64 in one (on a Mac, with Xcode's clang)
#   make test-linux       the PC program on a virtual screen (Xvfb) and its install.sh
#   make test-windows     the Windows package under Wine on a virtual screen (after make dist)
#   make test             the automated playthroughs and the save tests (build/test)
#   make asan             the same core and harness with AddressSanitizer + UBSan
#   make dist             the release packages in release/ (source, aarch64 and x86_64 drop-ins)
#   make test-dist        install.sh and uninstall.sh of those packages, on fake RetroArch folders
#   make install          core, .info and data under $(DESTDIR)$(PREFIX) (distributions)
#   make install-linux    the PC program, its data, menu entry and icon under $(DESTDIR)$(PREFIX)
#   make harness          the headless test frontend (tools/harness)
#   make harness-aarch64  the same for aarch64, to run under qemu-aarch64 (docs/SVILUPPO.md)
#
# Buildroot (devaOS) and Lakka pass their own CC/CFLAGS: see packaging/.

TARGET   ?= deva_adventures_libretro.so
BUILDDIR ?= build/host
CC       ?= cc
CFLAGS   ?= -O2 -g
PREFIX   ?= /usr
DATA_DIR ?= $(PREFIX)/share/deva_adventures
LIBRETRO_DIR ?= $(PREFIX)/lib/libretro
INFO_DIR ?= $(LIBRETRO_DIR)

SRC = src/libretro.c src/game.c src/gfx.c src/audio.c src/save.c src/hero.c src/fx.c src/quiz.c src/hud.c \
      src/scene_title.c src/scene_prova.c src/scene_menu.c src/scene_conta.c src/scene_parole.c \
      src/scene_sequenze.c src/scene_nome.c src/scene_memory.c src/scene_ritmo.c \
      src/scene_dove.c src/scene_emozioni.c src/scene_storie.c \
      src/story.c src/scene_mappa.c src/scene_sfida.c \
      src/scene_balla.c src/scene_premio.c src/scene_fine.c \
      src/ui.c src/scene_salvataggi.c src/scene_tastiera.c src/scene_opzioni.c src/scene_camerino.c \
      src/scene_album.c src/scene_ginnastica.c src/scene_trucco.c src/scene_forme.c src/scene_duelli.c \
      src/trans.c src/amb.c src/scene_lettere.c src/scene_ombre.c src/scene_sentiero.c \
      src/scene_negozio.c src/scene_misure.c src/anim.c src/plat.c
OBJ = $(SRC:src/%.c=$(BUILDDIR)/%.o) $(BUILDDIR)/third_party_impl.o

WARN      = -Wall -Wextra -Wshadow -Wno-unused-parameter
BASEFLAGS = -std=c99 -fPIC -fvisibility=hidden -D_POSIX_C_SOURCE=200809L \
            -DGAME_DATA_DIR='"$(DATA_DIR)"' -Isrc -Ithird_party

.PHONY: all clean aarch64 x86_64 linux linux-x86_64 windows win-exe sdl2-windows mac harness harness-aarch64 test \
        test-linux test-windows asan dist test-dist install uninstall install-linux uninstall-linux

all: $(BUILDDIR)/$(TARGET)

$(BUILDDIR)/$(TARGET): $(OBJ) link.T
	$(CC) $(CFLAGS) $(LDFLAGS) -shared -Wl,--version-script=link.T -Wl,--no-undefined -o $@ $(OBJ) -lm -lpthread

$(BUILDDIR)/%.o: src/%.c src/*.h | $(BUILDDIR)
	$(CC) $(CFLAGS) $(BASEFLAGS) $(WARN) -c -o $@ $<

# third-party code: built as is, warnings off
$(BUILDDIR)/third_party_impl.o: third_party/third_party_impl.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $(BASEFLAGS) -w -c -o $@ $<

$(BUILDDIR):
	mkdir -p $@

# the game on its own for a Linux PC: the same objects plus platform/sdl, SDL2 loaded at run time
LINUX_BIN = deva-adventures

SDL_OBJ = $(BUILDDIR)/main_sdl.o $(BUILDDIR)/os_sdl.o
PIE = -pie

$(BUILDDIR)/$(LINUX_BIN): $(OBJ) $(SDL_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $(PIE) -o $@ $(OBJ) $(SDL_OBJ) -lm -lpthread -ldl

$(BUILDDIR)/main_sdl.o: platform/sdl/main.c platform/sdl/*.h src/*.h | $(BUILDDIR)
	$(CC) $(CFLAGS) $(BASEFLAGS) $(WARN) -c -o $@ $<

$(BUILDDIR)/os_sdl.o: platform/sdl/os.c platform/sdl/os.h | $(BUILDDIR)
	$(CC) $(CFLAGS) $(BASEFLAGS) $(WARN) -c -o $@ $<

linux: $(BUILDDIR)/$(LINUX_BIN)

# Windows: a GUI program (no console window) with its icon, version and manifest (UTF-8 paths,
# crisp pixels on scaled displays); the UCRT of Windows 10 and 11, winpthreads built in
VERSION := $(shell sed -n 's/^\#define GAME_VERSION "\(.*\)"/\1/p' src/common.h)
comma := ,
WIN_BIN = deva-adventures.exe

$(BUILDDIR)/deva-adventures.manifest: packaging/windows/deva-adventures.manifest.in src/common.h | $(BUILDDIR)
	sed 's/@VERSION4@/$(VERSION).0/' $< > $@

$(BUILDDIR)/deva-adventures.rc: packaging/windows/deva-adventures.rc.in $(BUILDDIR)/deva-adventures.manifest | $(BUILDDIR)
	sed -e 's/@VERSIONC@/$(subst .,$(comma),$(VERSION))/' -e 's/@VERSION@/$(VERSION)/' \
	    -e 's#@MANIFEST@#$(BUILDDIR)/deva-adventures.manifest#' $< > $@

$(BUILDDIR)/$(WIN_BIN): $(OBJ) $(SDL_OBJ) $(BUILDDIR)/deva-adventures.rc packaging/windows/deva-adventures.ico
	$(CC) $(CFLAGS) $(LDFLAGS) -Wl,--subsystem,windows -o $@ $(OBJ) $(SDL_OBJ) $(BUILDDIR)/deva-adventures.rc

win-exe: $(BUILDDIR)/$(WIN_BIN)

ZIGCC = python3 -m ziglang cc -target aarch64-linux-gnu.2.17 -mcpu=cortex_a35
ZIGCC_X64 = python3 -m ziglang cc -target x86_64-linux-gnu.2.17
ZIGCC_WIN = python3 -m ziglang cc -target x86_64-windows-gnu
RELEASE_CFLAGS = -O2 -ffunction-sections -fdata-sections
RELEASE_LDFLAGS = -s -Wl,--gc-sections

aarch64:
	$(MAKE) BUILDDIR=build/aarch64 CC="$(ZIGCC)" CFLAGS="$(RELEASE_CFLAGS)" LDFLAGS="$(RELEASE_LDFLAGS)"

x86_64:
	$(MAKE) BUILDDIR=build/x86_64 CC="$(ZIGCC_X64)" CFLAGS="$(RELEASE_CFLAGS)" LDFLAGS="$(RELEASE_LDFLAGS)"

linux-x86_64:
	$(MAKE) BUILDDIR=build/x86_64 CC="$(ZIGCC_X64)" CFLAGS="$(RELEASE_CFLAGS)" LDFLAGS="$(RELEASE_LDFLAGS)" linux

windows:
	$(MAKE) BUILDDIR=build/win64 CC="$(ZIGCC_WIN)" CFLAGS="$(RELEASE_CFLAGS)" LDFLAGS="-s" win-exe

# SDL2 for the Windows package: the official build, these bytes only (packaging/sdl2/README.md)
SDL2_VERSION = 2.32.10
SDL2_WIN_ZIP = build/sdl2/SDL2-$(SDL2_VERSION)-win32-x64.zip
SDL2_WIN_SHA256 = 6cf9706eefd0a4a06dc764007934d428afaf029fabdd408a9e646048c91e18fb
SDL2_URL = https://github.com/libsdl-org/SDL/releases/download/release-$(SDL2_VERSION)

$(SDL2_WIN_ZIP):
	mkdir -p build/sdl2
	curl -fsSL -o $@.part $(SDL2_URL)/SDL2-$(SDL2_VERSION)-win32-x64.zip
	echo "$(SDL2_WIN_SHA256)  $@.part" | sha256sum -c -
	mv $@.part $@

sdl2-windows: $(SDL2_WIN_ZIP)

# macOS, on a Mac: one program for arm64 and x86_64 (Xcode's clang; programs there are always PIE);
# tools/release/mkapp.sh makes the app and the disk image around it
MAC_CC = clang -arch arm64 -arch x86_64 -mmacosx-version-min=10.13

mac:
	$(MAKE) BUILDDIR=build/mac CC="$(MAC_CC)" CFLAGS="-O2" LDFLAGS="-Wl,-dead_strip" PIE= linux

# release/: deva-adventures-<v>-src.tar.gz, -aarch64.zip, -x86_64.zip, -linux-x86_64.tar.gz,
# -windows-x64.zip and -setup.exe, SHA256SUMS (tools/release); the macOS one comes from mkapp.sh
dist: aarch64 x86_64 linux-x86_64 windows sdl2-windows
	python3 tools/release/mkdist.py --out release

# the PC program on a virtual X screen (Xvfb, openbox, xdotool, ImageMagick), then the install.sh of
# the PC package when release/ has it
test-linux: linux
	./tools/harness/test_linux.sh

# the Windows package of release/ under Wine on a virtual X screen (make dist first)
test-windows:
	./tools/harness/test_windows.sh

# the installer of the packages on fake RetroArch folders, with busybox when there is one
test-dist: harness
	./tools/release/test_install.sh

install: $(BUILDDIR)/$(TARGET)
	install -D -m 0755 $(BUILDDIR)/$(TARGET) $(DESTDIR)$(LIBRETRO_DIR)/$(TARGET)
	install -D -m 0644 deva_adventures_libretro.info $(DESTDIR)$(INFO_DIR)/deva_adventures_libretro.info
	mkdir -p $(DESTDIR)$(DATA_DIR)
	cp -R data/deva_adventures/. $(DESTDIR)$(DATA_DIR)/

uninstall:
	rm -f $(DESTDIR)$(LIBRETRO_DIR)/$(TARGET) $(DESTDIR)$(INFO_DIR)/deva_adventures_libretro.info
	rm -rf $(DESTDIR)$(DATA_DIR)

# the PC program finds its data in <bin>/../share/deva_adventures or in DATA_DIR (built in)
install-linux: $(BUILDDIR)/$(LINUX_BIN)
	install -D -m 0755 $(BUILDDIR)/$(LINUX_BIN) $(DESTDIR)$(PREFIX)/bin/$(LINUX_BIN)
	install -D -m 0644 packaging/linux/deva-adventures.desktop \
	        $(DESTDIR)$(PREFIX)/share/applications/deva-adventures.desktop
	install -D -m 0644 packaging/linux/deva-adventures.png \
	        $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/deva-adventures.png
	mkdir -p $(DESTDIR)$(DATA_DIR)
	cp -R data/deva_adventures/. $(DESTDIR)$(DATA_DIR)/

uninstall-linux:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(LINUX_BIN) $(DESTDIR)$(PREFIX)/share/applications/deva-adventures.desktop \
	      $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/deva-adventures.png
	rm -rf $(DESTDIR)$(DATA_DIR)

# the test frontend for aarch64, to run under qemu-user with a target rootfs:
#   qemu-aarch64 -L <rootfs> build/aarch64/harness build/aarch64/deva_adventures_libretro.so ...
harness-aarch64:
	mkdir -p build/aarch64
	$(ZIGCC) -O2 -std=c99 -D_POSIX_C_SOURCE=200809L -Ithird_party -o build/aarch64/harness \
	        tools/harness/harness.c -ldl -lm

harness: $(BUILDDIR)/$(TARGET)
	$(CC) -O2 -g -std=c99 -D_POSIX_C_SOURCE=200809L -Ithird_party -o build/harness tools/harness/harness.c -ldl -lm

test: harness
	./tools/harness/run_tests.sh
	./tools/harness/test_saves.sh

# AddressSanitizer + UBSan build of core and harness, then the playthroughs
asan:
	$(MAKE) BUILDDIR=build/asan CFLAGS="-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer" \
	        LDFLAGS="-fsanitize=address,undefined"
	$(CC) -O1 -g -fsanitize=address,undefined -std=c99 -D_POSIX_C_SOURCE=200809L -Ithird_party \
	        -o build/harness-asan tools/harness/harness.c -ldl -lm

clean:
	rm -rf build release
