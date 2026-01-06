# Copyright (C) 2025 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=mswebapp
PKG_RELEASE:=1.1.8

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/$(PKG_NAME)
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=MobileStation Web App 
  DEPENDS:= +can2udp +libstdcpp
endef

define Package/$(PKG_NAME)/description
  Mobile Station Web App from Ralf Mende
  Maerklin Mobile Station inspired Web UI to control locomotives and
  accessories (CS2/CS3 style).
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mswebapp_cpp $(1)/usr/bin/mswebapp
	$(INSTALL_DIR) $(1)/opt/mswebapp/www/static
	$(CP) ./files/www/static/* $(1)/opt/mswebapp/www/static
	$(INSTALL_DIR) $(1)/opt/mswebapp/www/static/grafics
	$(CP) ./files/www/static/grafics/* $(1)/opt/mswebapp/www/static/grafics
	$(INSTALL_DIR) $(1)/opt/mswebapp/www/templates
	$(CP) ./files/www/templates/* $(1)/opt/mswebapp/www/templates
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/mswebapp.init $(1)/etc/init.d/mswebapp
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
