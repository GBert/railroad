# Copyright (C) 2025 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=mswebapp
PKG_RELEASE:=1.0

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/$(PKG_NAME)
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=MobileStation Web App 
  DEPENDS:= +can2udp +python +libstdcpp
endef

define Package/$(PKG_NAME)/description
  Mobile Station Web App from Ralf Mende
  Maerklin Mobile Station inspired Web UI to control locomotives and
  accessories (CS2/CS3 style).
endef

#define Build/Prepare
#        $(CP) ./src/* $(PKG_BUILD_DIR)
#endef

#define Build/Compile
#endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mswebapp_cpp $(1)/usr/bin/mswebapp
	$(INSTALL_DIR) $(1)/opt/mswebapp
	$(CP) ./files/python/* $(1)/opt/mswebapp
	$(INSTALL_DIR) $(1)/opt/mswebapp/www
#	$(CP) ./files/www/* $(1)/opt/mswebapp/www
	$(INSTALL_DIR) $(1)/opt/mswebapp/var
	$(INSTALL_DIR) $(1)/opt/mswebapp/static
	$(CP) ./files/static/* $(1)/opt/mswebapp/static
	$(INSTALL_DIR) $(1)/etc/uci-defaults
	$(INSTALL_DATA) ./files/mswebapp $(1)/etc/uci-defaults/90_mswebapp
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
