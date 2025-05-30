#!/bin/bash

dt=`date +%Y%m%d`

if [ $# -gt 0 ];then
    dt=$1;
fi

for exch in RTS
do
    scount=`ssh circulumvite@10.242.128.114 "ls -ltr /usr/MDSlogs/$exch/*$dt* | wc -l" 2>/tmp/rts_m1_dc`
    ssh circulumvite@10.242.128.114 "gzip /usr/MDSlogs/$exch/*$dt" 2>>/tmp/rts_m1_dc
    dest_dir=/apps/data/RTSLoggedData/M1/${dt:0:4}/${dt:4:2}/${dt:6:2}
    ssh dvcinfra@10.23.74.40 "mkdir -p $dest_dir" 2>>/tmp/rts_m1_dc
    rsync -avz  circulumvite@10.242.128.114:/usr/MDSlogs/$exch/*_$dt.gz /spare/local/MDSlogs/RTS_M1/ 2>>/tmp/rts_m1_dc
    rsync -avz /spare/local/MDSlogs/RTS_M1/*_$dt.gz dvcinfra@10.23.74.40:$dest_dir/ 2>>/tmp/rts_m1_dc
    dcount=`ls -ltr /NAS1/data/RTSLoggedData/M1/${dt:0:4}/${dt:4:2}/${dt:6:2}/* | wc -l 2>>/tmp/rts_m1_dc` ;
    rm /spare/local/MDSlogs/RTS_M1/*
    if [ $scount == $dcount ]; then 
	ssh circulumvite@10.242.128.114 "rm /usr/MDSlogs/$exch/*_$dt.gz" ;
    else
	/bin/mail -s "RTS_M1_DC" -r "dvcinfraOnNy11" "kp@circulumvite.com" < /tmp/rts_m1_dc ; 	
    fi    
done
