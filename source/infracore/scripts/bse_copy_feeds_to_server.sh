#!/bin/bash

cd /media/ephemeral17/tmp_data_files/BSEPFLoggedData
for file in `find -type f`
do
   gzip $file
   name=${file##*/}
   date=$(echo $name | grep -oP '[\d]{8}' | tail -1 )
   YYYY=${date:0:4}
   MM=${date:4:2}
   DD=${date:6:2}

   FILENAME="BSE/"$YYYY"/"$MM"/"$DD"/"$name".gz";
   /home/dvctrader/s3cmd-1.5.0-rc1/s3cmd put "/media/ephemeral17/tmp_data_files/BSEPFLoggedData/"$FILENAME s3://s3dvc/NAS1/data/BSELoggedData/$FILENAME; #Upload to S3

   #Upload to HS1 server (EC2 file host) as well
   file="/NAS1/data/BSELoggedData/"$FILENAME ;
   hs1_disk=`$HOME/get_hs1_path $file`; #returns /media/ephemeral?
   path="$hs1_disk/s3_cache$file" ;
   enclosing_folder=`dirname $path` ;
   path_temp=$path ;
   path_temp+="_temp" ;
   s3_path="s3://s3dvc"$file ;
   ssh dvctrader@52.0.55.252 << FILE_UPDATE_SCRIPT
      mkdir -p $enclosing_folder ;
      /home/dvctrader/s3cmd-1.5.0-rc1/s3cmd get --skip-existing --no-progress $s3_path $path_temp ;
      mv $path_temp $path ;
      echo $path;
FILE_UPDATE_SCRIPT
done

cd /media/ephemeral17/tmp_data_files/BSEOFLoggedData
for file in `find -type f`
do
   gzip $file
   name=${file##*/}
   date=$(echo $name | grep -oP '[\d]{8}' | tail -1 )
   YYYY=${date:0:4}
   MM=${date:4:2}
   DD=${date:6:2}

   FILENAME="BSEOFLoggedData/BSE/"$YYYY"/"$MM"/"$DD"/"$name".gz";
   /home/dvctrader/s3cmd-1.5.0-rc1/s3cmd put "/media/ephemeral17/tmp_data_files/"$FILENAME s3://s3dvc/NAS1/data/$FILENAME; #Upload to S3

   #Upload to HS1 server (EC2 file host) as well
   file="/NAS1/data/"$FILENAME ;
   hs1_disk=`$HOME/get_hs1_path $file`; #returns /media/ephemeral?
   path="$hs1_disk/s3_cache$file" ;
   enclosing_folder=`dirname $path` ;
   path_temp=$path ;
   path_temp+="_temp" ;
   s3_path="s3://s3dvc"$file ;
   ssh dvctrader@52.0.55.252 << FILE_UPDATE_SCRIPT
      mkdir -p $enclosing_folder ;
      /home/dvctrader/s3cmd-1.5.0-rc1/s3cmd get --skip-existing --no-progress $s3_path $path_temp ;
      mv $path_temp $path ;
      echo $path;
FILE_UPDATE_SCRIPT
done

