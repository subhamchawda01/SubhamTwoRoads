cd /media/ephemeral17/tmp_data_files/NSEL1/NSE/2016
for file in `find -type f`
do
   gzip $file
   name=${file##*/}
   date=$(echo $name | grep -oP '[\d]{8}' | tail -1)
   YYYY=${date:0:4}
   MM=${date:4:2}
   DD=${date:6:2}

   FILENAME=$MM"/"$DD"/"$name".gz";
   DIRNAME=$MM"/"$DD"/" ; 
   /home/dvctrader/s3cmd-1.5.0-rc1/s3cmd put "/media/ephemeral17/tmp_data_files/NSEL1/NSE/2016/"$FILENAME s3://s3dvc/NAS1/data/NSEL1LoggedData/NSE/2016/$FILENAME; #Upload to S3

   #Upload to HS1 server (EC2 file host) as well
   file="/NAS1/data/NSEL1LoggedData/NSE/2016/"$FILENAME ;
   dir="/NAS1/data/NSEL1LoggedData/NSE/2016/"$DIRNAME ;
   hs1_disk=`$HOME/get_hs1_path $file`; #returns /media/ephemeral?
   path="$hs1_disk/s3_cache$file" ;
   dirpath="$hs1_disk/s3_cache$dir"

   mkdir -p $dirpath ;

   cp "/media/ephemeral17/tmp_data_files/NSEL1/NSE/2016/"$FILENAME $path ;
   echo "Copy /media/ephemeral17/tmp_data_files/NSEL1/NSE/2016/$FILENAME to $path" ;
done

echo "Trigger Sync for more dates on dvctrader@10.0.1.77. See if you need to delete synced files." | /bin/mail -s "Sync Finished on dvctrader@10.0.1.77" -r "nse_mass_l1_data_generator" "pranjal.jain@tworoads.co.in , vedant@tworoads.co.in"
