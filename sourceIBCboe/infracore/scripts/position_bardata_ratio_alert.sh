#!/bin/bash

GetPreviousWorkingDay() {
  YYYYMMDD=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P W`
  is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
  while [ $is_holiday_ = "1" ];
  do
    YYYYMMDD=`/home/pengine/prod/live_execs/update_date $YYYYMMDD P W`
    is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $YYYYMMDD T`
  done
}
GetNextWorkingDay(){
  next_working_day=`/home/pengine/prod/live_execs/update_date $date N W`
  is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
  while [[ $is_holiday = "1" ]]
  do
     next_working_day=`/home/pengine/prod/live_execs/update_date $next_working_day N W`
     is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $next_working_day T`
  done
}

init () {
  #Option=$1
  date=$1
  mode=$2
  YYYYMMDD=$date
  GetPreviousWorkingDay ;
  yesterday=$YYYYMMDD
  GetNextWorkingDay ;
# if mode is evening update the yesterday as today 
  [[ $mode == 'E' ]] && yesterday=$date
  [[ $mode == 'E' ]] || next_working_day=$date
  is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date T`

#  echo "yester, is_holiday:: $yesterday, $is_holiday"
#  echo $next_working_day
#  exit
  if [ $is_holiday = "1" ] ; then
    echo "NSE Holiday. Exiting...";
    exit;
  fi

  yyyy=${yesterday:0:4}
  mm=${yesterday:4:2}
  dd=${yesterday:6:2}
  echo "yesterday=> $yesterday"
  t1=`date --date="$yesterday" +%s`
  echo "date, t1= $date_format, $t1"
  start_ratio_file="/tmp/start_ratio_alert_report.txt"
  end_ratio_file="/tmp/end_ratio_alert_report.txt"
  bar_file="/tmp/bar_alert_report.txt"
  position_file="/tmp/position_alert_report.txt"
  mail_report="/tmp/pos_bar_ratio_report.html"

  ref_data="/spare/local/tradeinfo/NSE_Files/RefData/"
  shortcode_local="/tmp/temp1PositionBar"
  shortcode_server="/tmp/temp2PositionBar"
  servers="IND11 IND12 IND13 IND14 IND15 IND16 IND17 IND18 IND22 IND23 IND19 IND20"

  declare -A IND_Server_ip
  IND_Server_ip=( ["IND11"]="10.23.227.61" \
                ["IND12"]="10.23.227.62" \
                ["IND13"]="10.23.227.63" \
                ["IND14"]="10.23.227.64" \
                ["IND15"]="10.23.227.65" \
                ["IND16"]="10.23.227.81" \
                ["IND17"]="10.23.227.82" \
                ["IND18"]="10.23.227.83" \
                ["IND22"]="10.23.227.71" \
                ["IND23"]="10.23.227.72" \
                ["IND24"]="10.23.227.74" \
                ["IND19"]="10.23.227.69" \
                ["IND20"]="10.23.227.84")

#  IND_Server_ip=( ["IND11"]="-p 22761 dvctrader@202.189.245.205" \
#                ["IND12"]="-p 22762 dvctrader@202.189.245.205" \
#                ["IND13"]="-p 22763 dvctrader@202.189.245.205" \
#              ["IND14"]="-p 22764 dvctrader@202.189.245.205" \
#                 ["IND15"]="-p 22765 dvctrader@202.189.245.205" \
#                  ["IND16"]="-p 22781 dvctrader@202.189.245.205" \
#                ["IND17"]="-p 22782 dvctrader@202.189.245.205" \
#                  ["IND18"]="-p 22783 dvctrader@202.189.245.205" \
#                  ["IND19"]="-p 22769 dvctrader@202.189.245.205" \
#                 ["IND20"]="-p 22784 dvctrader@202.189.245.205")

count=1
  declare -a startRatio_array=()
  declare -a startRatio_server=()
  declare -a startRatio_count=()
  declare -a startRatio_di=()
  declare -a startRatio_shortCode=()
  declare -a EndRatio_array=()
  declare -a endRatio_server=()
  declare -a endRatio_count=()
  declare -a endRatio_di=()
  declare -a endRatio_shortCode=()
 # if [ $count -eq 1 ]; then
  true> $mail_report
  echo '<div style="display:flex;justify-content:space-around;">' >>$mail_report
  echo "<div style='margin:0 30px;'>" >>$mail_report
  #cat /tmp/position_bar_data_ratio_alert_header.txt > $mail_report 
  echo "<h3>Start Ratio Alert: </h3>">>$mail_report
#Start ratio
  startRatiolocal=`grep FUT "${ref_data}nse_fo_${next_working_day}_contracts.txt"| awk '{print $4}' | sort|uniq|wc -l`
  echo `grep FUT $ref_data"nse_fo_"$next_working_day"_contracts.txt"| awk '{print $4}' | sort|uniq`|tr " " "\n" > $shortcode_local
echo "<table id='myTable' class='table table-striped' ><thead><tr><th>Server</th><th>Count</th><th>Diff</th><th>Missing</th></tr></thead><tbody>">> $mail_report
  for server in "${!IND_Server_ip[@]}";
  do
    if [ "$server" == "IND11" ] || [ "$server" == "IND12" ] || [ "$server" == "IND13" ] || [ "$server" == "IND14" ]; then
      echo -e "$server continue\n"
      continue;
    fi
    StartRatio_count=`ssh ${IND_Server_ip[$server]} "grep $yesterday /spare/local/NseHftFiles/Ratio/StartRatio/NSE_*FUT0 | wc -l"`
    echo "Server, Start, grepRes=> $server, $StartRatio_count, $grepRes"
#    if [ $StartRatio_count -lt 380 ] ; then
      echo -e "$yesterday $server $StartRatio_count\n" >> $start_ratio_file ;
      startRatio_array+=("$server") 
      start_map[$server]=`expr $StartRatio_count`;
        
      echo `ssh ${IND_Server_ip[$server]} "grep -l $yesterday /spare/local/NseHftFiles/Ratio/StartRatio/NSE_*FUT0|cut -d '/' -f 7|cut -d '_' -f 2|sort|uniq"`|tr " " "\n" > $shortcode_server
        serc=$(wc -l <"$shortcode_server");
        echo "serc "$serc
        di=$( expr $startRatiolocal - $serc)
        echo "di "$di
       #echo -e "<tr><td>$server</td><td>$StartRatio_count</td><td>$di</td><td>`comm -3  $shortcode_local $shortcode_server`</td></tr>\n" >> $mail_report ;
        startRatio_server+=($server)
        startRatio_count+=($StartRatio_count)
        startRatio_di+=($di)
        startRatio_shortCode+=("`comm -3  $shortcode_local $shortcode_server`")
#    fi
  done
  comm_count=1;
  for (( i=1; i<${#startRatio_shortCode[@]}; i++ )); do
         if [[ ${startRatio_shortCode[0]} == ${startRatio_shortCode[$i]} ]] ; then
          comm_count=$(( $comm_count + 1))
          fi
              echo "Count Inside:"$comm_count
    done
    echo "Count:"$comm_count
    if [[ $comm_count == ${#startRatio_shortCode[@]} ]]; then
      for (( i=0; i<${#startRatio_shortCode[@]}; i++ )); do
           if [ $i = 0 ] ; then 
             echo -e "<tr><td>${startRatio_server[$i]}</td><td>${startRatio_count[$i]}</td><td>${startRatio_di[$i]}</td><td style='border: 1px solid black' rowspan="0">${startRatio_shortCode[$i]}</td></tr>\n" >> $mail_report ;
           else
            echo -e "<tr><td>${startRatio_server[$i]}</td><td>${startRatio_count[$i]}</td><td>${startRatio_di[$i]}</td></tr>\n" >> $mail_report ;
             fi
       done
    else
      for (( i=0; i<${#startRatio_shortCode[@]}; i++ )); do
        echo -e "<tr><td>${startRatio_server[$i]}</td><td>${startRatio_count[$i]}</td><td>${startRatio_di[$i]}</td><td>${startRatio_shortCode[$i]}</td></tr>\n" >> $mail_report ;
        done
    fi
  echo -e "</tbody></table>\n" >> $mail_report ;
  echo "</div>" >> $mail_report ;
  echo "<div style='margin:0 30px;'>" >>$mail_report ;
  
  echo "<h3>End Ratio Alert: </h3>">>$mail_report  
#End ratio
  endRatiolocal=`grep FUT "${ref_data}nse_fo_${next_working_day}_contracts.txt"| awk '{print $4}' | sort|uniq|wc -l`
  echo `grep FUT $ref_data"nse_fo_"$next_working_day"_contracts.txt"| awk '{print $4}' | sort|uniq`|tr " " "\n" > $shortcode_local
  echo "<table id='myTable' class='table table-striped' ><thead><tr><th>Server</th><th>Count</th><th>Diff</th><th>Missing</th></tr></thead><tbody>">> $mail_report
  for server in "${!IND_Server_ip[@]}";
  do
    if [ "$server" == "IND11" ] || [ "$server" == "IND12" ] || [ "$server" == "IND13" ] || [ "$server" == "IND14" ]; then
      echo -e "$server continue\n"
      continue;
    fi
    EndRatio_count=`ssh ${IND_Server_ip[$server]} "grep $yesterday /spare/local/NseHftFiles/Ratio/EndRatio/NSE_*FUT0 | wc -l"`
    echo "Server, end=> $server, $EndRatio_count"
#    if [ $EndRatio_count -lt 380 ]; then
      echo -e "$yesterday $server $EndRatio_count\n" >> $end_ratio_file ;
      EndRatio_array+=("$server")
      end_map[$server]=`expr $EndRatio_count`;

      echo `ssh ${IND_Server_ip[$server]} "grep -l $yesterday /spare/local/NseHftFiles/Ratio/EndRatio/NSE_*FUT0|cut -d '/' -f 7|cut -d '_' -f 2|sort|uniq"`|tr " " "\n" > $shortcode_server
        serc=$(wc -l <"$shortcode_server");
        echo "serc "$serc
        di=$(expr $endRatiolocal - $serc)
        echo "di"$di
       #echo -e "<tr><td>$server</td><td>$EndRatio_count</td><td>$di</td><td>`comm -3  $shortcode_local $shortcode_server`</td></tr>\n" >> $mail_report ;
        endRatio_server+=($server)
        endRatio_count+=($EndRatio_count)
        endRatio_di+=($di)
        endRatio_shortCode+=("`comm -3  $shortcode_local $shortcode_server`")

#    fi
  done
  comm_count=1;
  for (( i=1; i<${#endRatio_shortCode[@]}; i++ )); do
         if [[ ${endRatio_shortCode[0]} == ${endRatio_shortCode[$i]} ]] ; then
          comm_count=$(( $comm_count + 1))
          fi
              echo "COunt INside:"$comm_count
    done
    echo "Count:"$comm_count
    if [[ $comm_count == ${#endRatio_shortCode[@]} ]]; then
      for (( i=0; i<${#endRatio_shortCode[@]}; i++ )); do
           if [ $i = 0 ] ; then
             echo -e "<tr><td>${endRatio_server[$i]}</td><td>${endRatio_count[$i]}</td><td>${endRatio_di[$i]}</td><td style='border: 1px solid black' rowspan="0">${endRatio_shortCode[$i]}</td></tr>\n" >> $mail_report ;
           else
            echo -e "<tr><td>${endRatio_server[$i]}</td><td>${endRatio_count[$i]}</td><td>${endRatio_di[$i]}</td></tr>\n" >> $mail_report ;
             fi
       done
    else
      for (( i=0; i<${#endRatio_shortCode[@]}; i++ )); do
        echo -e "<tr><td>${endRatio_server[$i]}</td><td>${endRatio_count[$i]}</td><td>${endRatio_di[$i]}</td><td>${endRatio_shortCode[$i]}</td></tr>\n" >> $mail_report ;
        done
    fi
  echo -e "</tbody></table>\n" >> $mail_report ;
  echo "</div>" >> $mail_report ;

  echo "<div style='margin:0 30px;'>" >>$mail_report ;
  echo "<h3>Difference Start and End ratio Symbol:</h3>" >>$mail_report
  echo "<table id='myTable' class='table table-striped' style='border: 1px solid black;border-collapse: collapse' ><thead><tr><th>Missing</th></tr></thead><tbody>">> $mail_report
  server="IND17"
  echo `ssh ${IND_Server_ip[$server]} "grep -l $yesterday /spare/local/NseHftFiles/Ratio/StartRatio/NSE_*FUT0|cut -d '/' -f 7"`|tr " " "\n" > $shortcode_server
  sort $shortcode_server >/tmp/start_ratio_symbol_s
  echo `ssh ${IND_Server_ip[$server]} "grep -l $yesterday /spare/local/NseHftFiles/Ratio/EndRatio/NSE_*FUT0|cut -d '/' -f 7"`|tr " " "\n" > $shortcode_server
  sort $shortcode_server >/tmp/end_ratio_symbol_s
  startrc=$(wc -l <"/tmp/start_ratio_symbol_s");
  endrc=$(wc -l <"/tmp/end_ratio_symbol_s");
  echo "serc "$startrc
  di=$(expr $startrc - $endrc)
  comm -3  /tmp/end_ratio_symbol_s /tmp/start_ratio_symbol_s >$shortcode_server
  for line in `cat $shortcode_server`;
  do
    echo -e "<tr><td style='border: 1px solid black' >$line</td></tr>\n" >> $mail_report ;
  done
  echo -e "</tbody></table>\n" >> $mail_report ;
  echo "</div>" >> $mail_report ;
  echo "</div>" >> $mail_report ;


#START AND END RATIO FILE ALERT

  echo '<div style="display:flex;justify-content:space-around;">' >>$mail_report ;
  echo "<div style='margin:0 30px;'>" >>$mail_report ;
  echo "<h3>Start Ratio Not Generated for for:</h3>" >>$mail_report
  echo "<table border='1' id='myTable' class='table table-striped' style='border: 1px solid black;border-collapse: collapse' ><thead><tr><th>ShortCode</th><th>CASH_FUT0</th><th>FUT1_FUT0</th></tr></thead><tbody>">> $mail_report

  for sh in `cat $shortcode_local`; do
      echo "grep $yesterday  /home/dvctrader/usarraf/NseHftFiles/Ratio/StartRatio/NSE_${sh}_NSE_${sh}_FUT0 | wc -l"
      cash_count=`grep $yesterday /home/dvctrader/usarraf/NseHftFiles/Ratio//StartRatio/NSE_${sh}_NSE_${sh}_FUT0 | wc -l`;
      echo "grep $yesterday  /home/dvctrader/usarraf/NseHftFiles/Ratio/NSE_${sh}_FUT1_NSE_${sh}_FUT0  | wc -l"
      fut_count=`grep $yesterday  /home/dvctrader/usarraf/NseHftFiles/Ratio/StartRatio/NSE_${sh}_FUT1_NSE_${sh}_FUT0 | wc -l`;
      [[ $cash_count -eq 1 && $fut_count -eq 1 ]] && continue;
      echo -e "<tr><td>$sh</td><td>$cash_count</td><td>$fut_count</td></tr>\n" >> $mail_report ;
  done
  echo -e "</tbody></table>\n" >> $mail_report ;
  echo "</div>" >> $mail_report ;
  echo "<div style='margin:0 30px;'>" >>$mail_report ;

  echo "<h3>End Ratiio Not Generated For: </h3>">>$mail_report
  echo "<table border='1' id='myTable' class='table table-striped' style='border: 1px solid black;border-collapse: collapse' ><thead><tr><th>ShortCode</th><th>CASH_FUT0</th><th>FUT1_FUT0</th></tr></thead><tbody>">> $mail_report

  for sh in `cat $shortcode_local`; do
        echo "grep $yesterday  /home/dvctrader/usarraf/NseHftFiles/Ratio/EndRatio/NSE_${sh}_NSE_${sh}_FUT0 | wc -l"
        cash_count=`grep $yesterday  /home/dvctrader/usarraf/NseHftFiles/Ratio/EndRatio/NSE_${sh}_NSE_${sh}_FUT0 | wc -l`;
        echo "grep $yesterday  /home/dvctrader/usarraf/NseHftFiles/Ratio/EndRatio/NSE_${sh}_FUT1_NSE_${sh}_FUT0  | wc -l"
        fut_count=`grep $yesterday /home/dvctrader/usarraf/NseHftFiles/Ratio/EndRatio/NSE_${sh}_FUT1_NSE_${sh}_FUT0 | wc -l`;
        [[ $cash_count -eq 1 && $fut_count -eq 1 ]] && continue;
        echo -e "<tr><td>$sh</td><td>$cash_count</td><td>$fut_count</td></tr>\n" >> $mail_report ;
  done
  echo -e "</tbody></table>\n" >> $mail_report ;
  echo "</div>" >> $mail_report ;
  echo "</div>" >> $mail_report ;
#Bar Data
  echo '<div style="display:flex;justify-content:space-around;">' >>$mail_report ;
  today_start_time=`date -d"$yesterday" +"%s"`
  today_end_time=$((today_start_time+35940));
  echo "$today_end_time"
  ssh 10.23.227.69 "grep $today_end_time /spare/local/BarData/*" >/tmp/bardata_product_ind19
  cat /tmp/bardata_product_ind19 | awk '{print $1}'| awk -F'/' '{print $5}' | awk -F':' '{print $1}' >/tmp/tmp_fileoutput
  mv /tmp/tmp_fileoutput /tmp/bardata_product_ind19
  bar_data_count=`cat /tmp/bardata_product_ind19 | wc -l`
  echo -e "Bardata: $bar_data_count\n"
  if [ $bar_data_count -lt 190 ] ; then
    echo  "$date $bar_data_count" >> $bar_file
    echo "<div style='margin:0 30px;'>" >>$mail_report ;
    echo -e "<h3>BarData Alert: </h3><h2>$bar_data_count</h2>\n" >> $mail_report
    echo "</div>" >>$mail_report ;
  fi

  products_file='/tmp/bar_data_products'
  >${products_file}
  tail -n +2 "/spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_${next_working_day}.csv" | awk  -F "," '{print $2}' | awk '{print $1}'  > ${products_file}
  comm -3 <(sort /tmp/bardata_product_ind19) <(sort ${products_file}) >/tmp/diff_products_missing_barcount
  echo "<div style='margin:0 30px;'>" >>$mail_report ;
  echo "<h3>Bar Data Not Generated for:</h3>" >>$mail_report
  echo "<table border='1' id='myTable' class='table table-striped' style='border: 1px solid black;border-collapse: collapse' ><thead><tr><th>Missing</th><th>Last Update</th></tr></thead><tbody>">> $mail_report
  for line in `cat /tmp/diff_products_missing_barcount`;
  do
    line=${line//&/\\&}
    time_tt=`ssh 10.23.227.69 "tail -1 /spare/local/BarData/$line"|awk '{print $1}'`
    echo $time_tt
    time_ttt=`date -d"@$time_tt"`
    echo $time_ttt
    echo -e "<tr><td style='border: 1px solid black' >$line</td><td style='border: 1px solid black' >$time_ttt</td></tr>\n" >> $mail_report ;
  done
  echo -e "</tbody></table>\n" >> $mail_report ;
  echo "</div>" >> $mail_report ;
  echo "</div>" >> $mail_report ;
#security N contract file size check
  security_size=`ssh 10.23.227.69 "du /spare/local/files/NSEFTPFiles/$yyyy/$mm/$dd/security" | awk '{print $1}'`
  contrat_size=`ssh 10.23.227.69 "du /spare/local/files/NSEFTPFiles/$yyyy/$mm/$dd/contract" | awk '{print $1}'`
  echo "sec, con=> $security_size, $contrat_size"
  [ "$security_size" == "0" ] && echo "<h3>Security File alert:</h3><h2> file size is 0</h2>">>$mail_report
  [ "$contract_size" == "0" ] && echo "<h3>Contract File alert:</h3><h2> file size is 0</h2>">>$mail_report

#fi

echo "#############>Position Limit Alert:"
  echo "<h3>Position Limit Alert:</h3>">>$mail_report
  echo "<table border='1' id='myTable' class='table table-striped' style='border-collapse: collapse'<thead><tr><th>Servers</th><th>Directory</th><th>PositionLimit.csv</th><th>PosLmtcsv_and_bkp_match</th></tr></thead><tbody>" >> $mail_report
#Position Limit
  position_limit_path="/home/dvctrader/ATHENA/"
  for server in "${!IND_Server_ip[@]}";
  do
    echo "for ---- $server";
    if [ "$server" == "IND15" ] || [ "$server" == "IND19" ] || [ "$server" == "IND20" ]; then
      echo "entered"
      ssh dvctrader@${IND_Server_ip[$server]} "crontab -l | grep -v '^#' | grep run">/tmp/crontab_files
      #ssh ${IND_Server_ip[$server]} "crontab -l | grep -v '^#' | grep run">/tmp/crontab_files #backup line
      files=`cat /tmp/crontab_files | awk '{print $6}'`
      echo -e "for $server\n"
      
      for file in $files;
      do
        echo -e " $file->\n"

      ssh dvctrader@${IND_Server_ip[$server]} "cat $file | grep -v '^#' | grep '/ \|CONFIG'"> /tmp/run_content
	    #ssh ${IND_Server_ip[$server]} "cat $file | grep -v '^#' | grep '/ \|CONFIG'"> /tmp/run_content #backup line

	config_dirs=`awk '{print $2}' /tmp/run_content | awk -F/ '{print $5}'| uniq`
        for config_dir in $config_dirs;
   	do
 	  echo "config==> $config_dir"
    pos_limit_date=`ssh dvctrader@${IND_Server_ip[$server]} "stat -c '%y' $position_limit_path$config_dir'/PositionLimits.csv'"| awk '{print $1}'`	
 	  #pos_limit_date=`ssh ${IND_Server_ip[$server]} "stat -c '%y' $position_limit_path$config_dir'/PositionLimits.csv'"| awk '{print $1}'`	#backup line
	  #echo "pos_date=> $pos_limit_date"
	  t2=`date --date="$pos_limit_date" +%s`
          
	  #echo "t2=> $t2"
          diff=$(( t2 - t1 ))
	  #echo "DIFF=> $diff****"
	  posLmt_csv_date=1

	  is_FUT1=0
	  Position_limit_bck_exist=0
 	  PosLmtcsv_and_bckup_match=0
          if [ $diff -lt 0 ]; then
	    echo "if diff" 
	    #echo "$date $server $file" >> $position_file
            posLmt_csv_date=0
	  #  echo "$server, $file (date is not Proper)" >> $mail_report
	  fi
  
         # PositionLimit_content=$date' '$server' '$file	  

	  if echo "$config_dir" | grep 'CONFIG_FUT1'; then
	    is_FUT1=1
            echo "^^^^^^^^^entered^^^^^^^^^^^^fut1"
      if ssh dvctrader@${IND_Server_ip[$server]} stat $position_limit_path$config_dir"/PosLimits_bkp" \> /dev/null 2\>\&1  
   	  #if ssh ${IND_Server_ip[$server]} stat $position_limit_path$config_dir"/PosLimits_bkp" \> /dev/null 2\>\&1  #backup line
	    then
        if ssh dvctrader@${IND_Server_ip[$server]} stat $position_limit_path$config_dir"/PosLimits_bkp/PositionLimits.$yesterday" \> /dev/null 2\>\&1 || ssh dvctrader@${IND_Server_ip[$server]} stat $position_limit_path$config_dir"/PosLimits_bkp/PositionLimits_agg.$yesterday" \> /dev/null 2\>\&1
        then 
          #
          if ssh dvctrader@${IND_Server_ip[$server]} stat $position_limit_path$config_dir"/PosLimits_bkp/PositionLimits.$yesterday" \> /dev/null 2\>\&1
          then
            Position_limit_bck_exist=1
            echo "file Exist.............................................................."
            comparison=`ssh dvctrader@${IND_Server_ip[$server]} "cmp $position_limit_path$config_dir/PositionLimits.csv $position_limit_path$config_dir/PosLimits_bkp/PositionLimits.$yesterday"`
          elif ssh dvctrader@${IND_Server_ip[$server]} stat $position_limit_path$config_dir"/PosLimits_bkp/PositionLimits_agg.$yesterday" \> /dev/null 2\>\&1
          then
            Position_limit_bck_exist=1
            echo "file Exist.............................................................."
            comparison=`ssh dvctrader@${IND_Server_ip[$server]} "cmp $position_limit_path$config_dir/PositionLimits.csv $position_limit_path$config_dir/PosLimits_bkp/PositionLimits_agg.$yesterday"`
          fi 
          #
	      #comparison=`ssh ${IND_Server_ip[$server]} "cmp $position_limit_path$config_dir/PositionLimits.csv $position_limit_path$config_dir/PosLimits_bkp/PositionLimits.$yesterday"` #backup line
	        echo "comp=> $comparison"
	        if [ "$comparison" != "" ]; then
      		  PosLmtcsv_and_bckup_match=0
	        else
	          PosLmtcsv_and_bckup_match=1
	        fi
	      else
	        Position_limit_bck_exist=0
	        PosLmtcsv_and_bckup_match=0
	      fi
      fi
	  fi
    PositionLimit_content=$yesterday' '$server' '$config_dir' '$posLmt_csv_date' '$is_FUT1' '$Position_limit_bck_exist' '$PosLmtcsv_and_bckup_match
	  echo -e "Pos-limitcontetn=> $PositionLimit_content\n"
	 # if [ $posLmt_csv_date -eq 0 ]; then
	  echo -e "^^^^^^^date not matching\n"

	  echo -e "<tr><td>$server</td><td>$config_dir</td>" >> $mail_report
	
#for position Limit.csv
    if [ $posLmt_csv_date -eq 1 ]; then
      
      echo -e "pos_limit_Present :: "  
      echo -e "<td>Present</td>" >> $mail_report 
    else
      echo -e "<td><b>Not Present</b></td>" >> $mail_report 
      echo -e "pos_limit_Not Present :: " 
    fi 
            
#for backup position Limit.csv
    if [ $PosLmtcsv_and_bckup_match -eq 1 ]; then
      echo -e "<td>Present</td></tr>" >> $mail_report
      echo -e "bck_Present\n" 
	  else
      echo -e "<td><b>Not Present</b></td></tr>" >> $mail_report 
      echo -e "bck_Not Present\n" 
    fi

	  echo $PositionLimit_content >> $position_file
	 done
  done 
 fi
done
  echo "</tbody></table></div></body></html>" >> $mail_report
  #cat /tmp/pos_bar_ratio_report.html | mail -a "Content-type: text/html" -s "IND Count Alert" hardik.dhakate@tworoads-trading.co.in ravi.parikh@tworoads-trading.co.in 

  (
#, ravi.parikh@tworoads.co.in, uttkarsh.sarraf@tworoads.co.in, nishit.bhandari@tworoads-trading.co.in
# echo "To: raghunandan.sharma@tworoads-trading.co.in"
 echo "To: raghunandan.sharma@tworoads-trading.co.in, ravi.parikh@tworoads.co.in, uttkarsh.sarraf@tworoads.co.in, nishit.bhandari@tworoads-trading.co.in, sanjeev.kumar@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in, tarun.joshi@tworoads-trading.co.in, naman.jain@tworoads-trading.co.in "
 echo "Subject: ***************IND SERVER DAILY COUNT ALERT:[ $yesterday ]***************"
 echo "Content-Type: text/html"
 echo
 cat /tmp/pos_bar_ratio_report.html
 echo
 ) | /usr/sbin/sendmail -t

#fi#test purpose

}

init $*
