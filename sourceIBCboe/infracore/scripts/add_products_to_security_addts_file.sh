declare -A IND_Server_ip
IND_Server_ip=( ["IND14"]="10.23.227.64" \
                ["IND15"]="10.23.227.65" \
                ["IND16"]="10.23.227.81" \
                ["IND17"]="10.23.227.82" \
                ["IND18"]="10.23.227.83" \
                ["IND19"]="10.23.227.69" \
                ["IND20"]="10.23.227.84" \
                ["IND21"]="10.23.227.66" \
                ["IND22"]="10.23.227.71" \
                ["IND23"]="10.23.227.72" \
                ["IND24"]="10.23.227.74" )

declare -A server_to_profile_
server_to_profile_=( ["IND14"]="MSFO7" \
                     ["IND15"]="MSFO4" \
                     ["IND16"]="MSEQ2" \
                     ["IND17"]="MSEQ3" \
                     ["IND18"]="MSEQ4" \
                     ["IND19"]="MSFO5" \
                     ["IND20"]="MSFO6" \
                     ["IND21"]="MSEQ7" \
                     ["IND22"]="MSEQ9" \
                     ["IND23"]="MSEQ6" \
                     ["IND24"]="MSEQ10" )

declare -A server_to_hostname
server_to_hostname=( ["IND14"]="sdv-ind-srv14" \
                   ["IND15"]="sdv-ind-srv15" \
                   ["IND16"]="sdv-ind-srv16" \
                   ["IND17"]="sdv-ind-srv17" \
                   ["IND18"]="sdv-ind-srv18" \
                   ["IND19"]="sdv-ind-srv19" \
                   ["IND20"]="sdv-ind-srv20" \
                   ["IND21"]="sdv-ind-srv21" \
                   ["IND22"]="sdv-ind-srv22" \
                   ["IND23"]="sdv-ind-srv23" \
                   ["IND24"]="sdv-ind-srv24" )

send_mail(){
 echo "Sending mail"
 echo "" | mailx -s "$1" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in < $new_ipo_product_mail
}

print_msg_and_exit (){
   echo $1;
    exit
}

check_and_insert_CM () {
  for shortcode in "${@}"
  do
    echo "$shortcode"
    [ `grep -w $shortcode $security_file | wc -l` -eq 0 ] && echo "$shortcode 30" >> $security_file
    [ `ssh pengine@$ip_add "grep -w $shortcode ${default_addts_file_path}${hostname}_addts.cfg | wc -l "` -eq 0 ] && echo "$shortcode" >> /tmp/${server}_new_product && ssh pengine@$ip_add "echo "NSE $profiler ADDTRADINGSYMBOL $shortcode 10000 10000 10000 20000" >> ${default_addts_file_path}${hostname}_addts.cfg"
  done
}

check_and_insert_FO_FUT () {
  for shortcode in "${@}"
  do
    echo "$shortcode"
    [ `grep -w $shortcode $security_file | wc -l` -eq 0 ] && echo "$shortcode 30" >> $security_file
    [ `grep -w $shortcode $security_file | wc -l` -eq 0 ] && echo "$shortcode 30" >> $security_file
    [ `ssh pengine@$ip_add "grep -w $shortcode ${default_addts_file_path}${hostname}_addts.cfg | wc -l "` -eq 0 ] && echo "$shortcode" >> /tmp/${server}_new_product && ssh pengine@$ip_add "echo "NSE $profiler ADDTRADINGSYMBOL $shortcode 100 100 100 100" >> ${default_addts_file_path}${hostname}_addts.cfg"
  done
}

check_and_insert_FO_OPT () {
  for shortcode in "${@}"
  do
    echo "$shortcode"
    [ `grep -w $shortcode $security_file | wc -l` -eq 0 ] && echo "$shortcode 30" >> $security_file
    [ `grep -w $shortcode $security_file | wc -l` -eq 0 ] && echo "$shortcode 30" >> $security_file
    [ `ssh pengine@$ip_add "grep -w $shortcode ${default_addts_file_path}${hostname}_addts.cfg | wc -l "` -eq 0 ] && echo "$shortcode" >> /tmp/${server}_new_product && ssh pengine@$ip_add "echo "NSE $profiler ADDTRADINGSYMBOL $shortcode 20 20 20 20" >> ${default_addts_file_path}${hostname}_addts.cfg"
  done
}

