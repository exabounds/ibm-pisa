#!/bin/sh

wc -l $1 | awk -F" " '{print $1}' > $2
awk -F" " '{if (NF==7) {print $2 " "$3 " "$4 " " $5}}' $1 >> $2
