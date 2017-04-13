#!/bin/sh

#Write MAC address

TMP_FILE=/tmp/mac_addr

hex_to_ascii() {
	echo -e $1 | awk '{printf "%c", $1}'
}

OFFSET=851968

rm -f $TMP_FILE

for i in $1 $2 $3 $4 $5 $6
do
	hex_to_ascii $i >> $TMP_FILE
done

flashctl -p /dev/fs0 -o $OFFSET -l 64k -e -v
dd if=$TMP_FILE of=/dev/fs0 bs=1 seek=$OFFSET
sync;sync;sync

#Read MAC address
#hd -s $OFFSET -n 6 /dev/fs0

