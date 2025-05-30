#!/bin/bash

Data_Dir="/home/dvcinfra/important/ORS_REPLY_INTRADAILY/IND13_DAILY_FILES/"
Mds_Log="/home/dvcinfra/important/ORS_REPLY_INTRADAILY/mds_fast_first_trade_read_Volume_generic_update"
work_dir='/home/dvcinfra/important/ORS_REPLY_INTRADAILY/Product_Details'
mds_input_file="/tmp/ors_reply_mds_input_file"
mds_output_file="/tmp/ors_reply_mds_output_file"
monthly_limit="/tmp/monthly_limit_log_file"
OutFileCash="$work_dir/cash"
mail_file="/tmp/mail_fileintraratiopos"
back_up_intraday="/home/dvcinfra/important/back_up_postion_intraday/"
declare -A Monthly_data_tot
declare -A Monthly_ratio_tot
declare -A Monthly_trade_tot
declare -A Monthly_data_ind16
declare -A Monthly_trade_ind16
declare -A Monthly_data_ind17
declare -A Monthly_trade_ind17
declare -A Monthly_data_ind18
declare -A Monthly_trade_ind18
declare -A server_to_ip_map
server_to_ip_map=( ["IND16"]='10.23.227.81' ["IND17"]='10.23.227.82' ["IND18"]='10.23.227.83' )

GetPreviousWorkingDay() {
      prev_day=`/home/pengine/prod/live_execs/update_date $prev_day P A`
      is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $prev_day T`
      while [ $is_holiday_ = "1" ];
      do
        prev_day=`/home/pengine/prod/live_execs/update_date $prev_day P A`
        is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $prev_day T`
      done
}

date_=`date +"%Y%m%d"`
today_=$date_
prev_day=$today_
GetPreviousWorkingDay ;
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date_ T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi

YYYY=${today_:0:4}
MM=${prev_day:4:2}
TODAY_MM=${today_:4:2}
echo "prev_month: $MM"
echo "today_month: $TODAY_MM"
>$monthly_limit
if [ $TODAY_MM == $MM ];then
    echo "Same Month .. Reseting Limits";
    cp "/home/dvcinfra/important/Generate_ors_detail/Product_Details_Monthly/${YYYY}${MM}.cash" $monthly_limit
fi

while IFS=' ' read -r sym f1 f2 f3 f4 f5 f6 f7 f8 f9
do
  Monthly_data_ind16[$sym]=$f1
  Monthly_trade_ind16[$sym]=$f2
  Monthly_data_ind17[$sym]=$f3
  Monthly_trade_ind17[$sym]=$f4
  Monthly_data_ind18[$sym]=$f5
  Monthly_trade_ind18[$sym]=$f6
  Monthly_data_tot[$sym]=$f7
  Monthly_trade_tot[$sym]=$f8
  Monthly_ratio_tot[$sym]=$f9
done < $monthly_limit
trading_day=`ls /home/dvcinfra/important/Generate_ors_detail/Product_Details | grep \"${YYYY}${MM}\" | grep cash | wc -l`
trading_day=$(( $trading_day + 1 ))
echo $trading_day
rm -f /tmp/positionlimit_mail_file
rm -f /tmp/monthly_limit_position_mail
rm -f /tmp/monthly_limit_position_mail_tmp
rm -f /tmp/product_intraday_exist
rm -f /tmp/ind17_pos_mail_file
cd $back_up_intraday
pwd
ls | grep -v $date_| xargs rm -f

