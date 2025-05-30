#!/bin/bash

cd /media/ephemeral17/tmp_data_files/Again/SGXPFLoggedData
for file in `find -type f`
do
   gzip $file
   name=${file##*/}
   date=$(echo $name | grep -oP '[\d]{8}')
   YYYY=${date:0:4}
   MM=${date:4:2}
   DD=${date:6:2}

   FILENAME="SPR/"$YYYY"/"$MM"/"$DD"/"$name".gz";
   /home/dvctrader/s3cmd-1.5.0-rc1/s3cmd put "/media/ephemeral17/tmp_data_files/Again/SGXPFLoggedData/"$FILENAME s3://s3dvc/NAS1/data/SGXLoggedData/$FILENAME; #Upload to S3

   #Upload to HS1 server (EC2 file host) as well
   file="/NAS1/data/SGXLoggedData/"$FILENAME ;
   hs1_disk=`$HOME/get_hs1_path $file`; #returns /media/ephemeral?
   path="$hs1_disk/s3_cache$file" ;

   cp "/media/ephemeral17/tmp_data_files/Again/SGXPFLoggedData/"$FILENAME $path ;  
   echo "Copy /media/ephemeral17/tmp_data_files/Again/SGXPFLoggedData/$FILENAME to $path" ; 
done

cd /media/ephemeral17/tmp_data_files/Again/SGXOFLoggedData
for file in `find -type f`
do
   gzip $file
   name=${file##*/}
   date=$(echo $name | grep -oP '[\d]{8}')
   YYYY=${date:0:4}
   MM=${date:4:2}
   DD=${date:6:2}

   FILENAME="SGXOFLoggedData/SPR/"$YYYY"/"$MM"/"$DD"/"$name".gz";
   /home/dvctrader/s3cmd-1.5.0-rc1/s3cmd put "/media/ephemeral17/tmp_data_files/Again/"$FILENAME s3://s3dvc/NAS1/data/$FILENAME; #Upload to S3

   #Upload to HS1 server (EC2 file host) as well
   file="/NAS1/data/"$FILENAME ;
   hs1_disk=`$HOME/get_hs1_path $file`; #returns /media/ephemeral?
   path="$hs1_disk/s3_cache$file" ;
   
   cp "/media/ephemeral17/tmp_data_files/Again/"$FILENAME $path ; 
   echo "Copy /media/ephemeral17/tmp_data_files/Again/$FILENAME to $path" ; 

done

echo "Finished" ; 
