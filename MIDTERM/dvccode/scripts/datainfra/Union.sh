#!/bin/bash

### ABOUT

# given location date_for
if [ -z "$2" ]; then 
   echo usage: $0 "startdate enddate"
   echo usage: $0 "YYYYMMDD1 YYYYMMDD2"
   exit 0
fi
startdate=$1
enddate=$2



function exec_mysql(){
res=`mysql --host=10.23.74.51 --user=root res_gen << EO
$1;
EO`
echo -e "$res"
}


function chk_size(){
ip1=$1
path1=$2
ip3=$3
path2=$4

size1=`ssh -q $ip1 'du -b $path1'`
size1=`ssh -q $ip2 'du -b $path2'`

if [ $size1 -ne $size2 ]; then 
addFileEntry "$path1 $path2 SIZE_FAIL"
fi

}


function chk_sha1(){
ip1=$1
path_sha1=$2
ip3=$3
path_sha2=$4

sha11=`ssh -q $ip1 'sha1sum -b $path_sha1'`
sha12=`ssh -q $ip2 'sha1sum -b $path_sha2'`

if [ "$sha11"=="$sha12" ]; then 
addFileEntry "$path_sha1 $path_sha2 SHA_FAIL"
fi

}




function getHS1Path(){
#file="/NAS1/data/"$EXCHANGE"LoggedData/"$CURRENT_LOCATION/$dest_path
#file="/NAS1/data/"$EXCHANGE"LoggedData/"$CURRENT_LOCATION/$dest_path/$1
#  FILENAME="data/ORSData/$CURRENT_LOCATION/$dest_path/$file";

   #Upload to HS1 server (EC2 file host) as well
   file=$1
    hs1_disk=$(ssh -q -n $NAS_IP 'get_hs1_path  '$file'' </dev/null)
#hs1_disk=`$HOME/get_hs1_path $file`; #returns /media/ephemeral?
   path="$hs1_disk/s3_cache$file" ;
   echo $path; 
 
}


function getS3Path(){
   file="/NAS1/"$FILENAME ;

   s3_path="s3://s3dvc"$file ;
   echo $s3_path
}

function addFileEntry(){
echo ""
echo $1 >> $entryFile

}

function addLogEntry(){
  echo "" >> $logFile
  echo $1 >> $logFile



}

#///home/dvctrader/s3cmd-1.5.0-rc1/s3cmd
function union(){
loc_lines=`exec_mysql "select DISTINCT(location) from location_info"|tail -n +2`
echo $loc_lines
while read loc;
do
i=1;
date1="$startdate"
echo $date1
entryFile="$loc"_"$startdate"_"$enddate"
logFile=$loc"_"$startdate"_"$enddate

touch $logFile
addLogEntry "---------------"
while [ $date1 -ge $enddate ]; 
do
    date1=`date +%Y%m%d -d "${startdate}-${i} days"`
      #  echo $date1 # Use this however you want!
      check_files_NASHS1 $loc $date1
    ((i++))
done
done <<<"$loc_lines"
}




aws_gateway="10.0.1.77"
NAS_IP="10.23.74.41"

#check NAS vs ep file count for particular date is okay
function check_files_NASHS1(){
HS1="10.0.0.31"
#ls /media/ephemeral*/s3_cache/NAS1/data/CMELoggedData/CHI/2017/01/23/|grep 20170123|wc -l
loc=$1
date_for=$2
echo $date_for
conv_date=`date -d $date_for +%Y/%m/%d`
lines=`exec_mysql "select * from location_info where location='$1'"|tail -n +2`

#`$COPY_OVER_NON_COMBINED_SCRIPT /spare/local/MDSlogs/NonCombined/$EXCHANGE /spare/local/MDSlogs/GENERIC /spare/local/MDSlogs/$EXCHANGE $YYYYMMDD;
#Copy ORS binary and MDS logs
while read line;
do
  MESS=""
  EXCHANGE=`echo $line | awk '{print $3}'`
  NAS_FOLDER=`echo $line | awk '{print $4}'`
  IP=`echo $line | awk '{print $5}'`
  LOC_ID=`echo $line | awk '{print $1}'`
  echo "status for '$loc'_EXCHANGE '$EXCHANGE'_NAS_FOLDER '$NAS_FOLDER'_ date '$date_for 'SERVER''_'$IP'_'' HS1' $HS1"
  
  if [ "$NAS_FOLDER" == "ORS" ]; then
#/apps/data/ORSData/TOR/2017/01/
  pat1="/NAS1/data/ORSData/$loc/$conv_date"
  pat2="/media/ephemeral*/s3_cache/NAS1/data/ORSData/$loc/$conv_date"
 
  count_nas=$(ssh -q $NAS_IP 'ls  '$pat1'|grep gz|grep '$date_for'| wc -l' </dev/null)
  list_nas=$(ssh -q $NAS_IP 'ls  '$pat1'|grep gz|grep '$date_for'' </dev/null)
 # count_hs1=$(ssh -q dvctrader@$aws_gateway 'ssh -q -n '$HS1' "ls  '$pat2'|grep gz|grep '$date_for'| wc -l"' </dev/null)
 count_hs1=$(ssh -q -n $HS1 'ls  '$pat2'|grep gz|grep '$date_for'| wc -l' </dev/null)
#  list_hs1=$(ssh -q dvctrader@$aws_gateway 'ssh -q -n '$HS1' "ls  '$pat2'|grep gz|grep '$date_for'| wc -l"' </dev/null)
  MESS= "$MESS"
  if [ $co1 -eq $co2 ]; then 
  MESS= "$MESS COUNT_PASS"
  else
  COUNT="$(echo "$co1- $co2" | bc)"
  MESS= "$MESS COUNT_FAIL=$COUNT"
  
  fi
  
  else
   pat1="/NAS1/data/"$NAS_FOLDER"LoggedData/"$loc"/"$conv_date
  pat2="/media/ephemeral*/s3_cache/NAS1/data/ORSData/$loc/$conv_date"
  count_nas=$(ssh -q $NAS_IP 'ls  '$pat1'|grep gz|grep '$date_for'| wc -l' </dev/null)
  list_nas=$(ssh -q $NAS_IP 'ls  '$pat1'|grep gz|grep '$date_for'| wc -l' </dev/null)
  count_hs1=$(ssh -q -n $HS1 'ls  '$pat2'|grep gz|grep '$date_for'| wc -l' </dev/null)
  #list_hs1=$(ssh -q dvctrader@$aws_gateway 'ssh -q -n '$HS1' "ls  '$pat2'|grep gz|grep '$date_for'| wc -l"' </dev/null)
  if [ $co1 -eq $co2 ]; then 
  MESS="$MESS $COUNT_PASS"
  else
  COUNT="$(echo "$co1- $co2" | bc)"
  MESS="$MESS COUNT_FAIL=$COUNT"
  
  fi
  
  fi
  addLogEntry $MESS
   for product in $list_nas;do
     pat_hs1=`getHS1Path "$pat1/$product"`
          chk_size $NAS_IP $pat1 $HS1 $pat_hs1
          chk_sha1 $NAS_IP $pat1 $HS1 $pat_hs1
        done

done <<<"$lines"


}

union



