# Deva's Awesome Adventures - libretro core
#
#   make                  host build (build/host, for testing on this PC)
#   make aarch64          release build for arm64 handhelds (RK3326), Zig, glibc >= 2.17
#   make x86_64           release build for a Linux PC, Zig, glibc >= 2.17
#   make test             the automated playthroughs and the save tests (build/test)
#   make asan             the same core and harness with AddressSanitizer + UBSan
#   make dist             the release packages in release/ (source, aarch64 and x86_64 drop-ins)
#   make test-dist        install.sh and uninstall.sh of those packages, on fake RetroArch folders
#   make install          core, .info and data under $(DESTDIR)$(PREFIX) (distributions)
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
      src/scene_negozio.c src/scene_misure.c src/anim.c
OBJ = $(SRC:src/%.c=$(BUILDDIR)/%.o) $(BUILDDIR)/third_party_impl.o

WARN      = -Wall -Wextra -Wshadow -Wno-unused-parameter
BASEFLAGS = -std=c99 -fPIC -fvisibility=hidden -D_POSIX_C_SOURCE=200809L \
            -DGAME_DATA_DIR='"$(DATA_DIR)"' -Isrc -Ithird_party

.PHONY: all clean aarch64 x86_64 harness harness-aarch64 test asan dist test-dist install uninstall

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

ZIGCC = python3 -m ziglang cc -target aarch64-linux-gnu.2.17 -mcpu=cortex_a35
ZIGCC_X64 = python3 -m ziglang cc -target x86_64-linux-gnu.2.17
RELEASE_CFLAGS = -O2 -ffunction-sections -fdata-sections
RELEASE_LDFLAGS = -s -Wl,--gc-sections

aarch64:
	$(MAKE) BUILDDIR=build/aarch64 CC="$(ZIGCC)" CFLAGS="$(RELEASE_CFLAGS)" LDFLAGS="$(RELEASE_LDFLAGS)"

x86_64:
	$(MAKE) BUILDDIR=build/x86_64 CC="$(ZIGCC_X64)" CFLAGS="$(RELEASE_CFLAGS)" LDFLAGS="$(RELEASE_LDFLAGS)"

# release/: deva-adventures-<v>-src.tar.gz, -aarch64.zip, -x86_64.zip and SHA256SUMS (tools/release)
dist: aarch64 x86_64
	python3 tools/release/mkdist.py --out release

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