add_product_server () {
    for server in "${@:3}"
    do
      echo "$server"
      ip_add=${IND_Server_ip[$server]}
      profiler=${server_to_profile_[$server]}
      hostname=${server_to_hostname[$server]}
      if [ "$server" == "IND16" ] || [ "$server" == "IND17" ] || [ "$server" == "IND18" ] || [ "$server" == "IND23" ] || [ "$server" == "IND24" ]; then
        symbol="NSE_${product}"
        check_and_insert_CM $symbol

      elif [ "$server" == "IND15" ] || [ "$server" == "IND19" ]; then
        fut0="NSE_${product}_FUT0"
        fut1="NSE_${product}_FUT1"
        fut2="NSE_${product}_FUT2"
        check_and_insert_FO_FUT $fut0 $fut1 $fut2

      elif [ "$server" == "IND14" ] || [ "$server" == "IND20" ]; then
        P0_A="NSE_${product}_P0_A"
        P0_O1="NSE_${product}_P0_O1"
        P0_O2="NSE_${product}_P0_O2"
        P0_O3="NSE_${product}_P0_O3"
        FUT0="NSE_${product}_FUT0"
        C0_A="NSE_${product}_C0_A"
        C0_O1="NSE_${product}_C0_O1"
        C0_O2="NSE_${product}_C0_O2"
        C0_O3="NSE_${product}_C0_O3"
        check_and_insert_FO_OPT $P0_A $P0_O1 $P0_O2 $P0_O3 $FUT0 $C0_A $C0_O1 $C0_O2 $C0_O3
      else
        echo "INVALID SERVER: $server"
      fi
    done
}

[ $# -ge 3 ] || print_msg_and_exit "Usage : < script > <yyyymmdd> <CIRCULAR/new_products_file> <servers>"

date=$1
next_working_date=`/home/pengine/prod/live_execs/update_date $date N W`
circular_product_file=$2
security_file="/spare/local/tradeinfo/NSE_Files/SecuritiesMarginFiles/security_margin_${next_working_date}.txt"
#security_file="/tmp/security_margin_${next_working_date}.txt"
default_addts_file_path="/home/pengine/prod/live_configs/"
#default_addts_file_path="/tmp/"
#circular_file_="/tmp/action_to_consider.txt"
circular_file_="/spare/local/files/NSE/action_to_consider.txt"
#security_file="/tmp/security_margin_${date}.txt"
new_ipo_product_mail="/tmp/new_ipo_product_mail"
> $new_ipo_product_mail

echo "DATE: $date"
echo "NEXT_WORKING_DATE: $next_working_date"

[ ! -f $security_file ] && send_mail "SECUTIRY MARGIN FILE NOT PRESENT FOR $next_working_date" && print_msg_and_exit "$security_file file not present"

for server in "${@:3}"
do
  > /tmp/${server}_new_product
done

if [ "$circular_product_file" == "CIRCULAR" ]; then
  scp dvcinfra@10.23.5.67:/spare/local/files/NSE/action_to_consider.txt dvcinfra@10.23.5.26:/spare/local/files/NSE/action_to_consider.txt
  [ `grep $next_working_date $circular_file_ | grep "Listing" | wc -l` -eq 0 ] && print_msg_and_exit "NO NEW PRODUCT TO ADD FOR $next_working_date"
  for product in `grep $next_working_date $circular_file_ | cut -d' ' -f7` 
  do
    echo "$product"
    add_product_server $*
  done
else
  add_server_to_mail
  for product in `cat $circular_product_file`
  do
    echo "$product"
    add_product_server $*
  done
fi

for server in "${@:3}"
do
  echo "$server: " >> $new_ipo_product_mail 
  cat /tmp/${server}_new_product >> $new_ipo_product_mail
  echo -e "\n" >> $new_ipo_product_mail
done
send_mail "LIST OF NEW PRODUCT ADDED TO SECURITY AND ADDTS FILE $next_working_date"

/home/dvctrader/important/onexpiry/sync_trade_info.sh

