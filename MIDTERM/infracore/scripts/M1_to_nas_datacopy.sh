#!/bin/bash

dt=`date +%Y%m%d `

if [ $# -gt 0 ];then
    dt=$1;
fi


for exch in MICEX
do
    count1=`ssh circulumvite@10.242.128.114 "ls -lrt /usr/MDSlogs/$exch/*$dt* | wc -l"` ;
    ssh circulumvite@10.242.128.114 "gzip /usr/MDSlogs/$exch/*$dt" # just for safety
    dest_dir=/apps/data/MICEXLoggedData/M1/${dt:0:4}/${dt:4:2}/${dt:6:2}
    ssh dvcinfra@10.23.74.40 "mkdir -p $dest_dir"
    rsync -avz  circulumvite@10.242.128.114:/usr/MDSlogs/$exch/*_$dt.gz /spare/local/MDSlogs/MICEX_M1/
    rsync -avz /spare/local/MDSlogs/MICEX_M1/*_$dt.gz dvcinfra@10.23.74.40:$dest_dir/
    rm /spare/local/MDSlogs/MICEX_M1/* 
    count2=`ls -ltr /NAS1/data/MICEXLoggedData/M1/${dt:0:4}/${dt:4:2}/${dt:6:2}/* | wc -l` ;
    if [ $count1 == $count2 ] ; then
	ssh circulumvite@10.242.128.114 "rm /usr/MDSlogs/$exch/*_$dt.gz" ;
    else
	echo "Could not delete from M1"
    fi
done


#for exch in MICEX
#do
#   ssh circulumvite@10.242.128.114 "gzip /usr/MDSlogs/$exch/*$dt" # just for safety
#   dest_dir=/apps/data/MICEXLoggedData/M1/${dt:0:4}/${dt:4:2}/${dt:6:2}
#   ssh dvcinfra@10.23.74.40 "mkdir -p $dest_dir"
#   rsync -avz  circulumvite@10.242.128.114:/usr/MDSlogs/$exch/*_$dt.gz /spare/local/MDSlogs/MICEX_M1/
#   rsync -avz /spare/local/MDSlogs/MICEX_M1/*_$dt.gz dvcinfra@10.23.74.40:$dest_dir/
#   rm /spare/local/MDSlogs/MICEX_M1/*  
#   count1= `ssh circulumvite@10.242.128.114 "ls -lrt /usr/MDSlogs/MICEX | wc -l"`
#   count2= `ssh dvcinfra@10.23.74.40 "ls -lrt $dest_dir | wc -l "`
#   if ($count1 -eq $count2)
#   then
#	ssh circulumvite@10.242.128.114 "rm /usr/MDSlogs/$exch/*_$dt.gz"
#    else
#	echo "Could not delete from M1"
#    fi
#done


