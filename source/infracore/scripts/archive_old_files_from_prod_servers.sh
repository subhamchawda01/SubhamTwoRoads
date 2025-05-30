#!/bin/bash
#This script archives all the old files on prod servers to NAS

delete_threshold=4320 #archive files which are older than this (in mins)
time_now=`date +%s`
today_=`date +%Y%m%d`
file_list=/tmp/files_$time_now
NAS_IP=10.23.74.40
remote_user=dvcinfra
SERVER=`hostname`
temp_archive=$SERVER.$time_now.old_files.tar.gz
remote_location=/apps/old_logs/$SERVER

#Copy to NAS and delete from prod server
#Supposed to do this for a list of directories (old files in these dirs will be removed from this server)
for bkp_path in /spare/local/MDSlogs/;
do
  base_name=`basename $bkp_path`
  remote_folder=$remote_location/$base_name
  cd $bkp_path;
  #Prepare the list of files to be archived
  find ./ -type f -mmin +$delete_threshold > $file_list
  bkp_files=`cat $file_list | tr '\n' ' '`

  #Archive all old files
  #rm $temp_archive
  tar -zcvf $temp_archive $bkp_files

  #Previous step failed => do not proceed ahead
  if [ $? -ne 0 ]; then
    echo "Archiving files failed"
    rm $temp_archive
    continue
  fi

  #Send the archived tar file to NAS
  rsync -raz $temp_archive $remote_user@$NAS_IP:
  
  #Previous step failed => do not proceed ahead
  if [ $? -ne 0 ]; then
    echo "Could not send archived file to NAS"
    rm $temp_archive
    continue
  fi
  
  #ssh to NAS server and unpack the archive to appropriate location
  ssh $remote_user@$NAS_IP << UNPACK_SCRIPT

    #Create directory if doesn't exist, and move there
    mkdir -p $remote_folder
    cd $remote_folder

    #Simply untar the archive (since we are already in the right directory)
    tar -zxvf /home/$remote_user/$temp_archive

    #Remove the tar file (only if it was extracted successfully
    if [ \$? -eq 0 ]; then
      rm /home/$remote_user/$temp_archive
    fi

    #clean old files
    find ./ -mtime +10 -type f -exec rm {} \;

UNPACK_SCRIPT
  
  #Remove the archived files in case the transfer was successful
  for file in `cat $file_list | tr '\n' ' '`;
  do
    echo "Removing file $file"
    rm $file
  done

  #Clean up temp files
  rm $file_list
  rm $temp_archive

done


#Copy to NAS (don't delete from prod server)
#Supposed to do this for a list of directories (new files in these dirs will be copied from this server to NAS and files older than a week there will be removed)
for bkp_path in /spare/local/logs/ /spare/local/files/ /spare/local/ORSlogs/;
do
  base_name=`basename $bkp_path`
  remote_folder=$remote_location/$today_/$base_name/

  #ssh to NAS server and unpack the archive to appropriate location
  ssh $remote_user@$NAS_IP << CLEANER_SCRIPT

    #Create directory if doesn't exist, and move there
    mkdir -p $remote_folder
    cd $remote_location

    #Remove directories older than 7 days
    find ./ -name "*201*" -maxdepth 1 -mtime +3 -type d -exec rm -r {} \;

CLEANER_SCRIPT
  
  #After cleaning old ones, save new version
  cd $bkp_path;
  #Rsync files to today's folder on NAS
  rsync -raz ./* $remote_user@$NAS_IP:$remote_folder

done

#These directories are useless => remove their files without any backup
for folder in `cat /spare/local/files/folders_to_clean.txt`;
do
  cd $folder
  rm ./*
done
