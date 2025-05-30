#!/bin/bash

print_msg_and_exit () {

  echo $* ;
  exit ; 

}

init () {

  [ $# -eq 1 ] || print_msg_and_exit "Usage : < Segment >" ; 

  if [ "CD" != "$1" ] && [ "FO" != "$1" ] && [ "CM" != "$1" ] ; then 
    print_msg_and_exit "Invalid Segment : $1" ; 
  fi 

  LOCKFILE=/home/dvcinfra/$1".lockfile" ; 
  [ ! -f $LOCKFILE ] || print_msg_and_exit "ANOTHER INSTANCE OF THE PROCESS IS ALREADY RUNNING FOR THIS SEGMENT" ; 
  touch $LOCKFILE ; 

  SEGMENT=$1 ; 
  DATAFILE="INVALID" ; 
  DATADIR="/home/dvcinfra/INVALID" ;

  if [ "CD" == "$1" ] ; then 
    DATAFILE=cd_primary ; 
    DATADIR=/home/dvcinfra/CDTBTData ;

    if [ `ps -ef |grep nse_tbt_cd_primary | grep -v grep  | wc -l` -gt 0 ] ; then 
      print_msg_and_exit "THE RAW DATA LOGGER PROCESS IS RUNNING, IT'S NOT SAFE TO RUN CONVERSION, EXISTING..." ;  
    fi

  fi 

  if [ "FO" == "$1" ] ; then 
    DATAFILE=fo_primary ; 
    DATADIR=/home/dvcinfra/TBTData ;

    if [ `ps -ef |grep nse_tbt_fo_primary | grep -v grep  | wc -l` -gt 0 ] ; then 
      print_msg_and_exit "THE RAW DATA LOGGER PROCESS IS RUNNING, IT'S NOT SAFE TO RUN CONVERSION, EXISTING..." ;  
    fi

  fi 

  if [ "CM" == "$1" ] ; then 
    DATAFILE=cm_primary ; 
    DATADIR=/home/dvcinfra/CashTBTData ;

    if [ `ps -ef |grep nse_tbt_cm_primary | grep -v grep  | wc -l` -gt 0 ] ; then 
      print_msg_and_exit "THE RAW DATA LOGGER PROCESS IS RUNNING, IT'S NOT SAFE TO RUN CONVERSION, EXISTING..." ;  
    fi

  fi 

}

#Check Available Space, If it's under 90GB, Compress The Raw Files First 
Check_Available_Space () {

 Min_Space_Required=90 ; 

 Available_Space=`df -h /home/dvcinfra/ | tail -1 | awk '{print $4}' | sed 's/G//g'` ; 
 
 if [ $Available_Space -lt $Min_Space_Required ] ; then 
   #By Now Any FO Flies Can Be Compressed
   gzip /spare/local/RawData/*$DATAFILE* ;

 fi

 Available_Space=`df -h /home/dvcinfra/ | tail -1 | awk '{print $4}' | sed 's/G//g'` ; 

 if [ $Available_Space -lt $Min_Space_Required ] ; then 
   TODAY=`date +"%Y%m%d"` ;
   echo "THERE IS INSUFFICIENT SPACE TO CONVERT DATA ON `hostname`" | /bin/mail -s "NSE_$SEGMENT'_DATA_CONVERSION_COPY_FAILED_FOR ->' $TODAY" ravi.parikh@tworoads.co.in nseall@tworoads.co.in
   exit ; 
 fi

}

ConvertData () {

 if [ -f /home/dvcinfra/$SEGMENT"_CONVERSION_STATUS.dat" ] ; then 

   #What if we already had done the conversion and script exited 
   if [ "DONE" == `cat /home/dvcinfra/$SEGMENT"_CONVERSION_STATUS.dat"` ] ; then 
     return ; 
   fi 

 fi 

 rm -rf $DATADIR ; 

 TODAY=`date +"%Y%m%d"` ; 
 CONVERSION_EXEC=LiveExec/bin/NSE_CONVERT_$SEGMENT"_DATA" ; 
 INPUT_FILE=`ls /spare/local/RawData/*$DATAFILE*` ;

 `$CONVERSION_EXEC $INPUT_FILE $TODAY >$TODAY"_"$SEGMENT".cout" 2>$TODAY"_"$SEGMENT".cerr"` ; 

  #Mark Status 
  echo "DONE" > /home/dvcinfra/$SEGMENT"_CONVERSION_STATUS.dat" ; 
  gzip `find $DATADIR -type f` ; 

}

SyncFurtherInBackGround () {

  #We are done here 
  ssh -n -f 10.23.5.161 "/home/dvcinfra/LiveScripts/Upload$SEGMENT'Data.sh' &" & 

}

SyncData () {

  local_file_count=`find $DATADIR -type f | wc -l` ; 
  remote_sync_threshold=`echo $local_file_count | awk '{printf "%d", $1*0.3}'` ; 
  remote_file_count=0 ; 

  #Let's first notify remote about number of files  
  ssh dvcinfra@10.23.5.161 "echo $local_file_count > /home/dvcinfra/$TODAY'_'$SEGMENT'.db'" ; 

  while [ $remote_file_count -lt $local_file_count ] ; do

    rsync -avz --progress $DATADIR 10.23.5.161:/home/dvcinfra >>$SEGMENT"_"$TODAY"_sync_status.txt" ; 
    remote_file_count=`ssh 10.23.5.161 "find $DATADIR -type f | wc -l"` ; 

    echo "RSYNC ROUND COMPLETED AT : " `date` " WITH REMOTE_FILE_COUNT -> $remote_file_count, AGAINST EXPECTED LOCAL COUNT -> $local_file_count" ; 

    # Now Start Remote Sync 
    if [ $remote_file_count -ge $remote_sync_threshold ] ; then 
      echo "INITIATED REMOTE SYNC AS CURRENT REMOTE COUNT IS : " $remote_file_count " THRESHOLD : " $remote_sync_threshold " TOTAL : " $local_file_count ; 
      SyncFurtherInBackGround ; 
    fi 

  done 

}

CleanUp () { 

 echo "DONE WITH CONVERSION AND COPY, CLEANING UP..." ;

 #Let's Compress Data 
 gzip /spare/local/RawData/*$DATAFILE* ;

 #Move It To OldData 
 mv /spare/local/RawData/*$DATAFILE* /spare/local/RawData/OldData/ ; 

 #Remove TBTData
 rm -rf $DATADIR ;

 #Remove Status File 
 rm -rf /home/dvcinfra/$SEGMENT"_CONVERSION_STATUS.dat" ; 

 rm -rf $LOCKFILE ; 

}

init $* ;
Check_Available_Space ; 
ConvertData ; 
SyncData ; 
CleanUp ; 
