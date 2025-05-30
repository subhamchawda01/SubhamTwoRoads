#!/bin/bash

while [ true ] ; do

#At worst ensure, we never delete anything else
    cd /spare/local/MDSlogs ;

    #unconditionally compress generic files to save on space
    if [ `find /spare/local/MDSlogs/GENERIC -type f -mtime +2 | grep -v ".gz" | wc -l` -gt 0 ] ; then 
      gzip -f `find /spare/local/MDSlogs/GENERIC -type f -mtime +2 | grep -v ".gz"`
    fi 

#Check spare space
    spare_space=`df -h /spare | grep -v "Filesystem"  | awk '{print $5}' | sed 's/%//g'`

#possibly we have a lot of files and hence safer to iterate over each one 
    if [ $spare_space -gt 25 ] ; then  #let's try and remove things which are older than 5 days
    for dirlist in `find /spare/local/MDSlogs -maxdepth 1 -type d | sort -r | grep -v "zabbix"`
    do
       find $dirlist -type f -mtime +10 -exec rm -f {} \;
    done 
    fi 

    spare_space=`df -h /spare | grep -v "Filesystem"  | awk '{print $5}' | sed 's/%//g'`

    if [ $spare_space -gt 40 ] ; then  #let's try and remove things which are older than 5 days
    for dirlist in `find /spare/local/MDSlogs -maxdepth 1 -type d | sort -r | grep -v "zabbix"`
    do
       find $dirlist -type f -mtime +7 -exec rm -f {} \;
    done 
    fi 

    spare_space=`df -h /spare | grep -v "Filesystem"  | awk '{print $5}' | sed 's/%//g'`

    if [ $spare_space -gt 60 ] ; then  #let's try and remove things which are older than 5 days
    for dirlist in `find /spare/local/MDSlogs -maxdepth 1 -type d | sort -r | grep -v "zabbix"`
    do
       find $dirlist -type f -mtime +5 -exec rm -f {} \;
    done 
    fi 

    spare_space=`df -h /spare | grep -v "Filesystem"  | awk '{print $5}' | sed 's/%//g'`

    if [ $spare_space -gt 75 ] ; then 
     for dirlist in `find /spare/local/MDSlogs -maxdepth 1 -type d | sort -r | grep -v "zabbix"`
     do
       find $dirlist -type f -mtime +3 -exec rm -f {} \;
     done 
    fi 

    spare_space=`df -h /spare | grep -v "Filesystem"  | awk '{print $5}' | sed 's/%//g'`

    if [ $spare_space -gt 85 ] ; then 
     for dirlist in `find /spare/local/MDSlogs -maxdepth 1 -type d | sort -r | grep -v "zabbix"`
     do
       find $dirlist -type f -mtime +2 -exec rm -f {} \;
     done 
    fi 

    spare_space=`df -h /spare | grep -v "Filesystem"  | awk '{print $5}' | sed 's/%//g'`

    if [ $spare_space -gt 90 ] ; then 
     for dirlist in `find /spare/local/MDSlogs -maxdepth 1 -type d | sort -r | grep -v "zabbix"`
     do
       find $dirlist -type f -mtime +1 -exec rm -f {} \;
     done 
    fi 

  sleep 600 ;

done 
