#!/bin/bash

Data_Dir="/NAS1/data/ORSData/NSE"
Mds_Log="/home/dvcinfra/mds_log_reader1"
work_dir='/home/dvcinfra/important/Generate_ors_detail/Product_Details/'
data_sym='/spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt'
sym_conv='/home/pengine/prod/live_execs/get_shortcode_from_ds'
sym_in_file="/tmp/sym_to_convert"
sym_out_file="/tmp/sym_converted"
mkdir -p $work_dir
#export LD_LIBRARY_PATH=/opt/glibc-2.14/lib ;/home/pengine/prod/live_execs/get_shortcode_from_ds 20191227 FO /tmp/fie /tmp/fil2

if [ $# -ne 1 ] ; then
  echo "Called As : " $* ;
  echo "$0 Date(20191226)" ;
  exit ;
fi

date_=$1
#date_=`date +\%Y\%m\%d`
YYYY=${date_:0:4}
MM=${date_:4:2}
DD=${date_:6:2}
OutFileCash="$work_dir/${date_}_cash"
OutFileFut="$work_dir/${date_}_fut"
OutFileOpt="$work_dir/${date_}_opt"

if [ ! -d "$Data_Dir/$YYYY/$MM/$DD" ]; then
  echo "Data Not copied from IND13";
  echo ""| mailx -s "ORS Data Not found to Gen Html" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in
  exit
fi

>$OutFileCash
>$OutFileFut
>$OutFileOpt
>$sym_in_file
for file in $Data_Dir/$YYYY/$MM/$DD/*; do
    d="$(basename -- $file)";
    [[ $d == *"NSE_"* ]] || { `echo $d |cut -d'_' -f1 >>$sym_in_file`; }         
done

#`/home/pengine/prod/live_execs/get_shortcode_from_ds $date_ FO $sym_in_file $sym_out_file`

for file in $Data_Dir/$YYYY/$MM/$DD/*; do
    d="$(basename -- $file)"
    if [[ $d == *"NSE_"* ]]; then
      sym=`echo $d |cut -d'_' -f2`
    else
       continue
       sym_nse=`echo $d|cut -d'_' -f1`
       sym=`grep $sym_nse $sym_out_file | cut -d' ' -f2 | cut -d'_' -f2-`
    fi
    tt=`$Mds_Log ORS_REPLY $file`
    tradcount=`echo $tt| cut -d' ' -f2`
    totcount=`echo $tt | cut -d' ' -f1`
    if [[ $tradcount != 0 ]];then
      ratio=$(( totcount /tradcount ))
    else 
      ratio="-1"
    fi
    echo "$sym $totcount $tradcount $ratio"
#    echo "$Mds_Log ORS_REPLY $file | egrep 'Conf|CxRe|CxlRejc|CxReRejc|Cxld' |wc -l"
    if [[ $sym == *"_FUT"* ]]; then
      echo "$sym $totcount $tradcount $ratio" >>$OutFileFut
    elif [[ $sym == *"_C0_"* ]] || [[ $sym == *"_P0_"* ]]; then
      echo "$sym $totcount $tradcount $ratio" >>$OutFileOpt
    else
      echo "$sym $totcount $tradcount $ratio" >>$OutFileCash
    fi 
done

echo "Updating HTML"
#/home/dvcinfra/important/Generate_ors_detail/script/generate_ors_report.sh
