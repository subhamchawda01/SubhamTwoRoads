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

stop_bseshmwriter() {
  echo "FLUSHING MDS FILES....." ; 
  /home/pengine/prod/live_execs/combined_user_msg --dump_mds_files --only_ors_files 0 >/dev/null 2>&1
  sleep 5 ;
  echo "SHUTTING DOWN NSEShmWriter WRITER...." ;
  /home/pengine/prod/live_scripts/SmartDaemonController.sh BSEShmWriter STOP >/dev/null 2>&1
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
#some times mv fails if file are above 50k
  mv `ls | grep -v ORS_BSE | grep NSE_ | grep $date | grep B` ${data_dir}
  mv `ls | grep -v ORS_BSE | grep NSE_ | grep $date` ${data_dir}
  cd ${data_dir};
  count=`ls | wc -l`
  echo "DATA COUNT NOW ${count}";
}

start_data_convertion_space_opt() {
  data_type=$1;
#make sure data to convert is empty
#  mkdir -p ${data_alpha}
  echo ${data_alpha}
  echo "PWD:" `pwd`
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
  elif [ $1 == "ALL" ];then
        mv * ${data_to_convert_dir}
  else
    echo "INVALID DATA TO CONVERT"; #No need to exit as the dir will have no data 
  fi

  cd ${data_to_convert_dir};
  echo "PWD:" `pwd`
  mkdir ${data_alpha}
  echo "MAKING DIR: " ${data_alpha}
  mv `ls | egrep "NSE_[A-J a-j]."` ${data_alpha}
  cd ${data_alpha}
  count=`ls  | wc -l`;
  echo "STARTING $data_type DATA CONVESRIONS, COUNT : ${count}";
  /home/pengine/prod/live_scripts/convert_nse_generic_to_dotex_New.sh $date ${data_alpha} ${converted_dest_dir}
  cd ${converted_dest_dir};
  count1=`ls | wc -l`
  echo "DONE $data_type DATA CONVESRIONS, COUNT : ${count1}";
  mv * ${dest_data_dir}
  cd ${data_alpha}
  mv * ${data_to_convert_dir}
  cd ${data_to_convert_dir}
  mv `ls | egrep "NSE_[^A-J a-j]."` ${data_alpha}
  cd ${data_alpha}
  count=`ls  | wc -l`;
  echo "STARTING $data_type DATA CONVESRIONS, COUNT : ${count}";
  /home/pengine/prod/live_scripts/convert_nse_generic_to_dotex_New.sh $date ${data_alpha} ${converted_dest_dir}
  cd ${converted_dest_dir};
  count2=`ls | wc -l`
  echo "DONE $data_type DATA CONVESRIONS, COUNT : ${count2}";
  mv * ${dest_data_dir}
  cd ${data_alpha}
  mv * ${data_to_convert_dir}
  cd ${data_to_convert_dir}
  mv * ${data_dir}
  #now update the file count and status
  count_total=$((count1+count2))
  echo "$date $data_type $count_total completed" >>$data_copy_info_file
}



init_data_coversion() {
  [ $# -eq 1 ] || print_msg_and_exit "Usage : < script > < DATE >" ;
  date=$1;
  if [ $1 == "TODAY" ]; then
	date=`date +"%Y%m%d"`;
  fi
  is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date T`
  if [ $is_holiday = "1" ] ; then
       echo "NSE Holiday. Exiting...";
       exit;
  fi
  running_status='/tmp/data_copy_file'  
#  [ -f ${running_status} ] && print_msg_and_exit "CANNOT START ANOTHER INSTANCE FILE  EXISTS : ${running_status}" ;
  echo "Running Data copy convertion 1" >$running_status
  
  yyyy=${date:0:4}
  mm=${date:4:2}
  dd=${date:6:2}
  generic_data_dir="/spare/local/MDSlogs/GENERIC/"
  data_dir="/spare/local/MDSlogs/GENERIC/ALLDATA_${date}_NSE/"
  data_to_convert_dir="${data_dir}/DATA_TO_CONVERT/"
  data_alpha="${data_dir}/ALPHA_DATA"
  converted_dest_dir="/spare/local/MDSlogs/NSE/"
  dest_data_dir="/spare/local/MDSlogs/${yyyy}/${mm}/${dd}/"
  data_copy_info_file="/spare/local/data_copy_update.txt"
  local_Dir="/NAS1/data/BSELoggedData/BSE/${yyyy}/${mm}/${dd}/"
  touch $data_copy_info_file
  mkdir -p ${dest_data_dir}
  DEST_SERVER="10.23.5.62"
  #check time and stop only if we arent running for today"
  #since shmwriter will be stopped by eod and next day we 
  #can run any time for todays conversion"
  [ $date == `date +"%Y%m%d"` ] && check_time ;
  [ $date == `date +"%Y%m%d"` ] && stop_bseshmwriter ;
  
# move FUT DATA in GENERIC_NIFTY to  GENERIC in back
  cd /spare/local/MDSlogs/GENERIC_NIFTY
  mv * $generic_data_dir
  #check and move older data
  move_old_data
  #move data to be converted to alldata
  move_data
  echo "RUNNING CONVERTION OF DATA"
  start_data_convertion_space_opt ALL;

  ssh $DEST_SERVER "mkdir -p $local_Dir"
  echo "SYNCING  DATA  "
##  while true; do
##      conv_count=`ls $dest_data_dir | wc -l` ;
##      remote_count=`ssh $DEST_SERVER "ls $local_Dir | wc -l"` ;
##      [[ $conv_count -eq $remote_count ]] && break;
##     echo "COUNT $conv_count LOCAL: $remote_count "
##      rsync -avz --progress $dest_data_dir ${DEST_SERVER}:${local_Dir}
##      sleep 1m;
##  done
rsync -avz --progress $dest_data_dir ${DEST_SERVER}:${local_Dir}
  echo "GZIPING DATA  " 
  cd $data_dir
  gzip NSE_A* 
  gzip NSE_B*
  conv_count=`ls $dest_data_dir | wc -l` ;
  remote_count=`ssh $DEST_SERVER "ls $local_Dir | wc -l"` ;
  ls | grep -v NSE_A | grep -v NSE_B | xargs gzip
  echo "" | mailx -s "INDB12 $date GENERICCOUNT22: $conv_count LOCAL SERVER: $remote_count" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in

  rm -rf ${running_status}
}

init_data_coversion $*
