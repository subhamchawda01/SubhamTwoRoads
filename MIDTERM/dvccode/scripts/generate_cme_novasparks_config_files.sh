#!/bin/bash
base_dir="/tmp"

#Download latest ref files from CME ftp
ssh 10.23.74.52 wget ftp://ftp.cmegroup.com/SBEFix/Production/secdef.dat.gz -O $base_dir/secdef.dat.gz
scp 10.23.74.52:$base_dir/secdef.dat.gz $base_dir/secdef.dat.gz
gunzip -f $base_dir/secdef.dat.gz

#Generate the config files
python /home/pengine/prod/live_scripts/CME-MDP3-SecDef_Parsing.py $base_dir/

grep -v "\-" /spare/local/files/CMEMDP/cme-ref.txt > /spare/local/files/CMEMDP/cme-ref.txt.tmp

#Filter only the securities for which we are actually using data
for secid in `awk '{printf "%s:%s ",$4,$1;}' /spare/local/files/CMEMDP/cme-ref.txt.tmp`; do sec_pair=`echo $secid | tr ':' ' '`; grep "$sec_pair" $base_dir/cme_instrument_dict_raw.txt ; done | sort | uniq > $base_dir/cme_instrument_dict.txt

#Copy updated files
cp $base_dir/cme_instrument_dict.txt /spare/local/files/CMEMDP/cme_instrument_dict.txt
cp $base_dir/cme_instrument_group_raw.txt /spare/local/files/CMEMDP/cme_instrument_group.txt

#Sync them across servers
scp /spare/local/files/CMEMDP/cme_instrument_* ivan@10.23.82.100:file_drop/
scp /spare/local/files/CMEMDP/cme_instrument_* 10.23.82.53:/spare/local/files/CMEMDP/
scp /spare/local/files/CMEMDP/cme_instrument_* 10.23.82.54:/spare/local/files/CMEMDP/
scp /spare/local/files/CMEMDP/cme_instrument_* 10.23.82.55:/spare/local/files/CMEMDP/
scp /spare/local/files/CMEMDP/cme_instrument_* 10.23.82.56:/spare/local/files/CMEMDP/

#Generate new FPGA config from these new files
ssh ivan@10.23.82.100 /home/ivan/scripts/restart-middle-bay.sh >/dev/null 2>&1 &
