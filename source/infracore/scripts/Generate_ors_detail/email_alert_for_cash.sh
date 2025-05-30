#!/bin/bash
work_dir='/home/dvcinfra/important/Generate_ors_detail/Product_Details/'
mail_file="/tmp/mail_fileors"
tmp_file1="/tmp/fileunsortors"
tmp_file2="/tmp/filesortors"
tmp_file3="/tmp/fileunsortors3"
declare -A server_to_ip_map
server_to_ip_map=( ["IND16"]='10.23.227.81' ["IND17"]='10.23.227.82' ["IND18"]='10.23.227.83' ["IND22"]='10.23.227.71' )
#server_to_ip_map=( ["IND16"]='10.23.227.81' ["IND18"]='10.23.227.83' )


declare -A month_product_cash
declare -A month_exec_cash
declare -A product_price

SendMail(){
#      (echo To: "raghunandan.sharma@tworoads-trading.co.in" ; echo From: "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>"; echo Subject: "Daily ORS Report for Cash Products $date_"; echo "Content-Type: text/html;";cat ${mail_file}) | /usr/sbin/sendmail -t
      (echo To: "raghunandan.sharma@tworoads-trading.co.in nseall@tworoads.co.in hardik.dhakate@tworoads-trading.co.in" ; echo From: "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>"; echo Subject: "Daily ORS Report for Cash Products $prev_day"; echo "Content-Type: text/html;";cat ${mail_file}) | /usr/sbin/sendmail -t
}

GetPreviousWorkingDay() {
    prev_day=`/home/pengine/prod/live_execs/update_date $prev_day P A`
    is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $prev_day T`
    while [ $is_holiday_ = "1" ];
    do
         prev_day=`/home/pengine/prod/live_execs/update_date $prev_day P A`
         is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $prev_day T`
    done
}


