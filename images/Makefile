ROOT_DIR := $(notdir $(CURDIR))
ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

#weilun@adv - begin
include ../adv.mk
#weilun@adv - end

SRC := $(CURDIR)/../src
BOARD_ROOT := $(SRC)/hardware/startup/boards
HOST_MKIFS := mkifs

SUFFIXES := .build .ifs .raw

.PHONY: all clean 

#weilun@adv - begin
#all: ipl-mx6q-sabresmart.bin ifs-mx6q-sabresmart.bin  ifs-mx6q-sabresmart-graphics.bin
all: preinstall ifs-mx6q-sabresmart-graphics.bin
#weilun@adv - end

clean:
	$(RM_HOST) ifs-mx6q-sabresmart.bin ipl-mx6q-sabresmart.bin
	$(RM_HOST) ifs-mx6q-sabresmart-graphics.bin

#ipl-mx6q-sabresmart.bin:
#	$(SHELL) ./mkflashimage.sh

#ifs-mx6q-sabresmart.bin: smart-device.build
#	$(HOST_MKIFS) -vvv -r../install $(MKIFSFLAGS) $^ $@

#ifs-mx6q-sabresmart-graphics.bin: smart-device-graphics.build
#	$(HOST_MKIFS) -r../install -v $(MKIFSFLAGS) $^ $@

#weilun@adv - begin
preinstall:
ifeq ($(CONFIG_ADV_BOARD),rom5420)
	cp -f $(SRC)/hardware/startup/boards/imx6x/smart-device/arm/le.v7/startup-imx6x-smart-device  $(CURDIR)/../prebuilt/armle-v7/boot/sys/startup-imx6x-smart-device.rom5420
endif
ifeq ($(CONFIG_ADV_BOARD),rsb4410)
	cp -f $(SRC)/hardware/startup/boards/imx6x/smart-device/arm/le.v7/startup-imx6x-smart-device  $(CURDIR)/../prebuilt/armle-v7/boot/sys/startup-imx6x-smart-device.rsb4410
endif
ifeq ($(CONFIG_ADV_BOARD),rom7420)
	cp -f $(SRC)/hardware/startup/boards/imx6x/smart-device/arm/le.v7/startup-imx6x-smart-device  $(CURDIR)/../prebuilt/armle-v7/boot/sys/startup-imx6x-smart-device.rom7420
	cp -f $(SRC)/hardware/startup/boards/imx6x/smart-device/arm/le.v7/startup-imx6x-smart-device  $(CURDIR)/../install/armle-v7/boot/sys/startup-imx6x-smart-device.rom7420
endif
#ifeq ($(CONFIG_ADV_BOARD),dmsba16)
#	cp -f $(SRC)/hardware/startup/boards/imx6x/smart-device/arm/le.v7/startup-imx6x-smart-device  $(CURDIR)/../prebuilt/armle-v7/boot/sys/startup-imx6x-smart-device.dmsba16
#endif

ifeq ($(CONFIG_ADV_BOARD),rom7420)
ifs-mx6q-sabresmart-graphics.bin: smart-device-graphics.rom7420.build
	$(HOST_MKIFS) -vvv -r../install $(MKIFSFLAGS) $^ $@
endif
ifeq ($(CONFIG_ADV_BOARD),rom5420)
ifs-mx6q-sabresmart-graphics.bin: smart-device-graphics.rom5420.build
	$(HOST_MKIFS) -vvv -r../install $(MKIFSFLAGS) $^ $@
endif
ifeq ($(CONFIG_ADV_BOARD),rsb4410)
ifs-mx6q-sabresmart-graphics.bin: smart-device-graphics.rsb4410.build
	$(HOST_MKIFS) -vvv -r../install $(MKIFSFLAGS) $^ $@
endif
ifeq ($(CONFIG_ADV_BOARD),dmsba16)
ifs-mx6q-sabresmart-graphics.bin: smart-device-graphics.dmsba16.build
	$(HOST_MKIFS) -vvv -r../install $(MKIFSFLAGS) $^ $@
endif
#weilun@adv - end


