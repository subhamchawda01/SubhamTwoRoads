#!/bin/bash

#Runs on IND13 server

datacopy_ip=10.23.5.67

print_msg_and_exit () {
    echo $* ;
    exit ;
}

check_time () {

  if [ `date "+%H"` -lt 10 ] ; then
    echo "CAN'T RUN THE SCRIPT FOR TODAY BEFORE 10 UTC, WE CAN BE RUNNING PRODUCTION AT THIS TIME..." ;
    exit ;
  fi

}

stop_combinedwriter () {

  echo "FLUSHING MDS FILES....." ; 
  /home/pengine/prod/live_execs/combined_user_msg --dump_mds_files --only_ors_files 0 >/dev/null 2>&1
  sleep 5 ;
  echo "SHUTTING DOWN NSEShmWriter WRITER...." ;
  /home/pengine/prod/live_scripts/SmartDaemonController.sh NSEShmWriter STOP >/dev/null 2>&1

}

move_older_data () {

  mv $destdir $destdir"_OLD_"$dt ;
  ssh $datacopy_ip "mv $destdir $destdir'_OLD_$dt'" ;
  mkdir -p $destdir ;
  ssh $datacopy_ip "mkdir -p $destdir" ;

  remote_file_count=`ssh $datacopy_ip "ls $destdir/* 2>/dev/null| wc -l"` ;

  if [ $remote_file_count -ne 0 ] ; then 
    print_msg_and_exit "UNABLE TO CLEANUP REMOTE DIRECTORY" ;
  fi 

}

move_data () {

  generic_dir="/spare/local/MDSlogs/GENERIC" ;
  datadir="/spare/local/MDSlogs/GENERIC/ALLDATA_$dt" ;
  echo "CREATING DATADIR -> $datadir" ;
  mkdir -p $datadir ;

  cd $generic_dir; 
  mv *$dt* $datadir/ ;

  for files in `ls | grep "NSE"` ; do 
    mv $files $datadir/ ;
  done 

  count=`ls | grep $dt | wc -l` ;

  echo "MOVING DATA... GENERIC DATA COUNT NOW ->"$count ;

}


start_data_copy () {
  argv=$1
  cd $generic_dir ;
  count=`ls | wc -l` ;
  echo "DATA COUNT NOW AT GENERIC ->"$count ; 
  echo "INITIATING DATA CONVERTER..." ;
  /home/pengine/prod/live_scripts/convert_nse_generic_to_dotex.sh $dt ; 
  : '
    Now converted data is in destdir,
  '  
  if [ "$argv" == "CASH" ] || [ "$argv" == "FUT" ];then
    rsync -avz --progress $destdir $datacopy_ip":"$mdsdir
    echo "$1 DATA COPY DONE..." ;
    cd $destdir;
    mv * $destdatadir/ ;
    ssh $datacopy_ip "mv $destdir/* $remotedestdatadir/" ;
    ssh $datacopy_ip "rsync -avz --progress /NAS1/data/NSELoggedData/NSE/$yyyy/$mm/$dd dvctrader@3.89.148.73:/NAS1/data/NSELoggedData/NSE/$yyyy/$mm" ;
  fi

  cd $generic_dir ;
  mv *$dt* $datadir/ ; #move back data 
}

start_cashdata_copy (){
  echo "MOVING CASH DATA TO GENERIC... " ; 

  cd $datadir ;
  mv `ls | egrep -v "_CE_|_PE_|_FUT_"` $generic_dir/ ;
  cd $HOME ; 

  start_data_copy CASH;

}

start_futdata_copy (){
  echo "MOVING FUT DATA TO GENERIC... " ; 

  cd $datadir ;
  mv `ls | grep "_FUT_"` $generic_dir/ ;
  cd $HOME ; 

  start_data_copy FUT;

}

start_cedata_copy (){
  echo "MOVING CE OPTIONS DATA TO GENERIC... " ; 

  cd $datadir ;
  mv `ls | grep "_CE_"` $generic_dir/ ;
  cd $HOME ; 

  start_data_copy CE;

}

start_pedata_copy (){
  echo "MOVING PE OPTIONS DATA TO GENERIC... " ; 

  cd $datadir ;
  mv `ls | grep "_PE_"` $generic_dir/ ;
  cd $HOME ; 

  start_data_copy PE;

}

start_pe_and_ce_data_copy() {
  start_cedata_copy;
# commenting rsync now 
# rsync -avz --progress $destdir $datacopy_ip":"$mdsdir &
  PID=$!
  start_pedata_copy;
  wait $PID
#  rsync -avz --progress $destdir $datacopy_ip":"$mdsdir
  echo "PE AND CE DATA COPY DONE..." ;
  
  cd $destdir;
  mv * $destdatadir/ ;
# ssh $datacopy_ip "cd $destdir;mv * $remotedestdatadir/" ;
#  ssh $datacopy_ip "rsync -avz --progress /NAS1/data/NSELoggedData/NSE/$yyyy/$mm/$dd 3.89.148.73:/NAS1/data/NSELoggedData/NSE/$yyyy/$mm" ;

}

init_datacopy () {

  [ $# -eq 1 ] || print_msg_and_exit "Usage : < script > < DATE >" ;
 
  dt=$1 ;

  if [ "$dt" == "TODAY" ] ; then
    dt=`date +"%Y%m%d"` ;
  fi

  yyyy=${dt:0:4} ;
  mm=${dt:4:2};
  dd=${dt:6:2};

  mdsdir="/spare/local/MDSlogs" ;
  destdir="/spare/local/MDSlogs/NSE" ;
  destdatadir="/spare/local/MDSlogs/$yyyy/$mm/$dd" ;
  remotedestdatadir="/NAS1/data/NSELoggedData/NSE/$yyyy/$mm/$dd" ;

  mkdir -p $destdatadir ;
  ssh $datacopy_ip "mkdir -p $remotedestdatadir" ;

  check_time ;  
  stop_combinedwriter ;
  move_older_data;
  move_data ;
  start_cashdata_copy ;
  start_futdata_copy ;

  ssh $datacopy_ip "/home/dvcinfra/important/datacopy_complete.sh " & 

  start_pe_and_ce_data_copy ;
}

init_datacopy $* ;

