#!/bin/bash

dt=`date +%Y%m%d`

if [ $# -gt 0 ];then
    dt=$1;
fi

for exch in RTS MICEX
do
    ssh circulumvite@10.240.128.34 "gzip /spare/local/MDSlogs/$exch/*$dt" # just for safety
    dest_dir=/apps/data/${exch}LoggedData/OTK/${dt:0:4}/${dt:4:2}/${dt:6:2}
    ssh dvcinfra@10.23.74.40 "mkdir -p $dest_dir"
    rsync -avz  circulumvite@10.240.128.34:/spare/local/MDSlogs/$exch/*_$dt.gz /spare/local/MDSlogs/$exch/
    rsync -avz /spare/local/MDSlogs/$exch/*_$dt.gz dvcinfra@10.23.74.40:$dest_dir/
    rm /spare/local/MDSlogs/$exch/*
done
