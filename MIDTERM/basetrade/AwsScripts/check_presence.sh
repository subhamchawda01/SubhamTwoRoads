#!/bin/bash
#Checks for presence of file provided as argument (/NAS1/data/..)
file_path=$1
file_found=0
log_file=$HOME/.ec2_cache_log
echo $file_path >> $log_file
for num in {0..3}
  do
    full_path="/media/ephemeral"$num"/s3_cache/"$file_path
    if [ -f $full_path ]; then
      echo $full_path
      echo $full_path >> $log_file
      file_found=1
      break
    fi
done

if [ $file_found -eq 0 ]; then
  echo "0"
  echo "0" >> $log_file
fi
