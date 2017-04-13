#!/bin/sh
IP=192.168.88.10
SERVER_DIR=/home/weilun/nfs
MOUNT_DIR=/mnt/nfs
fs-nfs3 $IP:$SERVER_DIR $MOUNT_DIR
