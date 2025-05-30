#!/bin/bash
Data_Dir="/NAS1/data/ORSData/NSE"
Mds_Log="/home/pengine/prod/live_execs/mds_log_reader"

DD=$1

sym_in_file="/tmp/sym_to_convert_202005${DD}"
sym_out_file="/tmp/sym_converted_202005${DD}"
>$sym_out_file
>$sym_in_file

for file in $Data_Dir/2020/05/$DD/*; do
    d="$(basename -- $file)";
    [[ $d == *"NSE_"* ]] || { `echo $d |cut -d'_' -f1 >>$sym_in_file`; }
done
/home/pengine/prod/live_execs/get_shortcode_from_ds 202005${DD} FO $sym_in_file $sym_out_file

>/home/dvcinfra/ThrottleResult/throttle_output_202005${DD}
echo "Symbol shortcode ExchMssg Throttle_mssg Throttle_Ratio Conf Rejc Rejc/Conf Cxl Cxlrejc CxlRejc/Cxl CxRe CxReRejc CxReRejc/CxRe" >>/home/dvcinfra/ThrottleResult/throttle_output_202005${DD}
cd /run/media/dvcinfra/DATA/ORSData/NSE/2020/05/${DD}
for file in `ls /run/media/dvcinfra/DATA/ORSData/NSE/2020/05/${DD}` ;
#for file in `ls /run/media/dvcinfra/DATA/ORSData/NSE/2020/05/${DD}/NSE1136806_20200511.gz` ;
do
      d="$(basename -- $file)"
      if [[ $d == *"NSE_"* ]]; then
            continue
            sym=`echo $d |cut -d'_' -f2`
      else
        sym_nse=`echo $d|cut -d'_' -f1`
        sym=`grep $sym_nse $sym_out_file | cut -d' ' -f2 | cut -d'_' -f2-`
        echo  $sym
      fi
      file1="/tmp/file1_202005${DD}"
      file2="/tmp/f123_202005${DD}"
      $Mds_Log ORS_REPLY $file >$file1
      cat $file1 | cut -d' ' -f14 | sort | uniq>$file2
      if [[ $sym == *"_FUT"* ]];then
        echo "SYM: $d $sym"
        for line in `cat $file2`; do
            echo "LINE: $line"
            machine=$((line >> 16 ))
            echo "Machine: $machine"
            sed -i "s/SACI: $line/SACI: $machine/g" $file1
        done
      egrep 'SACI: 340|SACI: 305' $file1 >/tmp/tmp_file1
      cp /tmp/tmp_file1 $file1 
      fi
      tot_mssg=`egrep '\bConf\b|\bCxRe\b|\bCxld\b' $file1 |wc -l`
      th_mssg=`egrep '\bConf\b|\bCxRe\b|\bCxlRejc\b|\bCxReRejc\b|\bCxld\b|\bRejc\b' $file1|egrep '\bSE: 7\b|\bSE: 9\b' |wc -l`
      conf=`egrep '\bORR: Conf\b' $file1 | wc -l`
      rejc=`egrep '\bORR: Rejc\b' $file1| egrep '\bSE: 7\b|\bSE: 9\b'| wc -l`
      cxl=`egrep '\bORR: Cxld\b' $file1| wc -l`
      cxlrejc=`egrep '\bORR: CxlRejc\b' $file1| egrep '\bSE: 7\b|\bSE: 9\b'| wc -l` 
      mod=`egrep '\bORR: CxRe\b' $file1|wc -l`
      modrejc=`egrep '\bORR: CxReRejc\b' $file1| egrep '\bSE: 7\b|\bSE: 9\b'|wc -l`
      if [[ $tot_mssg -ne 0 ]];then
            th_ratio=$(( th_mssg / tot_mssg ))
            th_ratio=`bc <<< "scale=3; $th_mssg/$tot_mssg"`
      else
            th_ratio="-1"
      fi
      if [[ $conf -ne 0 ]];then
            conf_ratio=`bc <<< "scale=3; $rejc/$conf"`
      else
            conf_ratio="-1"
      fi
      if [[ $cxl -ne 0 ]];then
            cxl_ratio=`bc <<< "scale=3; $cxlrejc/$cxl"`
      else
            cxl_ratio="-1"
      fi
      if [[ $mod -ne 0 ]];then
            mod_ratio=`bc <<< "scale=3; $modrejc/$mod"`
      else
            mod_ratio="-1"
      fi
      echo $d $sym $tot_mssg $th_mssg $th_ratio $conf $rejc $conf_ratio $cxl $cxlrejc $cxl_ratio $mod $modrejc $mod_ratio >>/home/dvcinfra/ThrottleResult/throttle_output_202005${DD}
done