if [ $# -ne 1 ] ; then
    echo "Called As : " $* ;
    echo "$0 Date todays" ;
    exit ;
fi

today_=$1
prev_day=$1
GetPreviousWorkingDay ;

YYYY=${prev_day:0:4}
MM=${prev_day:4:2}
DD=${prev_day:6:2}
TODAY_MM=${today_:4:2}
MSTR=$(echo $(date -d $today_ +%b) | awk '{print toupper($1)}') ;
count=0
bhavcopy="/spare/local/tradeinfo/NSE_Files/Margin_Files/Exposure_Files/cm${DD}${MSTR}${YYYY}bhav.csv"
awk -F',' '$2 == "EQ" {print $1" "$6}' $bhavcopy >>/tmp/product_price_file_with_deci
cat /tmp/product_price_file_with_deci | cut -d'.' -f1 >>/tmp/product_price_file
while IFS=' ' read -r f1 f2
do
    product_price[$f1]=$f2
done < /tmp/product_price_file

for cashFile in `ls $work_dir/* | grep "${YYYY}${MM}.*cash"`; do
    count=$(( count + 1 ))
    echo $cashFile
    while IFS=' ' read -r sym_t f1 f2 f3 f4 f5 f6 f7 f8 f9
    do
        if [[ $f8 == '' ]]; then
            continue;
        fi
        sym=`echo $sym_t | cut -d'_' -f2`
        if [ ${month_product_cash[$sym]+_} ]; then
             month_product_cash[$sym]=$(( ${month_product_cash[$sym]} + $f7))
             month_exec_cash[$sym]=$(( ${month_exec_cash[$sym]} +  $f8))
        else
             month_product_cash[$sym]=$f7
             month_exec_cash[$sym]=$f8
        fi
    done < $cashFile
done

>$tmp_file1
>$tmp_file3
for sym in "${!month_product_cash[@]}"; do
    ratio=-1
    avg_cash_tot=$(( ${month_product_cash[$sym]} / $count ))
    avg_cash_exec=$(( ${month_exec_cash[$sym]} / $count ))
    [[ ${month_exec_cash[$sym]} == 0 ]] || ratio=$((${month_product_cash[$sym]} / ${month_exec_cash[$sym]}))
    [[ ! ${product_price[$sym]+abc} ]] && { echo "Not exists $sym"; product_price[$sym]=0; }
    if [[ ${product_price[$sym]} -gt 1000 ]] ; then
        if [[ ${month_product_cash[$sym]} -gt 1400000 ]]; then
           if [[ $ratio -eq -1 ]] || [[ $ratio -gt 490 ]]; then
              echo "$sym ${month_product_cash[$sym]} ${month_exec_cash[$sym]} $ratio ${product_price[$sym]}" >> $tmp_file1
              echo "$sym ${month_product_cash[$sym]} ${month_exec_cash[$sym]} $ratio" >> $tmp_file3
           fi
        elif [[ $avg_cash_tot -gt 80000 ]];then
           if [[ $ratio -eq -1 ]] || [[ $ratio -gt 490 ]]; then
              echo "$sym ${month_product_cash[$sym]} ${month_exec_cash[$sym]} $ratio ${product_price[$sym]}" >> $tmp_file1
           fi
        fi                                                                                 
    else
      if [[ ${month_product_cash[$sym]} -gt 800000 ]]; then
          if [[ $ratio -eq -1 ]] || [[ $ratio -gt 490 ]]; then
              echo "$sym ${month_product_cash[$sym]} ${month_exec_cash[$sym]} $ratio ${product_price[$sym]}" >> $tmp_file1
              echo "$sym ${month_product_cash[$sym]} ${month_exec_cash[$sym]} $ratio" >> $tmp_file3
          fi
      elif [[ $avg_cash_tot -gt 80000 ]];then
          if [[ $ratio -eq -1 ]] || [[ $ratio -gt 490 ]]; then
              echo "$sym ${month_product_cash[$sym]} ${month_exec_cash[$sym]} $ratio ${product_price[$sym]}" >> $tmp_file1
          fi
      fi
    fi
done

sort -n -rk 2 $tmp_file1 > $tmp_file2
>$mail_file
echo -e "<p>Trading Days: $count</p>" >> $mail_file
echo -e "<br/><table border="1"><thead><th> Products </th><th> Total </th><th> Exec </th><th> Ratio </th><th>Price</th><tbody>" >> $mail_file
while IFS=' ' read -r sym f1 f2 f3 f4
do
    echo "<tr><td>$sym</td><td>$f1</td><td>$f2</td><td>$f3</td><td>$f4</td></tr>" >> $mail_file
done < $tmp_file2

echo -e "</tbody></table>" >> $mail_file
echo -e "<br/><br/>" >> $mail_file

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $today_ T`
if [ $is_holiday = "1" ];then
    echo "NSE holiday..., exiting";
    SendMail
    exit
fi
echo "prev_month: $MM"
echo "today_month: $TODAY_MM"
if [ $TODAY_MM != $MM ];then
    echo "New Month .. Reseting Limits";
    SendMail
    exit
fi
########################################################
SendMail
echo "NOT UPDATING POS IN CONFIG"
exit
#########################################################

limit_file="/spare/local/files/NSE/PositionLimits/limits.${today_}"
tmp_pos_file="/tmp/PositionLimits.csv"
tmp_pos_file_original="/tmp/PositionLimits_original.csv"
tmp_limit_file="/tmp/limit_file_update"

for server in "${!server_to_ip_map[@]}";
do
  ssh dvctrader@${server_to_ip_map[$server]} "cat /home/dvctrader/ATHENA/run.sh | grep -v \"^#\"|grep -v START_RATIO|grep -v CONFIG_CM_NON_FO|grep LIVE | cut -d' ' -f2" >/tmp/live_file_path.txt
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
      if [ ! -f $tmp_pos_file ]; then
          echo "scp failed File not found!.ignoring"
          continue;
      fi
      cp $tmp_pos_file /home/dvcinfra/important/back_up_postion/PositionLimits.csv_${today_}_${server}_${i}
      cp $tmp_limit_file /home/dvcinfra/important/back_up_postion/limits.${today_} 
      cp $tmp_pos_file $tmp_pos_file_original
      for prod in `cat $tmp_file3|cut -d' ' -f1`;
      do
          echo "Symbol Change:- $prod" 
          if ! grep -q "NSE_${prod}_" $tmp_pos_file
          then
              echo "Not in Position file, Not Updating.."
              continue;
          fi 
          if ! grep -q "NSE_${prod}_" $tmp_limit_file
          then
              echo "Adding to Limit File"
              grep "NSE_${prod}_" $tmp_pos_file >> $tmp_limit_file
          fi
          prod=${prod//&/\\&}
          prodname=`echo "NSE_${prod}_MAXLONGPOS"`;
          sed -i "s/$prodname = .*/$prodname = 0/g" $tmp_pos_file ;
          prodname=`echo "NSE_${prod}_MAXSHORTPOS"`;
          sed -i "s/$prodname = .*/$prodname = 0/g" $tmp_pos_file ;
          prodname=`echo "NSE_${prod}_MAXLONGEXPOSURE"`;
          sed -i "s/$prodname = .*/$prodname = 0/g" $tmp_pos_file ;
          prodname=`echo "NSE_${prod}_MAXSHORTEXPOSURE"`;
          sed -i "s/$prodname = .*/$prodname = 0/g" $tmp_pos_file ;
      done
      if cmp -s "$tmp_pos_file" "$tmp_pos_file_original"; then
           echo "Files Same , no scp call"
       else
           echo "File diff for from some prod..copying to $server"
           scp $tmp_limit_file dvctrader@${server_to_ip_map[$server]}:$limit_file
           scp $tmp_pos_file dvctrader@${server_to_ip_map[$server]}:$pos_file
      fi

      echo "<h4>Postionlimit file: $pos_file</h4>" >> $mail_file
      for prod in `cat $tmp_file3|cut -d' ' -f1`;
      do
        prod=${prod//&/\\&}
        if ! grep -q "NSE_${prod}_" $tmp_pos_file
        then
            echo "Not In Pos file"
            continue;
        fi
        echo "NSE_$prod<br/>" >> $mail_file
        ssh dvctrader@${server_to_ip_map[$server]} "grep \"NSE_${prod}_\" $pos_file" >> $mail_file
        echo "<br/>" >> $mail_file
      done
  done
done

for i in 123707 123708 123709 123710 123711 123614 123615; do ssh dvctrader@10.23.227.81 "/home/pengine/prod/live_execs/user_msg --traderid $i --setmaxpos 1" ;done
for i in 123805 123806 123807 123808 123810 123811 123812 123708 123709; do ssh dvctrader@10.23.227.82 "/home/pengine/prod/live_execs/user_msg --traderid $i --setmaxpos 1" ; done
for i in 123515 123516 123517 123518 123519 123520; do ssh dvctrader@10.23.227.83 "/home/pengine/prod/live_execs/user_msg --traderid $i --setmaxpos 1" ; done
#for i in 123805 123806 123807 123808 123810 123811 123812 123708 123709; do ssh dvctrader@10.23.227.71 "/home/pengine/prod/live_execs/user_msg --traderid $i --setmaxpos 1" ; done

SendMail
