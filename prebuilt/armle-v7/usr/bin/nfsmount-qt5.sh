#!/bin/sh
IP=192.168.88.10
SERVER_DIR=/home/weilun/nfs/qt5_imx6_bin/opt
MOUNT_DIR=/opt
fs-nfs3 $IP:$SERVER_DIR $MOUNT_DIR
