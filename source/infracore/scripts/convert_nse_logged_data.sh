#!/bin/bash

print_msg_and_exit() {
  echo $*;
  exit;
}

check_time() {
  if [ `date "+%H"` -lt 10 ] ; then
    echo "CAN'T RUN THE SCRIPT FOR TODAY BEFORE 10 UTC, WE CAN BE RUNNING PRODUCTION AT THIS TIME..." ;
    exit ;
  fi

}

stop_nseshmwriter() {
  echo "FLUSHING MDS FILES....." ; 
  /home/pengine/prod/live_execs/combined_user_msg --dump_mds_files --only_ors_files 0 >/dev/null 2>&1
  sleep 5 ;
  echo "SHUTTING DOWN NSEShmWriter WRITER...." ;
  /home/pengine/prod/live_scripts/SmartDaemonController.sh NSEShmWriter STOP >/dev/null 2>&1
}

move_old_data() {
    mkdir -p  "/spare/local/MDSlogs/NSE_$date"
    if [ -z "$(ls -A ${converted_dest_dir})" ];
    then
        mv $converted_dest_dir /spare/local/MDSlogs/NSE_$date
    fi
    mkdir -p $converted_dest_dir 
}


move_data() {
  echo "CREATING DATA DIR ${data_dir}" 
  mkdir -p ${data_dir}
  mkdir -p ${data_to_convert_dir}
  cd ${generic_data_dir}	
  echo "MOVING DATA TO ${data_dir}"
  pwd
  mv `ls | grep NSE_ | grep $date` ${data_dir}
  cd ${data_dir};
  count=`ls | wc -l`
  echo "DATA COUNT NOW ${count}";
}

start_data_convertion() {
  data_type=$1; 
#make sure data to convert is empty
  cd ${data_to_convert_dir}
  mv * ${data_dir}
  cd ${data_dir};
  if [ $1 == "CASH" ];then
    mv `ls | egrep -v "_FUT|_CE_|_PE_"` ${data_to_convert_dir}
  elif [ $1 == "FUT" ];then
    mv `ls | grep _FUT_` ${data_to_convert_dir}
  elif [ $1 == "PE" ];then
    mv `ls | grep _PE_` ${data_to_convert_dir}
  elif [ $1 == "CE" ];then
    mv `ls | grep _CE_` ${data_to_convert_dir}
  else 
    echo "INVALID DATA TO CONVERT"; #No need to exit as the dir will have no data 
  fi

  cd ${data_to_convert_dir};
  count=`ls  | wc -l`;
  echo "STARTING $data_type DATA CONVESRIONS, COUNT : ${count}";
  /home/pengine/prod/live_scripts/convert_nse_generic_to_dotex_New.sh $date ${data_to_convert_dir} ${converted_dest_dir}
  cd ${converted_dest_dir};
  count=`ls | wc -l`
  echo "DONE $data_type DATA CONVESRIONS, COUNT : ${count}"; 
  mv * ${dest_data_dir}
  cd ${data_to_convert_dir}
  mv * ${data_dir}
  #now update the file count and status
  echo "$date $data_type $count completed" >>$data_copy_info_file
}

init_data_coversion() {
  [ $# -eq 1 ] || print_msg_and_exit "Usage : < script > < DATE >" ;
  date=$1;
  if [ $1 == "TODAY" ]; then
	date=`date +"%Y%m%d"`;
  fi

  running_status='/tmp/data_copy_file'  
  [ -f ${running_status} ] && print_msg_and_exit "CANNOT START ANOTHER INSTANCE FILE  EXISTS : ${running_status}" ;
  echo "Running Data copy convertion 1" >$running_status
  
  yyyy=${date:0:4}
  mm=${date:4:2}
  dd=${date:6:2}
  generic_data_dir="/spare/local/MDSlogs/GENERIC/"
  data_dir="/spare/local/MDSlogs/GENERIC/ALLDATA_${date}/"
  data_to_convert_dir="${data_dir}/DATA_TO_CONVERT/"
  converted_dest_dir="/spare/local/MDSlogs/NSE/"
  dest_data_dir="/spare/local/MDSlogs/${yyyy}/${mm}/${dd}/"
  data_copy_info_file="/spare/local/data_copy_update.txt"
  touch $data_copy_info_file
  mkdir -p ${dest_data_dir}
	
  #check time and stop only if we arent running for today"
  #since shmwriter will be stopped by eod and next day we 
  #can run any time for todays conversion"
  [ $date == `date +"%Y%m%d"` ] && check_time ;
  [ $date == `date +"%Y%m%d"` ] && stop_nseshmwriter ;
  
  #check and move older data
  move_old_data
  #move data to be converted to alldata
  move_data
  start_data_convertion FUT;
  start_data_convertion CASH;
  start_data_convertion PE;
  start_data_convertion CE;    
  rm -rf ${running_status}
}

init_data_coversion $*
