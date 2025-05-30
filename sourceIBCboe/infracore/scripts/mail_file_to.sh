#!/bin/sh

if [ $# -ne 3 ] ; then
    echo "USAGE: $0 sub toaddr file";
    exit;
fi

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


SUBJECT=$1; shift;
TOADDR=$1; shift;
FILENAME=$1; shift;

mailx -s $SUBJECT -r$USER@circulumvite.com $TOADDR < $FILENAME
