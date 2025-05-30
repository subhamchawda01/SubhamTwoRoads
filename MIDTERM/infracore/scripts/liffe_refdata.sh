#!/bin/bash

#ref data file path: TODO: switch to prod 
liffe_ref_date_file="/spare/local/files/LIFFE/liffe-ref.txt"

#backup the current ref file
cp $liffe_ref_date_file $liffe_ref_date_file"_bkp"

# Get the needed files from the EFS server.

DATE=`date +"%Y%m%d"`
/home/pengine/prod/live_scripts/liffe_efs_file_download.py $DATE


>$liffe_ref_date_file
/home/pengine/prod/live_execs/generate_liffe_refdata /spare/local/files/LIFFE/future_std_data_file.xml > $liffe_ref_date_file
/home/pengine/prod/live_execs/generate_liffe_refdata /spare/local/files/LIFFE/commodity_std_data_file.xml >> $liffe_ref_date_file 

# If file of size zero then dont sync
if [ `cat $liffe_ref_date_file | wc -l` -le 0 ]; then
 exit;
fi

#sync to bsl servers
scp $liffe_ref_date_file 10.23.52.51:$liffe_ref_date_file
scp $liffe_ref_date_file 10.23.52.52:$liffe_ref_date_file
scp $liffe_ref_date_file 10.23.52.53:$liffe_ref_date_file

#sync to ny servers
scp $liffe_ref_date_file 10.23.74.51:$liffe_ref_date_file
scp $liffe_ref_date_file 10.23.74.53:$liffe_ref_date_file
scp $liffe_ref_date_file 10.23.74.54:$liffe_ref_date_file
scp $liffe_ref_date_file 10.23.74.55:$liffe_ref_date_file

#sync to all other machines
ssh dvcinfra@10.23.74.51 "/home/dvcinfra/infracore/scripts/sync_file_to_all_machines.pl $liffe_ref_date_file"

