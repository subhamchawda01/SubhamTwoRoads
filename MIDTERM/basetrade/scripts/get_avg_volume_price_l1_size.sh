
product=$1;
date=$2;
begin_time=$3;
end_time=$4;
temp_file=$5;

begin_time_unix=`~/infracore/scripts/get_unix_time.pl $date $begin_time`;
end_time_unix=`~/infracore/scripts/get_unix_time.pl $date $end_time`;
#temp_file=~/temp_file\_$product\_$date\_$begin_time\_$end_time;


>$temp_file

~/basetrade_install/bin/mkt_trade_logger SIM $product $date  | grep TradePrint | awk -v a=$begin_time_unix  '{ if ( $1>=a ) print $0 }' | awk -v b=$end_time_unix '{ if ( $1 <= $end_time_unix ) print $0 }' > $temp_file;
 

#if [[ -s $temp_file ]] ; then
#avg_l1_bid_size=`awk '{sum += $11;num +=1} END {print sum/num}' $temp_file`;
#avg_l1_ask_size=`awk '{sum += $14;num +=1} END {print sum/num}' $temp_file`;
#avg_volume=`awk '{sum += $5} END {print sum}' $temp_file`;
#avg_trade_size=`awk '{sum += $5; num+=1} END {print sum/num}' $temp_file`;
#avg_traded_price=`awk '{sum += $7;num +=1} END {print sum/num}' $temp_file`;
#
#echo $date $avg_volume $avg_traded_price $avg_trade_size $avg_l1_bid_size $avg_l1_ask_size;
#rm $temp_file;
#else
#x="";
#fi;
