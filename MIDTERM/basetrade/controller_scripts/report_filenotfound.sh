#!/bin/bash

USAGE="$0 base_dir_to_search YYYYMMDD";

if [ $# -lt 2 ] ;
then
    echo $USAGE;
    exit;
fi

base_dir=$1;
YYYYMMDD=$2;
if [ $YYYYMMDD = "TODAY" ] ; then                                                                                                                                                                           
    YYYYMMDD=$(date "+%Y%m%d");                                                                                                                                                                               
fi
if [ $YYYYMMDD = "TODAY-1" ] ; then                                                                                                                                                                           
    YYYYMMDD=`date --date=yesterday +%Y%m%d`;                                                                                                                                                                      
fi

yyyy=${YYYYMMDD:0:4}; mm=${YYYYMMDD:4:2}; dd=${YYYYMMDD:6:2};
uid=`date +%N`;
notfound_tmpfile="$HOME/notfound_tmpfile_"$uid;

#ip_list=`cat ~/AWSScheduler/instance_cores.txt | awk '{if($2>0){ print $1; }}'`;
ip_list=`cat /mnt/sdf/JOBS/all_instances.txt | grep nil | awk '{print $4}'`;
ip=`cat ~/AWSScheduler/instance_cores.txt | awk '{if($2>0){ print $1; }}' | head -n1`;

ge0=`ssh $ip "~/basetrade_install/bin/get_exchange_symbol GE_0 $YYYYMMDD" 2>/dev/null`;
eod_file=/apps/data/MFGlobalTrades/EODPnl/ors_pnls_"$YYYYMMDD".txt;
if [ -s $eod_file ]; then
	symbol_list=`grep LPX $eod_file | cut -d"|" -f2 | awk '{if($2){print $1, $2} else{print $1}}' | tr " " "~"`;
else
	shc_list=`cat /spare/local/tradeinfo/curr_trade_prod_list.txt`;
	symbol_list=`for shc in $shc_list; do ssh $ip "~/basetrade_install/bin/get_exchange_symbol $shc $YYYYMMDD" | tr " " "~"; echo ""; done 2>/dev/null`;
fi;
symbol_list=`echo $symbol_list | tr "\n" " "`;

for ip in $ip_list; do
echo $ip >> $notfound_tmpfile;
ssh $ip "for symbol in $symbol_list; do ls -lrt $base_dir/NAS1/data/ORSData/*/$yyyy/$mm/$dd/\$symbol'_'$YYYYMMDD; done | grep -v 'ORSData/NY4'; ls -lrt $base_dir/NAS1/data/*/*/$yyyy/$mm/$dd/* | grep -v 'QUINCY\|CHIX_L1LoggedData\|ESPEEDLoggedData\|TSELoggedData\|ORSData\|ASXLoggedData\|ASXPFLoggedData' | grep -v 'EUREXLoggedData/CFE' | grep -v 'CSMLoggedData/FR2' | grep -v 'TMX_FSLoggedData/FR2' | grep -v 'HKEXLoggedData/CHI' | grep -v 'CMELoggedData/MOS/$yyyy/$mm/$dd/GE' | grep -v 'LIFFELoggedData/HK' | grep -v 'LIFFELoggedData/TOK' | grep -v 'ICELoggedData/HK' | grep -v 'ICELoggedData/TOK' | grep -v 'ICELoggedData/CFE' | grep -v 'EOBIPriceFeedLoggedData/BRZ' | grep -v 'LIFFELoggedData/BSL/$yyyy/$mm/$dd/XF*'; if [ ! -z '$ge0' ]; then ls -lrt $base_dir/NAS1/data/CMELoggedData/MOS/$yyyy/$mm/$dd/$ge0* ; fi;" >> $notfound_tmpfile 2>/dev/null;
done; 


if [ -s $notfound_tmpfile ] ; then       
	/bin/mail -s "FileNotFoundAlert - for $YYYYMMDD in $base_dir" -r "nseall@tworoads.co.in" "nseall@tworoads.co.in" < $notfound_tmpfile ;
	#/bin/mail -s "FileNotFoundAlert - for $YYYYMMDD in $base_dir" -r "archit@circulumvite.com" "archit@circulumvite.com" < $notfound_tmpfile ;
fi

rm -rf $notfound_tmpfile ; 
