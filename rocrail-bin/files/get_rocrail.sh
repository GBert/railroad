#!/bin/sh

SNAPSHOT_SOURCE=https://wiki.rocrail.net/rocrail-snapshot/
SNAPSHOT=Rocrail-ARMHF.zip
SNAPSHOT=Rocrail-PiOS11-ARMHF.zip
CONFIG_FILES="plan.xml rocrail.ini rocnetnode.ini"
SAVE_DATE=$(date +%Y%m%d%H%M)

killall -q rocrail rocnetnode
cd /tmp
rm -f $SNAPSHOT
wget $SNAPSHOT_SOURCE$SNAPSHOT

# save config files if exists

for CONFIG_FILE in $CONFIG_FILES
do
  if [ -e /opt/rocrail/$CONFIG_FILE ]
  then
    cp /opt/rocrail/$CONFIG_FILE /opt/rocrail/$CONFIG_FILE$SAVE_DATE
  fi
done

unzip $SNAPSHOT -od /opt/rocrail
chmod +x /opt/rocrail/bin/rocrail

# restore config files

for CONFIG_FILE in $CONFIG_FILES
do
  if [ -e /opt/rocrail/$CONFIG_FILE$SAVE_DATE ]
  then
    cp /opt/rocrail/$CONFIG_FILE$SAVE_DATE /opt/rocrail/$CONFIG_FILE
  fi
done

