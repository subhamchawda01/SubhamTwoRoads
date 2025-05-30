#!/bin/bash

if [ $# -lt 1 ] ; then echo "USAGE: $0 src_fullpath [tgt_fullpath=src_fullpath]"; exit; fi;

src=$1; shift;
tgt=$src;
if [ $# -gt 0 ] ; then tgt=$1; shift; fi

tgt_dir=`dirname $tgt`;
ssh -o ConnectTimeout=60 54.208.92.178 "mkdir -p $tgt_dir";
rsync -avz --timeout=60 $src 54.208.92.178:$tgt;
ssh -o ConnectTimeout=60 54.208.92.178 "sh /home/dvctrader/basetrade/AwsScripts/update_file_on_workers.sh $src $tgt"
