# SPDX-License-Identifier: MIT
# Deva's Awesome Adventures - package for Lakka (LibreELEC build system).
# Copy this folder to packages/lakka/libretro_cores/deva_adventures in the Lakka tree and add
# deva_adventures to LIBRETRO_CORES in packages/lakka/libretro_cores/package.mk (see README.md).

PKG_NAME="deva_adventures"
PKG_VERSION="1.0.0"
PKG_LICENSE="MIT"
PKG_SITE="" # no public repository yet: the source tree is local (DEVA_ADVENTURES_SRC below)
PKG_URL=""
PKG_DEPENDS_TARGET="toolchain"
PKG_LONGDESC="Deva's Awesome Adventures: educational contentless libretro core for 5-year-olds (Italian voice), 18 games and 4 stories."
PKG_TOOLCHAIN="make"

# The source tree: by default the checkout next to the Lakka tree; override with
# DEVA_ADVENTURES_SRC=/path/to/deva-adventures in the environment of the build.
DEVA_ADVENTURES_SRC="${DEVA_ADVENTURES_SRC:-${ROOT}/../deva-adventures}"

PKG_MAKE_OPTS_TARGET="BUILDDIR=build/target DATA_DIR=/usr/share/deva_adventures"

unpack() {
  [ -f "${DEVA_ADVENTURES_SRC}/src/libretro.c" ] || die "deva_adventures: no source tree in ${DEVA_ADVENTURES_SRC}"
  mkdir -p "${PKG_BUILD}"
  # everything but the developer's build outputs and release packages
  tar -C "${DEVA_ADVENTURES_SRC}" --exclude=./build --exclude=./release --exclude=./.git -cf - . | tar -C "${PKG_BUILD}" -xf -
}

# -O2 with the full vectorizer (the sprite and fade loops are written for it: same pixels, NEON),
# unused sections dropped at link time
pre_make_target() {
  export CFLAGS="${CFLAGS} -O2 -ftree-vectorize -fvect-cost-model=dynamic -ffunction-sections -fdata-sections"
  export LDFLAGS="${LDFLAGS} -Wl,--gc-sections"
}

makeinstall_target() {
  mkdir -p ${INSTALL}/usr/lib/libretro ${INSTALL}/usr/share/deva_adventures
  cp -v build/target/deva_adventures_libretro.so ${INSTALL}/usr/lib/libretro/
  cp -v deva_adventures_libretro.info ${INSTALL}/usr/lib/libretro/
  cp -PR data/deva_adventures/. ${INSTALL}/usr/share/deva_adventures/
}
