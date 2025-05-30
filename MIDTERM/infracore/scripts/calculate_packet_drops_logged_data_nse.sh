#!/bin/bash

USAGE="$0 YYYYMMDD";
if [ $# -ne 1 ] ;
then
    echo $USAGE
    exit;
fi

YYYYMMDD=$1;
dd=${YYYYMMDD:6:2};
mm=${YYYYMMDD:4:2};
yy=${YYYYMMDD:2:2};
yyyy=${YYYYMMDD:0:4};

#path of bhavcopy and lotsize files
bhavcopy_file=/spare/local/tradeinfo/NSE_Files/BhavCopy/fo/$mm$yy/$dd$mm"fo_0000.md";
lotsize_file=/spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_$mm$yy.csv

#inner for loop: returns top 20 fut contracts based on traded lots in bhavcopy file
#inner for loop: also logs lotsize, expiry to help in lots calculation from logged data
#outer for loop: calculates num_lots traded in logged data & compares to bhavcopy calculations 

num_packet_drops=0;
for bhavcopy_stats in \
	$(for entry in $(egrep  "FUTIDX|FUTSTK" $bhavcopy_file | tr -d ' '| awk -F"," '{print $3"~"$4"~"$12}');
	do
		underlying=$(echo $entry | awk -F"~" '{print $1}');
		expiry=$(echo $entry | awk -F"~" '{print $2}');
		volume=$(echo $entry | awk -F"~" '{print "10#"$3}');
		expiry_yyyymmdd=$(date -d "$expiry" "+%Y%m%d");
		
		lotsize=$(grep "$underlying" $lotsize_file | tr -d ' '| awk -F"," -v underlying=$underlying '{if($2==underlying) print $3}');
		[ -n $lotsize -a $lotsize -gt 0 ] || continue;
		vol=$((volume/lotsize));
		echo "$underlying $expiry_yyyymmdd $lotsize $vol";
	done | sort -rnk4 | head -20 | tr ' ' '~' | tr -d ' ');
do
	underlying=`echo $bhavcopy_stats | awk -F"~" '{print $1}'`;
	expiry_yyyymmdd=`echo $bhavcopy_stats | awk -F"~" '{print $2}'`;
	lotsize=`echo $bhavcopy_stats | awk -F"~" '{print $3}'`;
	bhvcpy_lots=`echo $bhavcopy_stats | awk -F"~" '{print $4}'`;
	nas_logged_file=/NAS1/data/NSELoggedData/NSE/$yyyy/$mm/$dd/NSE_$underlying"_FUT_"$expiry_yyyymmdd"_"$YYYYMMDD".gz";
	if [ -f $nas_logged_file ]; then
		logged_data_lots=`/home/pengine/prod/live_execs/mds_log_reader NSE $nas_logged_file | grep "TradeQty:" | awk -v lotsize=$lotsize '{sum+=$2} END {print sum/lotsize}'`;
		allowed_packet_drops="0.95" #allowing 5% packet drops
		allowed_num_packets=`awk -v  bhvcpy_lots=$bhvcpy_lots -v allowed_packet_drops=$allowed_packet_drops 'BEGIN{print int(bhvcpy_lots*allowed_packet_drops)}'`;
		if [ $logged_data_lots -lt $allowed_num_packets ]; then
			num_packet_drops=$((num_packet_drops+1));
			echo "$underlying"_"$expiry_yyyymmdd $bhvcpy_lots $logged_data_lots";
		fi
	fi
done

if [ $num_packet_drops -eq 0 ]; then
	echo "NO DROPS";
fi

