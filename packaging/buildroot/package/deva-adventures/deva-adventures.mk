################################################################################
#
# deva-adventures
#
################################################################################

DEVA_ADVENTURES_VERSION = 1.2.0
DEVA_ADVENTURES_SITE = $(call qstrip,$(BR2_PACKAGE_DEVA_ADVENTURES_SRC))
DEVA_ADVENTURES_SITE_METHOD = local
# the developer's build outputs and release packages stay out of the copy
DEVA_ADVENTURES_OVERRIDE_SRCDIR_RSYNC_EXCLUSIONS = --exclude /build --exclude /release
DEVA_ADVENTURES_LICENSE = MIT
DEVA_ADVENTURES_LICENSE_FILES = LICENSE THIRD_PARTY.md

DEVA_ADVENTURES_CORE_DIR = $(call qstrip,$(BR2_PACKAGE_DEVA_ADVENTURES_CORE_DIR))
DEVA_ADVENTURES_INFO_DIR = $(call qstrip,$(BR2_PACKAGE_DEVA_ADVENTURES_INFO_DIR))

# -O2 with the full vectorizer (the sprite and fade loops are written for it: same pixels, NEON),
# unused sections dropped at link time
DEVA_ADVENTURES_CFLAGS = $(TARGET_CFLAGS) -O2 -ftree-vectorize -fvect-cost-model=dynamic \
	-ffunction-sections -fdata-sections
DEVA_ADVENTURES_LDFLAGS = $(TARGET_LDFLAGS) -Wl,--gc-sections

define DEVA_ADVENTURES_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D) BUILDDIR=build/target \
		CC="$(TARGET_CC)" CFLAGS="$(DEVA_ADVENTURES_CFLAGS)" LDFLAGS="$(DEVA_ADVENTURES_LDFLAGS)" \
		DATA_DIR=/usr/share/deva_adventures
endef

define DEVA_ADVENTURES_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/build/target/deva_adventures_libretro.so \
		$(TARGET_DIR)$(DEVA_ADVENTURES_CORE_DIR)/deva_adventures_libretro.so
	$(INSTALL) -D -m 0644 $(@D)/deva_adventures_libretro.info \
		$(TARGET_DIR)$(DEVA_ADVENTURES_INFO_DIR)/deva_adventures_libretro.info
	mkdir -p $(TARGET_DIR)/usr/share/deva_adventures
	cp -a $(@D)/data/deva_adventures/. $(TARGET_DIR)/usr/share/deva_adventures/
endef

$(eval $(generic-package))
