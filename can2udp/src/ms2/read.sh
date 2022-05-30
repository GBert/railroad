#!/bin/sh

for i in `ls -1 *.bin`
do
       	echo $i
	length=$(../read_lococard $i | egrep "^ID" | cut -d" " -f2 | cut -b 1-5)
	../read_lococard $i | egrep "^ID|proto|$length"
	echo
done
