# Copyright (C) 2022 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=rocrail-bin
PKG_VERSION:=20220612

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

# Sart make install at build time

define Package/$(PKG_NAME)
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=rocrail-bin
  DEPENDS:=@(arm)
  URL:=https://www.rocrail.net
  MAINTAINER:=Gerhard Bertelsmann
endef

define Package/$(PKG_NAME)/description
  Rocrail bin installer
endef

define Build/Compile
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/opt/rocrail
	$(INSTALL_BIN) ./files/get_rocrail.sh $(1)/opt/rocrail
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
