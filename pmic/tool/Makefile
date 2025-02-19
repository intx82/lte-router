#
# Copyright (C) 2025 intx82 <intx82@gmail.com>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=pmicctrl
PKG_RELEASE:=1
PKG_FLAGS:=nonshared

include $(INCLUDE_DIR)/package.mk
define Package/pmicctrl
  SECTION:=utils
  CATEGORY:=Base system
  TITLE:=PMIC Control utillity
  DEPENDS:=+libstdcpp +libubus +libubox +libjson-c +libblobmsg-json +libuci
endef

define Package/pmicctrl/description
 Power managment utility
endef

# define Build/Compile
# 	$(TARGET_CC) $(TARGET_CFLAGS) -o $(PKG_BUILD_DIR)/pmicctrl.o -c $(PKG_BUILD_DIR)/pmicctrl.c
# 	$(TARGET_CC) $(TARGET_LDFLAGS) -o $(PKG_BUILD_DIR)/$1 $(PKG_BUILD_DIR)/pmicctrl.o
# endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) TOOLCHAIN="$(TARGET_CROSS)"
endef


define Package/pmicctrl/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/pmicctrl.elf $(1)/usr/bin/pmicctrl
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/pmic.daemon $(1)/etc/init.d/pmic.daemon
endef

$(eval $(call BuildPackage,pmicctrl))