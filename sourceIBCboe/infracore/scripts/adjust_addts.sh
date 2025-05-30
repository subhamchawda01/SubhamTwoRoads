rm -rf /tmp/cumulative_limits
rm -rf /tmp/addts_file
rm -rf /tmp/limits
for script in `crontab -l | grep -v '^#' | grep run | awk '{if(NF == 6) print}' | awk '{print $NF}'`;
do 
  for pos_file in `cat $script | grep -v "#" | grep CONFIG | awk '{print $2}'   | awk -F "/" '{print $1"/"$2"/"$3"/"$4"/"$5"/PositionLimits.csv"}' | sort | uniq`;
  do
    multiplier_=1;
    if [ `grep MAX_SIZE_MULTIPLIER $pos_file | awk '{if($3>1) print $3}' | head -1 | wc -l` == "1" ]; then
      multiplier_=`grep MAX_SIZE_MULTIPLIER $pos_file | awk '{print $3}' | head -1`;
    fi

    cat $pos_file | grep NSE_ | awk -F  "_" '{print $1"_"$2" "$3}' | awk '{print $1" "$NF}' |awk -v mul=${multiplier_} '{if(a[$1] < $2){a[$1]=$2}} END{ for(i in a)print i,a[i]*mul }' >> /tmp/limits
  done
done

cat /tmp/limits | awk '{a[$1] += $2} END {for(i in a) print i, a[i]}' > /tmp/cumulative_limits
host_name=`hostname`
profile=`tail -n1 /home/pengine/prod/live_configs/${host_name}_addts.cfg | awk '{print $2}'`

while IFS= read -r line
do 
   symbol=`echo $line | awk '{print $1}'`
   limit=`echo $line | awk '{print $2}'`;
   ors_current_limit=`grep -w "$symbol" /home/pengine/prod/live_configs/${host_name}_addts.cfg | tail -n1 | awk '{print $5}'`
   if [ $limit -eq 0 ];
   then
	continue;
   fi
   if [ -z $ors_current_limit  ] || [ $ors_current_limit -lt $limit ];
   then
	echo "NSE ${profile} ADDTRADINGSYMBOL \"${symbol}\" ${limit} ${limit} ${limit} $(( 2 * limit))" >> /tmp/addts_file
   fi
done < /tmp/cumulative_limits
grep -v NSE_YESBANK /tmp/addts_file > temp_addts
mv temp_addts /tmp/addts_file

if [ -f /tmp/addts_file ];
then
    /home/pengine/prod/live_scripts/ADDTRADINGSYMBOL.sh /tmp/addts_file
fi
