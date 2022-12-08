#!/bin/bash

# fügt über config broadcast eine Lok hinzu

cat > /www/config/bctest << _EOF_
[lokomotive]
lokomotive
 .name=BR 89
 .uid=0x14
 .vorname=
 .adresse=0x14
 .typ=mm2_dil8
 .sid=0x65
 .icon=BR 80
 .symbol=2
 .av=20
 .tachomax=80
 .mfxtyp=110
 .funktionen
 ..nr=0
 ..typ=1
 .funktionen
 ..nr=1
 .funktionen
 ..nr=2
 .funktionen
 ..nr=3
 .funktionen
 ..nr=4
 .funktionen
 ..nr=5
 .funktionen
 ..nr=6
 .funktionen
 ..nr=7
 .funktionen
 ..nr=8
 .funktionen
 ..nr=9
 .funktionen
 ..nr=10
 .funktionen
 ..nr=11
 .funktionen
 ..nr=12
 .funktionen
 ..nr=13
 .funktionen
 ..nr=14
 .funktionen
 ..nr=15
_EOF_

cansend can0 00434775#6263746573740000

