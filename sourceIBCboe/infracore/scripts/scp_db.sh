#!/bin/bash

# THIS VERSION WILL SAVE THE MYSQL DUMP TO THE LOCATION OF THE SCRIPT RUNNING IT

file_name=`date +%Y%m%d`

#Create Local MySQL Dump
script_dir="/media/ephemeral16/db_backup/regular/"
#scp -r /media/ephemeral16/db_backup/adhoc/20170321/ dvcinfra@10.23.74.41:/spare/db_backups/results_db/adhoc/ 
if scp -r 10.0.0.31:${script_dir}${file_name} dvcinfra@10.23.74.41:/spare/db_backups/results_db/regular/
then
echo "File copied to remote server successfully"
 fi
