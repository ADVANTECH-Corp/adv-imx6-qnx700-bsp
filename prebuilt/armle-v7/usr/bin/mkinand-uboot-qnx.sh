#!/bin/sh

SD_DEVICE="/dev/emmc0"
IMGDIR=/mnt/nfs/img
 
if  [[ -e $IMGDIR/u-boot_crc.bin ]]
then
	dd if=$IMGDIR/u-boot_crc.bin.crc of=${SD_DEVICE} conv=sync seek=2 bs=512
	dd if=$IMGDIR/u-boot_crc.bin of=${SD_DEVICE} conv=sync seek=3 bs=512
else
	echo "u-boot_crc.bin did bot exist"	
fi

echo "[Done]"
