#!/bin/bash


declare -A server_to_base_id
server_to_base_id=( ["IND14"]="314" \
                    ["IND15"]="305" \
                    ["IND16"]="308" \
                    ["IND17"]="307" \
                    ["IND18"]="306" \
                    ["IND19"]="340" \
                    ["IND20"]="320" \
                    ["IND21"]="330" \
                    ["IND22"]="322" \
                    ["IND11"]="301" )


if [ $# -ne 2 ] ; then
  echo "USAGE: <SYMBOL(SBIN/SBIN_C0_O1)> <SERVER(IND15)>"
  exit
fi

[ ! ${server_to_base_id[$2]+abc} ] && echo "INVALID SERVER"
[ ! ${server_to_base_id[$2]+abc} ] && exit


symbol=$1
server=$2
today=`date +%Y%m%d`
nse_symbol="NSE_${symbol}"
exch_symbol_exec="/home/pengine/prod/live_execs/get_exchange_symbol"
ind11_reply_path="/spare/local/ORSBCAST/NSE"
ind21_reply_path="/spare/local/ORSBCAST_MULTISHM/NSE"
mds_log_reader_exec="/home/pengine/prod/live_execs/mds_log_reader"

exch_symbol=`$exch_symbol_exec $nse_symbol $today`
base_id=${server_to_base_id[$server]}
file_name="${exch_symbol}_${today}"

echo "$server $file_name"
if [ $server == "IND11" ] ; then 

  cd $ind11_reply_path
  if [ ! -f $file_name ] ; then 
    echo "NO LIVE ORDER : NO ORDER SENT YET"
    exit
  fi
  LD_PRELOAD=/home/dvcinfra/important/libcrypto.so.1.1 $mds_log_reader_exec ORS_REPLY_LIVE $file_name | awk -v base=$base_id '{if (rshift($14,16) == base) {saos_orr[$20]=$10; saos_saci[$20]=$14;} next;} END {count = 0; for (saos in saos_orr) {if (saos_orr[saos] == "Conf" || saos_orr[saos] == "CxRe") {print "HAS LIVE ORDER FOR SAOS: "saos,"SACI: "saos_saci[saos]; ++count;}} if ( count == "0" ) print "NO LIVE ORDER"}'
   
else

  cd $ind21_reply_path
  if [ ! -f $file_name ] ; then
    echo "NO LIVE ORDER : NO ORDER SENT YET"
    exit
  fi
  LD_PRELOAD=/home/dvcinfra/important/libcrypto.so.1.1 $mds_log_reader_exec ORS_REPLY_LIVE $file_name | awk -v base=$base_id '{if (rshift($10,16) == base) {saos_orr[$16]=$8; saos_saci[$16]=$10;} next;} END {count = 0; for (saos in saos_orr) {if (saos_orr[saos] == "Conf" || saos_orr[saos] == "CxRe") {print "HAS LIVE ORDER FOR SAOS: "saos,"SACI: "saos_saci[saos]; ++count;}} if ( count == "0" ) print "NO LIVE ORDER"}'

fi

