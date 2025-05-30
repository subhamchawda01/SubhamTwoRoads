if [ $# -lt 8 ]; then echo "USAGE : $0 shc ilist pred_duration start_date num_days_lookback start_time end_time sample_msecs output_filename [threshold]"; exit; fi 

shc=$1;
ilist=$2; 
pred_duration=$3;
start_date=$4; 
num_days_lookback=$5;
start_time=$6
end_time=$7
sample_msecs=$8
output_filename=$9

base_px=`cat $ilist | head -n1 | awk '{print $4}'`;

rm -f $output_filename

zeropos_keep=-10.0;

if [ $# -gt 8 ]; 
then 
   zeropos_keep=${10}; 
   weights=($(cat $ilist | grep "INDICATOR " | cut -d ' ' -f 2 ))
fi

thresh_present=`echo $zeropos_keep | awk '{if ( $1 < 0 ) { print -1} else {print 1 } }'`;

work_dir="$HOME/cancel_labels";
mkdir -p $work_dir;
new_ilist="$work_dir/new_ilist";

cp $ilist $new_ilist;

bid_label_filename="$work_dir/bid_cancel_labels";
ask_label_filename="$work_dir/ask_cancel_labels";

bid_label_indicator="INDICATOR 1.00 CancelBidLabel $shc $pred_duration $bid_label_filename";
ask_label_indicator="INDICATOR 1.00 CancelAskLabel $shc $pred_duration $ask_label_filename";

if [ $thresh_present -gt 0 ]
then
	bid_price_indicator="INDICATOR 1.00 L1Price $shc BidPrice";
        ask_price_indicator="INDICATOR 1.00 L1Price $shc AskPrice";
        base_price_indicator="INDICATOR 1.00 L1Price $shc $base_px";
        sed -i "s;INDICATORSTART;INDICATORSTART\n$ask_price_indicator;g" $new_ilist;
        sed -i "s;INDICATORSTART;INDICATORSTART\n$bid_price_indicator;g" $new_ilist;
        sed -i "s;INDICATORSTART;INDICATORSTART\n$base_price_indicator;g" $new_ilist;
fi

trade_logger=~/basetrade_install/bin/mkt_trade_logger;
datagen=~/basetrade_install/bin/datagen

datagen_output_filename="$work_dir/dgen_out";

sed -i "s;INDICATORSTART;INDICATORSTART\n$ask_label_indicator;g" $new_ilist;
sed -i "s;INDICATORSTART;INDICATORSTART\n$bid_label_indicator;g" $new_ilist;

for date in `~/basetrade/scripts/get_list_of_dates_for_shortcode.pl $shc $start_date $num_days_lookback`;
do  
	$trade_logger SIM $shc $date 2>/dev/null | grep OnMarketUpdate | awk -v var=$date 'BEGIN{bp=0}{if($13 < bp) { cmd="~/basetrade_install/bin/get_mfm_from_utc_time "$1" "var; cmd | getline d; print d,-1; close(cmd) } if($13 > bp) { cmd="~/basetrade_install/bin/get_mfm_from_utc_time "$1" "var; cmd | getline d; print d,1; close(cmd) } bp=$13}' > $bid_label_filename;
        $trade_logger SIM $shc $date 2>/dev/null | grep OnMarketUpdate | awk -v var=$date 'BEGIN{ap=1000000000}{if($14 > ap) { cmd="~/basetrade_install/bin/get_mfm_from_utc_time "$1" "var; cmd | getline d; print d,1; close(cmd) } if($14 < ap) { cmd="~/basetrade_install/bin/get_mfm_from_utc_time "$1" "var; cmd | getline d; print d,-1; close(cmd) } ap=$14}' > $ask_label_filename;         
        $datagen $new_ilist $date $start_time $end_time 102134 $datagen_output_filename $sample_msecs 0 0 0 0;        

        if [ $thresh_present -lt 0 ];
        then
        	cut -f5- -d ' ' $datagen_output_filename  >> $output_filename      
        else
                min_price_increment=`~/basetrade_install/bin/get_min_price_increment $shc $date`;
                echo $min_price_increment
                echo $zeropos_keep
                zeropos_keep_norm=`echo $zeropos_keep  $min_price_increment | awk '{print $1*$2}'`;
                echo $zeropos_keep_norm
                cut -f5- -d ' ' $datagen_output_filename | awk -v wstr="${weights[*]}" 'BEGIN { split(wstr, weights, " ") } { for ( i=6; i < NF; i++ ) { $i=$i*weights[i-5]; sum+=$i }; print $1, $2, $3, $4, $5 sum }' | awk -v thresh=$zeropos_keep_norm '{ tgt_px=$3+$6; bid_px=$4; ask_px=$5; if ( tgt_px < bid_px + thresh ) { bid_down=1; } else { bid_down=0 }; if ( tgt_px > ask_px - thresh ) { ask_up=1 } else { ask_up=0 }; print $1,$2, bid_down, ask_up}' >> $output_filename;        
        fi
done

#rm -rf $work_dir;