cd /home/dvcinfra/important/ORS_REPLY_INTRADAILY/IND13_DAILY_FILES/
pwd
ls | grep -v $date_| xargs rm -f
echo "Syncing Cash data"
rsync -avz 10.23.227.63:/spare/local/MDSlogs/GENERIC/ORS_NSE_* /home/dvcinfra/important/ORS_REPLY_INTRADAILY/IND13_DAILY_FILES/ --delete-after
echo "Synced"
>$OutFileCash
>$mds_input_file
for file in $Data_Dir/*$date_*; do
    d="$(basename -- $file)";
    echo $file >> $mds_input_file
done

start_time=`date -d"$date_ -1 day" +"%s"`
end_time=`date -d"$date_ +1 day" +"%s"`
echo "$Mds_Log GENERIC $mds_input_file $start_time $end_time >$mds_output_file" 
$Mds_Log GENERIC $mds_input_file $start_time $end_time >$mds_output_file
echo "MDS Done"
echo "<div style='margin:0 30px;'>" >>/tmp/positionlimit_mail_file ;
echo "<h3>IntraDay </h3>" >>/tmp/positionlimit_mail_file
echo "<div style='margin:0 30px;'>" >>/tmp/monthly_limit_position_mail ;
echo "<h3>Cumulative ( monthly +today )</h3>" >>/tmp/monthly_limit_position_mail;
echo "<div style='margin:0 30px;'>" >>/tmp/ind17_pos_mail_file ;
echo "<h3>IND17 </h3>" >>/tmp/ind17_pos_mail_file;
echo "<table border='1' id='myTable' class='table table-striped' style='border: 1px solid black;border-collapse: collapse' ><thead><tr><th>Symbol</th><th>IND16 Order</th><th>IND16 Exec</th><th>IND16 Ratio</th><th>IND18 Order</th><th>IND18 Exec</th><th>IND18 Ratio</th><th>TotalOrder</th><th>TotalExec</th><th>TotalRatio</th><th>C_Order</th><th>C_Ratio</th><th>Price</th></tr></thead><tbody>">> /tmp/positionlimit_mail_file
echo "<table border='1' id='myTable2' class='table table-striped' style='border: 1px solid black;border-collapse: collapse' ><thead><tr><th>Symbol</th><th>IND16 Order</th><th>IND16 Exec</th><th>IND16 Ratio</th></th><th>IND18 Order</th><th>IND18 Exec</th><th>IND18 Ratio</th><th>TotalOrder</th><th>TotalExec</th><th>TotalRatio</th><th>UptoPrevDay</th><th>Price</th></tr></thead><tbody>">> /tmp/monthly_limit_position_mail
echo "<table border='1' id='myTable3' class='table table-striped' style='border: 1px solid black;border-collapse: collapse' ><thead><tr><th>Symbol</th><th>IND17 Order</th><th>IND17 Exec</th><th>IND17 Ratio</th><th>C_Order</th><th>C_Ratio</th><th>Price</th></tr></thead><tbody>">> /tmp/ind17_pos_mail_file

sort -n -k8 $mds_output_file >/tmp/mds_outsortedFile
cp /tmp/mds_outsortedFile $mds_output_file
log_file="/tmp/prod_alreadysent$today_"
touch $log_file
while read -r sym f1 f2 f3 f4 f5 f6 f7 f8 f9
do
    if [[ $f2 -ne 0 ]];then
       ratio_ind16=$(( f1 / f2 ))
    else 
       ratio_ind16="-1"
    fi
    if [[ $f4 -ne 0 ]];then
        ratio_ind17=$(( f3 / f4 ))
    else 
        ratio_ind17="-1"
    fi
    if [[ $f6 -ne 0 ]];then
        ratio_ind18=$(( f5 / f6 ))
    else 
       ratio_ind18="-1"
    fi
    if [[ $f8 -ne 0 ]];then
      ratio_tot=$(( f7 / f8 ))
    else 
      ratio_tot="-1"
    fi
    color_ind17="white"
    color_comb="white"
    if ! grep -q "${sym}_IND17" $log_file;then
      color_ind17="yellow"
    fi
    if ! grep -q "${sym}_COMB" $log_file;then
      color_comb="yellow"
    fi
    echo "Daily Ratio $sym $f7 $f8 $ratio_tot"
    [[ ! ${Monthly_data_tot[$sym]+abc} ]] && { echo "Not exists"; Monthly_data_ind16[$sym]=0; Monthly_trade_ind16[$sym]=0;Monthly_data_ind17[$sym]=0; Monthly_trade_ind17[$sym]=0;Monthly_data_ind18[$sym]=0; Monthly_trade_ind18[$sym]=0; Monthly_data_tot[$sym]=0; Monthly_trade_tot[$sym]=0;Monthly_ratio_tot[$sym]=0;}
    [[ ${Monthly_ratio_tot[$sym]} -eq -1 ]] && { echo "Ratio -1"; Monthly_ratio_tot[$sym]=1000; }
    mon_ord_ind16=$(( ${Monthly_data_ind16[$sym]} +  $f1 ))
    mon_trade_ind16=$(( ${Monthly_trade_ind16[$sym]} + $f2 ))
    [[ $mon_trade_ind16 -eq 0 ]] && mon_trade_ind16=1
    mon_ord_ind17=$(( ${Monthly_data_ind17[$sym]} +  $f3 ))
    mon_trade_ind17=$(( ${Monthly_trade_ind17[$sym]} + $f4 ))
    [[ $mon_trade_ind17 -eq 0 ]] && mon_trade_ind17=1
    mon_ord_ind18=$(( ${Monthly_data_ind18[$sym]} +  $f5 ))
    mon_trade_ind18=$(( ${Monthly_trade_ind18[$sym]} + $f6 ))
    [[ $mon_trade_ind18 -eq 0 ]] && mon_trade_ind18=1
    mon_ord_tot=$(( ${Monthly_data_tot[$sym]} +  $f7 ))
    mon_trade_tot=$(( ${Monthly_trade_tot[$sym]} + $f8 ))
    [[ $mon_trade_tot -eq 0 ]] && mon_trade_tot=1
    mon_ratio_ind16=$(( $mon_ord_ind16 / $mon_trade_ind16 ))
    mon_ratio_ind17=$(( $mon_ord_ind17 / $mon_trade_ind17 ))
    mon_ratio_ind18=$(( $mon_ord_ind18 / $mon_trade_ind18 ))
    mon_ratio_tot=$(( $mon_ord_tot / $mon_trade_tot ))
    [[ ${Monthly_data_tot[$sym]} -gt 800000 ]] && [[ ${Monthly_ratio_tot[$sym]} -gt 490 ]] && [[ $f7 -lt 10000 ]] && { continue; }
    if [[ $f3 -gt 100000 ]] && [[ $ratio_tot -gt 500 ]]; then
            echo "<tr style=\"background-color:${color_ind17};\"><td>${sym}</td><td>$f3</td><td>$f4</td><td>${ratio_ind17}</td><td>$mon_ord_tot</td><td>${mon_ratio_tot}</td><td>$f9</td></tr>" >>/tmp/ind17_pos_mail_file
            echo "Exist" >/tmp/product_intraday_exist
            echo "${sym}_IND17" >>$log_file
    elif [[ $f3 -gt 50000 ]] && [[ $ratio_tot -eq $f3 ]]; then
            echo "<tr style=\"background-color:${color_ind17};\"><td>${sym}</td><td>$f3</td><td>$f4</td><td>${ratio_ind17}</td><td>$mon_ord_tot</td><td>${mon_ratio_tot}</td><td>$f9</td></tr>" >>/tmp/ind17_pos_mail_file
            echo "Exist" >/tmp/product_intraday_exist
            echo "${sym}_IND17" >>$log_file
    elif [[ $f7 -gt 50000 ]] && [[ $ratio_tot -eq $f7 ]]; then
            echo "<tr style=\"background-color:${color_comb};\"><td>${sym}</td><td>$f1</td><td>$f2</td><td>${ratio_ind16}</td><td>$f5</td><td>$f6</td><td>${ratio_ind18}</td><td>$f7</td><td>$f8</td><td>${ratio_tot}</td><td>$mon_ord_tot</td><td>${mon_ratio_tot}</td><td>$f9</td></tr>" >>/tmp/positionlimit_mail_file
            echo "Exist" >/tmp/product_intraday_exist
            echo "${sym}_COMB" >>$log_file
    elif [[ $f7 -gt 100000 ]] && [[ $ratio_tot -gt 500 ]];then
            echo "<tr style=\"background-color:${color_comb};\"><td>${sym}</td><td>$f1</td><td>$f2</td><td>${ratio_ind16}</td><td>$f5</td><td>$f6</td><td>${ratio_ind18}</td><td>$f7</td><td>$f8</td><td>${ratio_tot}</td><td>$mon_ord_tot</td><td>${mon_ratio_tot}</td><td>$f9</td></tr>" >>/tmp/positionlimit_mail_file
            echo "Exist" >/tmp/product_intraday_exist
            echo "${sym}_COMB" >>$log_file
    fi
    echo $mon_ord_tot $mon_ratio_tot
    price=`echo $f9 | cut -d'.' -f1`
    if [[ $price -gt 1000 ]] ; then
          if [[ $mon_ord_tot -gt 1450000 ]] && [[ $mon_ratio_tot -gt 490 ]];then
              if [[ $mon_ratio_tot -gt 50000 ]];then
                   echo "${sym} $mon_ord_ind16 $mon_trade_ind16 $mon_ratio_ind16 $mon_ord_ind17 $mon_trade_ind17 $mon_ratio_ind17 $mon_ord_ind18 $mon_trade_ind18 $mon_ratio_ind18 $mon_ord_tot $mon_trade_tot -1 $f9 ${Monthly_ratio_tot[$sym]}" >>/tmp/monthly_limit_position_mail_tmp
              else
                    echo "${sym} $mon_ord_ind16 $mon_trade_ind16 $mon_ratio_ind16 $mon_ord_ind17 $mon_trade_ind17 $mon_ratio_ind17 $mon_ord_ind18 $mon_trade_ind18 $mon_ratio_ind18 $mon_ord_tot $mon_trade_tot ${mon_ratio_tot} $f9  ${Monthly_ratio_tot[$sym]}" >>/tmp/monthly_limit_position_mail_tmp
              fi
              echo "Exist" >/tmp/product_intraday_exist
          fi
    else
    if [[ $mon_ord_tot -gt 950000 ]] && [[ $mon_ratio_tot -gt 490 ]];then
              if [[ $mon_ratio_tot -gt 50000 ]];then
                   echo "${sym} $mon_ord_ind16 $mon_trade_ind16 $mon_ratio_ind16 $mon_ord_ind17 $mon_trade_ind17 $mon_ratio_ind17 $mon_ord_ind18 $mon_trade_ind18 $mon_ratio_ind18 $mon_ord_tot $mon_trade_tot -1 $f9 ${Monthly_ratio_tot[$sym]}" >>/tmp/monthly_limit_position_mail_tmp
              else
                   echo "${sym} $mon_ord_ind16 $mon_trade_ind16 $mon_ratio_ind16 $mon_ord_ind17 $mon_trade_ind17 $mon_ratio_ind17 $mon_ord_ind18 $mon_trade_ind18 $mon_ratio_ind18 $mon_ord_tot $mon_trade_tot ${mon_ratio_tot} $f9  ${Monthly_ratio_tot[$sym]}" >>/tmp/monthly_limit_position_mail_tmp
              fi
              echo "Exist" >/tmp/product_intraday_exist
          fi
    fi

done < $mds_output_file

limit_file="/spare/local/files/NSE/PositionLimits/limits.${today_}"
tmp_pos_file="/tmp/PositionLimits.csv"
tmp_pos_file_original="/tmp/PositionLimits_original.csv"
tmp_limit_file="/tmp/limit_file_update"
tmp_file3="/tmp/fileunsortorsintra3"
touch /tmp/monthly_limit_position_mail_tmp
cp /tmp/monthly_limit_position_mail_tmp $tmp_file3 
runsetmaxpos=0
runsetmaxind16=0
runsetmaxind17=0
runsetmaxind18=0
>$mail_file
if [ -s "$tmp_file3" ] 
then
 echo "Enter"
 for server in "${!server_to_ip_map[@]}";
 do
  ssh dvctrader@${server_to_ip_map[$server]} "cat /home/dvctrader/ATHENA/run.sh | grep -v \"^#\"|grep -v START_RATIO|grep LIVE | cut -d' ' -f2" >/tmp/live_file_path.txt
  >/tmp/live_file_dic.txt
  for file in `cat /tmp/live_file_path.txt`;
  do
     echo $(dirname "${file}") >>/tmp/live_file_dic.txt
  done
  sort /tmp/live_file_dic.txt | uniq >/tmp/pos_file_dir.txt
  i=0
  echo "<h2>Server: $server</h2>" >> $mail_file
  for filepath in `cat /tmp/pos_file_dir.txt`;
  do
    pos_file="${filepath}/PositionLimits.csv"
    limit_file="/spare/local/files/NSE/PositionLimits/limits.${today_}"
    [[ $i -gt 0 ]] && limit_file="/spare/local/files/NSE/PositionLimits/limits.${today_}_${i}"
    echo "Postion file: $pos_file"
    echo "Limit file: $limit_file"
    rm -f $tmp_pos_file $tmp_limit_file $tmp_pos_file_original
    i=$((i+1))
    ssh dvctrader@${server_to_ip_map[$server]} "touch $limit_file"
    scp dvctrader@${server_to_ip_map[$server]}:$pos_file $tmp_pos_file
    scp dvctrader@${server_to_ip_map[$server]}:$limit_file $tmp_limit_file
    if [[ ! -f "$tmp_pos_file" ]] && [[ ! -f "$tmp_limit_file" ]]; then
        echo "scp failed for ignoring...$server"
        continue;
    fi
    timestamp=`date +"%s"`
    back_up_pos_file="${back_up_intraday}/PositionLimits.csv_${server}_${date_}_${timestamp}"
    echo $back_up_pos_file
    cp $tmp_pos_file $back_up_pos_file
    cp $tmp_pos_file $tmp_pos_file_original
    for prod in `cat $tmp_file3|cut -d' ' -f1`;
    do
        echo "Symbol Change:- $prod" 
        if ! grep -q "${prod}_" $tmp_pos_file
        then
          echo "Not in Position file, Not Updating.."
          continue;
        fi
        if ! grep -q "${prod}_" $tmp_limit_file
        then
          echo "Adding to Limit File"
          grep "${prod}_" $tmp_pos_file >> $tmp_limit_file
        else 
          echo "Already in limit file continuing"
          continue
        fi
        prod=${prod//&/\\&}
        prodname=`echo "${prod}_MAXLONGPOS"`;
        sed -i "s/$prodname = .*/$prodname = 0/g" $tmp_pos_file ;
        prodname=`echo "${prod}_MAXSHORTPOS"`;
        sed -i "s/$prodname = .*/$prodname = 0/g" $tmp_pos_file;
        prodname=`echo "${prod}_MAXLONGEXPOSURE"`;
        sed -i "s/$prodname = .*/$prodname = 0/g" $tmp_pos_file ;
        prodname=`echo "${prod}_MAXSHORTEXPOSURE"`;
        sed -i "s/$prodname = .*/$prodname = 0/g" $tmp_pos_file ;
    done
    if cmp -s "$tmp_pos_file" "$tmp_pos_file_original"; then
        echo "Files Same , no scp call"
    else
        echo "File diff for from some prod..copying to $server"
        scp $tmp_limit_file dvctrader@${server_to_ip_map[$server]}:$limit_file
        scp $tmp_pos_file dvctrader@${server_to_ip_map[$server]}:$pos_file
        runsetmaxpos=1;
    fi
    echo "<h4>Postionlimit file: $pos_file</h4>" >> $mail_file
    for prod in `cat $tmp_file3|cut -d' ' -f1`;
    do
      prod=${prod//&/\\&}
      if ! grep -q "${prod}_" $tmp_pos_file
      then
        echo "Not In Pos file"
        continue;
      fi
      echo "$prod<br/>" >> $mail_file
      ssh dvctrader@${server_to_ip_map[$server]} "grep "${prod}_" $pos_file" >> $mail_file
      echo "<br/>" >> $mail_file
    done
  done
 done
fi

if [[ $runsetmaxpos -eq 1 ]];then
  echo "Running set max for machines"
  for i in 123707 123708 123709 ; do ssh dvctrader@10.23.227.81 "/home/pengine/prod/live_execs/user_msg --traderid $i --setmaxpos 1" ;done
  for i in 123805 123806 123807 123808 ; do ssh dvctrader@10.23.227.82 "/home/pengine/prod/live_execs/user_msg --traderid $i --setmaxpos 1" ; done
  for i in 123515 123516 123517 123518 ; do ssh dvctrader@10.23.227.83 "/home/pengine/prod/live_execs/user_msg --traderid $i --setmaxpos 1" ; done
fi

sort -n -k11 /tmp/monthly_limit_position_mail_tmp >/tmp/month_soretedFile
cp /tmp/month_soretedFile /tmp/monthly_limit_position_mail_tmp
while read -r sym f1 f2 f3 f4 f5 f6 f7 f8 f9 f10 f11 f12 f13 f14
do
  color_month="white"
  if ! grep -q "${sym}_MONTH" $log_file;then
      color_month="yellow"
  fi
  echo "<tr style=\"background-color:${color_month};\"><td>${sym}</td><td>$f1</td><td>$f2</td><td>$f3</td><td>$f7</td><td>$f8</td><td>$f9</td><td>$f10</td><td>$f11</td><td>$f12</td><td>$f14</td><td>$f13</td></tr>" >>/tmp/monthly_limit_position_mail
  echo "${sym}_MONTH" >>$log_file
done < /tmp/monthly_limit_position_mail_tmp

echo -e "</tbody></table>\n" >> /tmp/positionlimit_mail_file ;
echo -e "</tbody></table>\n" >> /tmp/monthly_limit_position_mail ;
echo -e "</tbody></table>\n" >> /tmp/ind17_pos_mail_file
echo "</div>" >>/tmp/positionlimit_mail_file
echo "</div>" >>/tmp/monthly_limit_position_mail
echo "</div>" >>/tmp/ind17_pos_mail_file
echo "Check Mail"
if [[ -f /tmp/product_intraday_exist ]];then
        echo "Sending Mail"
        (
          echo "To: raghunandan.sharma@tworoads-trading.co.in, hardik.dhakate@tworoads-trading.co.in, ravi.parikh@tworoads.co.in, uttkarsh.sarraf@tworoads.co.in, nishit.bhandari@tworoads-trading.co.in"
#          echo "To: raghunandan.sharma@tworoads-trading.co.in"
          echo "Subject: Ratio Limit Alert for ${today_}"
          echo "Content-Type: text/html"
          echo
          cat /tmp/positionlimit_mail_file
          cat /tmp/monthly_limit_position_mail
          cat /tmp/ind17_pos_mail_file
          echo
          cat $mail_file
        ) | /usr/sbin/sendmail -t
fi

echo "Updating HTML"
cp $mds_output_file $OutFileCash
awk '{$10=""; print $0}' $monthly_limit >/tmp/monthly_remove_ratio
cp /tmp/monthly_remove_ratio $monthly_limit
cat $monthly_limit >> $OutFileCash
/home/dvcinfra/important/ORS_REPLY_INTRADAILY/script/generate_ors_report_intraday.sh
echo "Done"
