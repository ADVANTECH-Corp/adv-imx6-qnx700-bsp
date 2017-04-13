#!/bin/sh

INAND=1
DEVEX="/dev/emmc0"
PARTATION=0
IMGDIR=/mnt/nfs/img


program() {

cat << EOF

Transfer U-Boot to target device

EOF


if [[ -z $1 ]]; then
cat << EOF
SYNOPSIS:
    $PROGNAME {device node}

EXAMPLE:
    $0 $DEVEX

EOF
    exit 1
fi

[ ! -e $1 ] && echo "Device $1 not found" && exit 1

echo "All data on "$1" now will be destroyed! Continue? [y/n]"
read ans
if [ $ans != 'y' ]; then exit 1; fi

DRIVE=$1

## Clear partition table
if [ $PARTATION == 1 ]; then
	echo "[Partitioning $1...]"
	dd if=/dev/zero of=$DRIVE bs=512 count=2 conv=sync
	#end of cylinders=2982
	fdisk $DRIVE add -t 77 -c16,2982
	slay devb-sdmmc-mx6_generic
	devb-sdmmc-mx6_generic cam quiet blk rw,cache=2M sdio addr=0x0219c000,irq=57,bs=nocd disk name=emmc
	sleep 2
fi

#echo "[Copy u-boot.bin]"
#dd if=$IMGDIR/u-boot_crc.bin.crc of=$1 bs=512 seek=2 conv=sync &>/dev/null
#dd if=$IMGDIR/u-boot_crc.bin     of=$1 bs=512 seek=3 conv=sync &>/dev/null

echo "[Copy ifs-mx6q-sabresmart-graphics.bin]"
dd if=$IMGDIR/ifs-mx6q-sabresmart-graphics.bin of=$1 bs=1024k seek=5 conv=sync

if [ $PARTATION == 1 ]; then
	DPART=`ls -1 ${DRIVE}t77 2> /dev/null`
	[ -z $DPART ] && DPART=`ls -1 ${DRIVE}t77 2> /dev/null`
	[ -z $DPART ] && echo "$DRIVE's partition 1 not found" && exit 1

	echo "[Making qnx6fs filesystems...]"

	if ! mkqnx6fs -q ${DRIVE}t77 ; then
    		echo  "Cannot build a filesystem on $DPART"
    		exit 1
	fi
fi

#echo "[Copying rootfs...]"
#if ! mount -t qnx6  ${DRIVE}t77 /mnt &> /dev/null; then
#    echo  "Cannot mount ${DRIVE}t77"
#    exit 1
#fi

#cp -a ../image/rootfs/* /mnt &> /dev/null

#umount /mnt

echo "[Done]"

}

program $DEVEX

