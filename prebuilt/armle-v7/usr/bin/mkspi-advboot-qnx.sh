#!/bin/sh

IMG_FILE=/img/adv_boot_1G_micron.bin 
#IMG_FILE=/img/adv_boot.bin 
#IMG_FILE=/mnt/nfs/img/adv_boot.bin 
 
if  [[ -e $IMG_FILE ]]
then
	echo "[Copy adv_boot.bin]"
	#flashctl -p /dev/fs0 -ev
	flashctl -p /dev/fs0 -o 1k -l 256k -e -v
	dd if=$IMG_FILE of=/dev/fs0 bs=512 seek=2 skip=2 
else
	echo "adv_boot.bin did not exist"	
fi

echo "[Done]"
