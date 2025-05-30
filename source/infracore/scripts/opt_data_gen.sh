#!/bin/bash

tradeinfo_dir='/spare/local/tradeinfo/NSE_Files/'
datasource_exchsymbol_file="/spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt"
log_dir="/NAS4/hardik/"
option_log_dir="${log_dir}option_log/"
bar_data_dir="${log_dir}BarData_IDX/"
shc_exec="${log_dir}get_shortcode_for_symbol_from_file"
bar_data_exec="${log_dir}nse_historical_bar_data_generator_option"
#bar_data_exec="${log_dir}nse_historical_bar_data_generator_option_new"
bar_data_exec="/home/dvctrader/stable_exec/nse_historical_bar_data_generator_20210929"

print_msg_and_exit() {
  echo $* ;
  exit ;
}

[ $# -eq 3 ] || print_msg_and_exit "Usage : < script > < CD/FO/SPOT > < PROD[NIFTY50]> <DATE>"


is_cd=$1
#prod_list=$2
prod=$2
dt=$3

ref_data_file="/spare/local/tradeinfo/NSE_Files/RefData/nse_cd_${dt}_contracts.txt"

is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $dt T`


  >${option_log_dir}/exch_sym_${prod}_${dt}

START_TIME="IST_915"
END_TIME="IST_1530"

start_=$(date -d "$dt 0345" +%s)
end_=$(date -d "$dt 1000" +%s)

if [ $is_cd = "SPOT" ];
then

  while [ $is_holiday_ = "1" ];
  do
    echo "holiday $dt"
    exit
  done

  bar_data_dir="/spare/local/BarData_SPOT/"

  lineNum=$(awk -v start_="$start_" '$1>=start_{print NR;exit}' ${bar_data_dir}${prod})
  echo "Line_num: $lineNum"
  if [ -z "$lineNum" ]
  then
        echo "no corrupt data for $prod"
  else
        sed -i "$lineNum,\$d" "${bar_data_dir}${prod}"
  fi


  echo "SPOT ${bar_data_dir}"
  echo "${bar_data_exec} ${dt} ${shc_file} ${START_TIME} ${END_TIME} 1 ${bar_data_dir}  "
  ${bar_data_exec} ${dt} <(echo "NSE_${prod}") ${START_TIME} ${END_TIME} 1 ${bar_data_dir}  

  echo "${prod} DONE"
  exit
  #${bar_data_exec} ${dt} ${shc_file} ${START_TIME} ${END_TIME} 1 ${bar_data_dir}  

  exit
elif [ $is_cd = "CD" ];
then 
  echo "CD"
  exit
  is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE_CD $dt T`
  while [ $is_holiday_ = "1" ];
  do
    echo "holiday $dt"
    exit
  done

  bar_data_dir="${log_dir}BarData_FOREX/"
  end_=$(date -d "$dt 1130" +%s)

  lineNum=$(awk -v start_="$start_" '$1>=start_{print NR;exit}' ${bar_data_dir}${prod})
  if [ -z "$lineNum" ]
  then
        echo "no corrupt data for $prod"
  else
        sed -i "$lineNum,\$d" "${bar_data_dir}${prod}"
  fi

  YYYY=${dt:0:4}
  YY=${dt:2:2}

  echo "CD Option Bar data $dt $YYYY $YY"
  END_TIME="IST_1700" #"IST_1530"


#  for prod in `cat $prod_list`;
#  do 
    exp_dt=`grep $prod $ref_data_file | grep FUT | awk '{print $8}' | grep "${prod}${yy}" | sort -u | awk '{print substr($1,7,2), substr($1,9,3)}' | grep "^${YY}" | awk -v yr=${YYYY} '{if($2=="JAN"){ mnt="01"; } \
                  else if($2 == "FEB") {mnt="02"; } \
                  else if($2 == "MAR") {mnt="03"; } \
                  else if($2 == "APR") {mnt="04"; } \
                  else if($2 == "MAY") {mnt="05"; } \
                  else if($2 == "JUN") {mnt="06"; } \
                  else if($2 == "JUL") {mnt="07"; } \
                  else if($2 == "AUG") {mnt="08"; } \
                  else if($2 == "SEP") {mnt="09"; } \
                  else if($2 == "OCT") {mnt="10"; } \
                  else if($2 == "NOV") {mnt="11"; } \
                  else if($2 == "DEC") {mnt="12"; } \
                  print $1,mnt \
    }' | sort -nk2 | head -1 | awk -v yr=${YYYY} '{print yr$2}'`

#
#
    echo "Exp: $prod $exp_dt"

    echo "egrep \"NSE_${prod}_PE_${exp_dt}|NSE_${prod}_CE_${exp_dt}\" ${datasource_exchsymbol_file} | awk '{print \$1}' >>${option_log_dir}/exch_sym_${prod}_${dt}"
    egrep "NSE_${prod}_PE_${exp_dt}|NSE_${prod}_CE_${exp_dt}" ${datasource_exchsymbol_file} | awk '{print $1}' >>${option_log_dir}/exch_sym_${prod}_${dt}
#  done

else
  echo "FO ELSE"
  exit
  #get curr exp date
  END_TIME="IST_1530"


  lineNum=$(awk -v start_="$start_" '$1>=start_{print NR;exit}' ${bar_data_dir}${prod})
  if [ -z "$lineNum" ]
  then
        echo "no corrupt data for $prod"
  else
        sed -i "$lineNum,\$d" "${bar_data_dir}${prod}"
  fi


  contract_file=${tradeinfo_dir}'/ContractFiles/nse_contracts.'${dt}
  exp_dt=`cat ${contract_file} | grep IDXFUT | grep BANKNIFTY | awk -v date=${dt} '{if($NF>=date)print $NF'} | sort | uniq | head -n1 | tail -1`
  echo "Exp: $exp_dt"
  >${option_log_dir}/exch_sym_${prod}_${dt}

#  for prod in `cat $prod_list`;
#  do 
    echo "egrep \"NSE_${prod}_PE_${exp_dt}|NSE_${prod}_CE_${exp_dt}\" ${datasource_exchsymbol_file} | awk '{print \$1}' >>${option_log_dir}/exch_sym_${prod}_${dt}"
    egrep "NSE_${prod}_PE_${exp_dt}|NSE_${prod}_CE_${exp_dt}" ${datasource_exchsymbol_file} | awk '{print $1}' >>${option_log_dir}/exch_sym_${prod}_${prod}_${dt}
#  done

fi

#exit
#for prod in `cat $prod_list`;
#do 
#  echo "egrep \"NSE_${prod}_PE_${exp_dt}|NSE_${prod}_CE_${exp_dt}\" ${datasource_exchsymbol_file} | awk '{print \$1}' >>${option_log_dir}/exch_sym_${prod}_${dt}"
#  egrep "NSE_${prod}_PE_${exp_dt}|NSE_${prod}_CE_${exp_dt}" ${datasource_exchsymbol_file} | awk '{print $1}' >>${option_log_dir}/exch_sym_${prod}_${dt}
#done

if [ `wc -l ${option_log_dir}/exch_sym_${prod}_${dt} | awk '{print $1}'` = "0" ];
then
  echo "EXIT $dt"
  exit
fi

shc_file="${option_log_dir}/shc_${prod}_${dt}"
echo "${shc_exec} ${option_log_dir}/exch_sym_${prod}_${dt} ${dt}"
${shc_exec} ${option_log_dir}/exch_sym_${prod}_${dt} ${dt} | awk '{print $2}' > ${shc_file}

wc_=`wc $shc_file`
echo "shc_count: $wc_"

START_TIME="IST_915"
echo "${bar_data_exec} ${dt} ${shc_file} ${START_TIME} ${END_TIME} 1 ${bar_data_dir}  "
${bar_data_exec} ${dt} ${shc_file} ${START_TIME} ${END_TIME} 1 ${bar_data_dir}  

echo "sort -u ${bar_data_dir}${prod} | sort -nk1 >${option_log_dir}/${prod} ; mv ${option_log_dir}/${prod} ${bar_data_dir}${prod}"
sort -u ${bar_data_dir}${prod} | sort -nk1 >${option_log_dir}/${prod} ; mv ${option_log_dir}/${prod} ${bar_data_dir}${prod}

echo "${prod} DONE"
