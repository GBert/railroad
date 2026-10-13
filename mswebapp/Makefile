# Copyright (C) 2025 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.

include $(TOPDIR)/rules.mk

PKG_NAME:=mswebapp
PKG_VERSION:=1.2.0
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/RalfMende/MobileStationWebApp.git
PKG_SOURCE_VERSION:=v1.2.0

PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

CMAKE_SOURCE_DIR:=src/backend

define Package/$(PKG_NAME)
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=MobileStation Web App
  DEPENDS:=+libstdcpp
endef

define Package/$(PKG_NAME)/description
  Maerklin Mobile Station inspired web UI to control locomotives and
  accessories using CS2/CS3-style configuration files.
endef

CMAKE_OPTIONS += \
	-DMSWEBAPP_WITH_CAN=ON

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mswebapp $(1)/usr/bin/mswebapp

	$(INSTALL_DIR) $(1)/usr/share/mswebapp/www
	$(CP) $(PKG_BUILD_DIR)/src/frontend/. $(1)/usr/share/mswebapp/www/

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/packaging/openwrt/mswebapp/files/etc/init.d/mswebapp $(1)/etc/init.d/mswebapp
endef

$(eval $(call BuildPackage,$(PKG_NAME)))