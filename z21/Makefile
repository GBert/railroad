# Copyright (C) 2021 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=z21emu
PKG_RELEASE:=2.17

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Roco Z21 Emulator
  DEPENDS:=+zlib +libsqlite3 +libcurl +libuuid +minizip
endef

define Package/$(PKG_NAME)/description
  Roco/Fleischmann z21 Emulator
endef

define Build/Prepare
	$(CP) ./src/* $(PKG_BUILD_DIR)
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) $(TARGET_CONFIGURE_OPTS) \
		CFLAGS="$(TARGET_CFLAGS) $(TARGET_CPPFLAGS) -std=gnu99 -DNO_XPN_TTY"
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/z21emu $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/cs2toz21 $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/z21emu.init $(1)/etc/init.d/z21emu
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
