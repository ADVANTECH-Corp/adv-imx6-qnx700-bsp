#!/bin/sh
# script to build a binary IPL and boot image for i.MX6Q Sabre Smart board. 
# NOTE the image (ipl-ifs-mx6q-sabresmart.bin) must be built as binary, i.e. [virtual=armle,binary] in the buildfile 
set -v

#	Convert IPL into BINARY format
${QNX_HOST}/usr/bin/ntoarmv7-objcopy --input-format=elf32-littlearm --output-format=binary ../install/armle-v7/boot/sys/ipl-mx6q-sabresmart ./tmp-ipl-mx6q-sabresmart.bin

${QNX_HOST}/usr/bin/ntoarmv7-objcopy --input-format=elf32-littlearm --output-format=binary ../install/armle-v7/boot/sys/ipl-mx6q-sabresmart-enableTZASC ./tmp-ipl-mx6q-sabresmart-enableTZASC.bin

#	Pad BINARY IPL
mkrec  -ffull -r tmp-ipl-mx6q-sabresmart.bin > ipl-mx6q-sabresmart.bin
mkrec  -ffull -r tmp-ipl-mx6q-sabresmart-enableTZASC.bin > ipl-mx6q-sabresmart-enableTZASC.bin

rm tmp-ipl-mx6q-sabresmart.bin tmp-ipl-mx6q-sabresmart-enableTZASC.bin
