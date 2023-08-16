#!/bin/sh

# get M*rklins CS2 image and extract some useful data
set -e

FILE=cs2update.img
FS_TYPE=reiserfs
LINK=https://streaming.maerklin.de/public-media/cs2/cs2update.img
MNT_POINT=/tmp/mnt
WEB_DIR=/www

cd /root

set -x

wget $LINK -O $FILE
mkdir -p $MNT_POINT
mount -o loop -t $FS_TYPE $FILE $MNT_POINT

cp -rp $MNT_POINT/home/cs2/fcticons $WEB_DIR
cp -rp $MNT_POINT/home/cs2/icons $WEB_DIR
cp -rp $MNT_POINT/home/cs2/magicons_ $WEB_DIR
cp -rp $MNT_POINT/home/cs2/doc $WEB_DIR
mkdir -rp $WEB_DIR/update
cp -p $MNT_POINT/home/cs2/update/*.bin $WEB_DIR/update
cp -p $MNT_POINT/home/cs2/update/*.ms2 $WEB_DIR/update
# cp -rp $MNT_POINT/home/cs2/spiel $WEB_DIR
cp -rp $MNT_POINT/home/cs2/doc $WEB_DIR

umount $MNT_POINT

