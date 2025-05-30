#!/bin/bash

if [ $# -lt 1 ] ; then echo "USAGE: $0 src_fullpath [tgt_fullpath=src_fullpath]"; exit; fi;

src=$1; shift;
tgt=$src;
if [ $# -gt 0 ] ; then tgt=$1; shift; fi 

tgt_dir=`dirname $tgt`;
#Updating local exec directories on workers
for loc in `grep -v "10.0.0." /mnt/sdf/JOBS/all_instances.txt | grep running | awk '{print $NF}'`;
do
    ssh -o ConnectTimeout=60 $loc "mkdir -p $tgt_dir";
    echo "rsync -ravz --timeout=60 $src/ $loc:$tgt/";
    rsync -ravz --timeout=60 -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" $src/ $loc:$tgt/;
done
