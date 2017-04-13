# Each board-specific cfglib includes this wfdcfg.mk file
# (and can override variables if required).

ifeq ($(WFD_SRCDIR),)
WFD_SRCDIR=$(PROJECT_ROOT)/../..
endif

IS_DEBUG_BUILD:=$(filter g, $(VARIANT_LIST))
DBG_LIBSUFFIX := $(if $(IS_DEBUG_BUILD),_g)
DBG_DIRSUFFIX := $(if $(IS_DEBUG_BUILD),-debug)

INSTALLDIR=$(firstword $(INSTALLDIR_$(OS)) \
    usr/lib/graphics/iMX6X$(DBG_DIRSUFFIX) )

LDOPTS+=-Wl,--exclude-libs,ALL

# These values should not be changed nor overridden,
# as they are dictated by a naming convention.
NAME=$(IMAGE_PREF_SO)wfdcfg-$(PROJECT)
SONAME_DLL=$(IMAGE_PREF_SO)wfdcfg$(IMAGE_SUFF_SO).0

DESC=Library to return port modes for iMX6x
define PINFO
PINFO DESCRIPTION = "$(DESC)"
endef

USEFILE=$(NAME).use
EXTRA_CLEAN += $(NAME).use

# To generate USEFILE, include usemsg.mk after qtargets.mk.
