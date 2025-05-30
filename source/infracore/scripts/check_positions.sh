#!/bin/bash
temp_file="/tmp/filecron"
temp1="/tmp/temp1PositionMissing"
temp2="/tmp/temp2PositionMissing"
temp3="/tmp/temp3PositionMissing"
temp4="/tmp/temp4PositionMissing"
res="/tmp/resultPositionMissing"
mail_report="/tmp/position_missing.html"
true>$res
true>$temp_file
true>$mail_report

servers="IND11 IND12 IND13 IND14 IND15 IND16 IND17 IND18 IND19 IND20"
  declare -A IND_Server_ip
  IND_Server_ip=( ["IND11"]="10.23.227.61" \
                ["IND12"]="10.23.227.62" \
                ["IND13"]="10.23.227.63" \
                ["IND14"]="10.23.227.64" \
                ["IND15"]="10.23.227.65" \
                ["IND16"]="10.23.227.81" \
                ["IND17"]="10.23.227.82" \
                ["IND18"]="10.23.227.83" \
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

  echo "<h2>Missing Positions<h2>"  >>$mail_report
  echo "<table id='myTable' class='table table-striped' style='font-size:12px'><thead><tr style='font-size:15px'><th>Server</th><th>CSV File</th><th>Missing</th></tr></thead><tbody>">> $mail_report
for server in "${!IND_Server_ip[@]}"; do
#for server in "IND19"; do
    if [ "$server" == "IND11" ] || [ "$server" == "IND12" ] || [ "$server" == "IND13" ] || [ "$server" == "IND14" ] ; then
      echo -e "$server contiue\n"
      continue;
    fi
    echo $server >>$res
    echo $server
    echo ${IND_Server_ip[$server]}
   
    ssh ${IND_Server_ip[$server]} 'crontab -l|grep -E "^[^#].*/run"|cut -d" " -f 6' > $temp_file
    
#while IFS=$'\n' read -r cron_file
    IFS=$'\n'
    for cron_file in `cat $temp_file`; 
#for line in "/home/dvctrader/ATHENA/run_ind13.sh" "/home/dvctrader/ATHENA/run_midterm.sh";
    do
       echo "script file: "$cron_file; 
       files_csv=($(ssh ${IND_Server_ip[$server]} cat $cron_file|grep -E "^[^#]"| cut -d " " -f2 ))
# continue;
       for file_csv in "${files_csv[@]}"; do
       if [[ ! $file_csv == *.csv ]] ;  then
       echo "Unknown CSV file:"$file_csv;
       continue;
       fi
          true>$temp1;
          true>$temp2;
          echo "CSV File   :"$file_csv
          if ssh ${IND_Server_ip[$server]} stat $file_csv \> /dev/null 2\>\&1 ;  then
            id=($(ssh ${IND_Server_ip[$server]} cat $file_csv|cut -d " " -f3))
            for i in "${id[@]}"; do
            if [ `echo $i|cut -d'_' -f1` = "NSE" ] ; then
             echo $i| cut -d'_' -f 2 >>$temp1     
#echo  $i| cut -d'_' -f 2   
            elif [ `echo $i|cut -d'_' -f1` = "HDG" ] ; then
             echo $i| cut -d'_' -f 3 >>$temp1
#echo $i| cut -d'_' -f 3
            fi
            done
            pos=`dirname "$file_csv"`"/PositionLimits.csv"
            echo "Position File "$pos
            if ssh ${IND_Server_ip[$server]} stat $pos  \> /dev/null 2\>\&1 ;  then
            id=($(ssh ${IND_Server_ip[$server]} cat $pos|cut -d " " -f1))
            for i in "${id[@]}"; do
             echo $i| cut -d'_' -f 2 >>$temp2
#echo $i| cut -d'_' -f 2
            done
            fi
            echo `sort -u $temp1` > $temp1
            echo `sort -u $temp2` > $temp2
            tr ' ' '\n' <$temp1 >$temp3
            tr ' ' '\n'  <$temp2 >$temp4
            echo $file_csv >>$res
            echo `comm -23 $temp3 $temp4` >>$res
            declare -a MissingFile 
            MissingFile=($(comm -23 $temp3 $temp4))
            table="<table id='myTable' class='table table-striped' style='font-size:12px; border: 1px solid black; border-collapse:collapse;table-layout:fixed'>"
            echo $MissingFile
            for (( i=0; i<${#MissingFile[@]}; i++ )); 
            do 
             modval=$(($i%6))
              if [[ $modval = 0 ]] ; then
              table=$table"<tr>"
              fi
              table=$table"<td  style='font-size:12px; border: 1px solid black; min-width:90px'>${MissingFile[$i]}</td>"
              echo ${MissingFile[$i]}
              if [[ $modval = 5 ]] ; then
              table=$table"</tr>"
              fi
            done
            if [[ $table == *d\> ]] ; then
            table=$table"</tr>"
            fi
            table=$table"</table>"
            echo $table
#exit 0
             if [ `comm -23 $temp3 $temp4|wc -l` -gt 0 ] ; then
                  echo -e "<tr><td style='text-align:center'>$server</td><td style='text-align:center'>$file_csv</td><td style='text-align:center'>$table</td></tr>\n" >> $mail_report ;
             fi
           fi
       done  #end of csv files
  done  #cron file
done #server
echo -e "</tbody></table>\n" >> $mail_report ;

(
#, ravi.parikh@tworoads.co.in, uttkarsh.sarraf@tworoads.co.in, nishit.bhandari@tworoads-trading.co.in
 echo "To: raghunandan.sharma@tworoads-trading.co.in, hardik.dhakate@tworoads-trading.co.in, ravi.parikh@tworoads.co.in, uttkarsh.sarraf@tworoads.co.in, nishit.bhandari@tworoads-trading.co.in"
 echo "Subject: *************** IND SERVER Position Missing ALERT:***************"
 echo "Content-Type: text/html"
 echo
 cat /tmp/position_missing.html
 echo
 ) | /usr/sbin/sendmail -t

