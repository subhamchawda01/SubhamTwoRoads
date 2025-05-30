#!/bin/bash
if [ $# -ne 1 ] ; then 
  echo "USAGE: <script> <instrument(ICICIBANK_CE_440.00_0)>"
  exit
fi

date_=`date +\%Y\%m\%d`
date_=20201009
tradeinfo_dir='/spare/local/tradeinfo/NSE_Files/'
GET_EXCHANGE_SYM="/home/pengine/prod/live_execs/get_exchange_symbol"
get_shortcode_sym_exec="/home/pengine/prod/live_execs/get_shortcode_for_symbol"
DATAEXCH_SYM_FILE="/spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt"
MANUAL_ORDER_FILE="/spare/local/files/nse_strategy_orders_manual"
MIDTERM_ORDER_FILE="/spare/local/files/nse_strategy_orders_midterm"
DISP_ORDER_FILE="/spare/local/files/nse_strategy_orders_disp"
ORS_TRADE_FILE="/spare/local/ORSlogs/NSE_FO/MSFO/trades.${date_}"
TRADE_EXEC_PATH="/spare/local/logs/alllogs/MediumTerm/"
TRADE_FILE="/home/dvctrader/important/MANUAL_TRADE/New_ExecutionTradesMatching"
LOTSIZE_FILE="/spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_${date_}.csv"
DATAEXCH_SYM_FILE="/spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt"

GetNearestExpiry() {
      contract_file=${tradeinfo_dir}'/ContractFiles/nse_contracts.'${date_}
      expiry=`cat ${contract_file} | grep IDXFUT | grep BANKNIFTY | awk -v date=${date_} '{if($NF>=date)print $NF'} | sort | uniq | awk -v exp_no=${exp_no_} '{if(NR==exp_no+1) print $0}' | head -n1`
}


YYYYMMDD=`date +\%Y\%m\%d`
time_in_sec_=`date +%s`
GetNearestExpiry

instrument=$1
underlying=`echo $instrument | sed 's/NSE_//g' | cut -d_ -f1`
exp_no_=`echo $instrument | awk '{print substr($(NF),length($(NF)))}'`
if [ `echo $instrument | egrep "_CE_|_PE_" | wc -l` -eq "1" ]; then
    echo "      OPTION"
    data_src_entry_=`echo $instrument | awk -v exp_date_=${expiry} -F_ '{print "NSE_"$1"_"$2"_"exp_date_"_"$3}'`
  elif [ `echo $instrument | egrep "_FUT" | wc -l` -eq "1" ]; then
    echo "      FUT"
    data_src_entry_=`echo $instrument | awk -v exp_date_=${expiry} -F_ '{print $1"_"$2"_FUT_"exp_date_}'`
  else
    echo "      EQUITY"
fi
echo "entry in SymbolFile    $data_src_entry_"
exch_symbol_=`grep -w $data_src_entry_ $DATAEXCH_SYM_FILE | awk '{print $1}'`
echo "inst: $instrument exch_sym:  $exch_symbol_   data_src_entry: $data_src_entry_ $DATAEXCH_SYM_FILE"

if [ `echo $instrument | egrep "_CE_|_PE_" | wc -l` -eq "1" ]; then
    echo "OPTIONS SHORTCODE"
    shortcode_=`LD_PRELOAD=/home/dvctrader/important/libcrypto.so.1.1 $get_shortcode_sym_exec $exch_symbol_ $date_`
else
    shortcode_=${instrument}
fi
echo "Shortcode_= $shortcode_"
lot_size_=`grep $underlying $LOTSIZE_FILE | awk -F, -v exp_no=${exp_no_} -F, '{if(exp_no==0){print $3} else if(exp_no==1){print $4} else {print $5}}'`

trade_exec_buy=`grep $instrument /spare/local/logs/alllogs/MediumTerm/*trades_${date_}.dat | grep BUY | wc -l`
trade_exec_sell=`grep $instrument /spare/local/logs/alllogs/MediumTerm/*trades_${date_}.dat | grep SELL | wc -l`

trade_ors_buy=`cat /spare/local/ORSlogs/NSE_FO/MSFO/trades.${date_} | tr '\001' ' ' | grep $exch_symbol_ | awk 'BEGIN{sum=0} {if($2 == 0) sum+=1;} END{print sum}'`
trade_ors_sell=`cat /spare/local/ORSlogs/NSE_FO/MSFO/trades.${date_} | tr '\001' ' ' | grep $exch_symbol_ | awk 'BEGIN{sum=0} {if($2 == 1) sum+=1;} END{print sum}'`

echo "grep $instrument /spare/local/logs/alllogs/MediumTerm/*trades_${date_}.dat	"
echo "Trades BUY SELL"
echo "ORS $trade_ors_buy $trade_ors_sell"
echo "Exec $trade_exec_buy $trade_exec_sell"
price_=`grep $shortcode_ /spare/local/logs/alllogs/MediumTerm/*trades_* |tail -1 | awk '{print $5}'`
total_comm_=`grep $shortcode_ /spare/local/logs/alllogs/MediumTerm/*trades_* |tail -1 | awk '{print $6}'`
ORDER_ID=''
tranch_no=''
TYPE_=''
file_add_order=''

echo "$price_"
echo "$lot_size_"

if [[ $trade_exec_buy -ne $trade_ors_buy ]];then
	echo "Buy Miss in Buy"
	TYPE_="BUY"
	for order_id in `grep $instrument $MANUAL_ORDER_FILE $MIDTERM_ORDER_FILE $DISP_ORDER_FILE | grep -v "%-" | cut -d':' -f2 | awk '{print $1}'`; do
		order_count=`grep $instrument $MANUAL_ORDER_FILE $MIDTERM_ORDER_FILE $DISP_ORDER_FILE | grep $order_id | cut -d'%' -f4 | cut -d'.' -f1 | awk 'function abs(lot) {return (lot < 0 ? -lot : lot)} {print abs($1)}'`
		trade_count=`grep $instrument /spare/local/logs/alllogs/MediumTerm/*trades_${date_}.dat | grep BUY | grep $order_id | wc -l`
		echo "orderID: $order_id order: $order_count  trade:  $trade_count"
		if [[ $order_count -ne $trade_count ]];then
			echo "ORDER ID FOUND"
			ORDER_ID=$order_id
			tranch_no=`grep $instrument $MANUAL_ORDER_FILE $MIDTERM_ORDER_FILE $DISP_ORDER_FILE| grep $order_id | awk '{print $4}'`
			file_add_order=`grep $instrument $MANUAL_ORDER_FILE $MIDTERM_ORDER_FILE $DISP_ORDER_FILE| grep $order_id | cut -d':' -f1 | rev | cut -d'_' -f1 | rev`
			
			break;
		fi
	done
	echo		
	echo -e "${ORDER_ID}_${tranch_no}_${instrument}\t${shortcode_}\t${TYPE_}\t${lot_size_}\t${price_}\t${total_comm_} ${time_in_sec_}\tPASS"
	echo "ADD to trade file /spare/local/logs/alllogs/MediumTerm/${file_add_order}_execlogic_trades_${date_}.dat"

fi




if [[ $trade_exec_sell -ne $trade_ors_sell ]];then
	echo "Trade Miss in Sell"
	TYPE_="SELL"
	for order_id in `grep $instrument $MANUAL_ORDER_FILE $MIDTERM_ORDER_FILE $DISP_ORDER_FILE | grep "%-" |cut -d':' -f2 | awk '{print $1}'`; do
                order_count=`grep $instrument $MANUAL_ORDER_FILE $MIDTERM_ORDER_FILE $DISP_ORDER_FILE | grep $order_id |   cut -d'%' -f4 | cut -d'.' -f1 | awk 'function abs(lot) {return (lot < 0 ? -lot : lot)} {print abs($1)}'`
		trade_count=`grep $instrument /spare/local/logs/alllogs/MediumTerm/*trades_${date_}.dat | grep SELL | grep $order_id | wc -l`
		echo "orderID: $order_id order: $order_count  trade:  $trade_count"
		if [[ $order_count -ne $trade_count ]];then
			echo "ORDER ID FOUND"
                        ORDER_ID=$order_id
                        tranch_no=`grep $instrument $MANUAL_ORDER_FILE $MIDTERM_ORDER_FILE $DISP_ORDER_FILE| grep $order_id | awk '{print $4}'`
			file_add_order=`grep $instrument $MANUAL_ORDER_FILE $MIDTERM_ORDER_FILE $DISP_ORDER_FILE| grep $order_id | cut -d':' -f1 | rev | cut -d'_' -f1 | rev`

                        break;
                fi

	done
	echo
	echo -e "${ORDER_ID}_${tranch_no}_${instrument}\t${shortcode_}\t${TYPE_}\t${lot_size_}\t${price_}\t${total_comm_} ${time_in_sec_}\tPASS"
	echo "ADD to trade file /spare/local/logs/alllogs/MediumTerm/${file_add_order}_execlogic_trades_${date_}.dat"
fi

echo
echo "END..."
exit
