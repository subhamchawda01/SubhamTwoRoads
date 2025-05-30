#!/bin/bash

Print_Msg_and_Exit(){
  echo $* ;
  exit ;
}

GetNearestExpiry() {
      contract_file=${tradeinfo_dir}'/ContractFiles/nse_contracts.'${date_}
      expiry=`cat ${contract_file} | grep IDXFUT | grep BANKNIFTY | awk -v date=${date_} '{if($NF>=date)print $NF'} | sort | uniq | awk -v exp_no=${exp_no_} '{if(NR==exp_no+1) print $0}' | head -n1`
}


[ $# -eq 3 ] || Print_Msg_and_Exit "Usage : < script > < date > < num > < START_VALUE for Order_id > <tranch_no>" ;
export LD_LIBRARY_PATH=/opt/glibc-2.14/lib ;
date_=$1
start_value_=$2
tranch_no=$3
time_in_sec_=`date -d$date_ +%s`
#Trade_FILE="/tmp/ExecutionTradesMatching"
tradeinfo_dir='/spare/local/tradeinfo/NSE_Files/'
Trade_FILE="/home/dvctrader/important/MANUAL_TRADE/New_ExecutionTradesMatching"
#Trade_FILE="/home/dvctrader/important/MANUAL_TRADE/ExecutionTradesMatching_bkp"
LOTSIZE_FILE="/spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_`date +%Y%m%d`.csv"
DATAEXCH_SYM_FILE="/spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt"
ORS_TRADE_FILE="/spare/local/ORSlogs/NSE_FO/MSFO/trades.${date_}"
get_shortcode_sym_exec="/home/dvctrader/important/MANUAL_TRADE/get_shortcode_for_symbol"

#/home/dvctrader/anaconda3/bin/python /home/dvctrader/important/MANUAL_TRADE/Intraday_trades_match.py $date

line_num=`grep -n "TRADES MISMATCH" $Trade_FILE | tail -1 | cut -d: -f1` ;
ORDER_ID_STRT_=`date +%s`


#date_=`date +%Y%m%d`

echo $date_
echo "line_num: $line_num"
#for line in `awk -v line_no=$line_num '{if(NR>line_no) print $0}' $Trade_FILE | egrep "_PE_|_CE_|_FUT" | awk -F"|" '{gsub(/ /, "", $0); printf "%s,%s\n",$2,$5}'`; 
for line in `awk -v line_no=$line_num '{if(NR>line_no) print $0}' $Trade_FILE | egrep "_PE_|_CE_|_FUT" | awk -F"|" '{gsub(/ /, "", $0); if($5>0){type="BUY"; } else {type="SELL"; } printf "%s,%s,%s\n",$2,$5,type}'`;
do
  echo "LINE: $line"
  instrument=`echo $line | cut -d, -f1`
  exp_no_=`echo $instrument | awk '{print substr($(NF),length($(NF)))}'`
  underlying=`echo $line | sed 's/NSE_//g' | cut -d_ -f1`
  no_of_lots=`echo $line | cut -d, -f2` # | awk '{if($1<0) $1=$1*-1; print $1}'`
  TYPE_=`echo $line | cut -d, -f3`
  expiry=$date_
  echo "inst: $instrument exp: $exp_no_ :${underlying}_IND"
  GetNearestExpiry

  get_sign_=""
  if [ "${TYPE_}" == "SELL" ]; then
    get_sign_="-" 
    echo "SELL trade****"
  fi
  if [ `echo $line | egrep "_CE_|_PE_" | wc -l` -eq "1" ]; then
    echo "      OPTION"
    comm_=0.0011538;
    data_src_entry_=`echo $instrument | awk -v exp_date_=${expiry} -F_ '{print "NSE_"$1"_"$2"_"exp_date_"_"$3}'`
  elif [ `echo $instrument | egrep "_FUT" | wc -l` -eq "1" ]; then
    echo "      FUTURE"
    comm_=0.00008727;
    data_src_entry_=`echo $instrument | awk -v exp_date_=${expiry} -F_ '{print $1"_"$2"_FUT_"exp_date_}'`
  else 
    echo "      EQUITY"
    comm_=0.00017905;
  fi
  
  exch_symbol_=`grep -w $data_src_entry_ $DATAEXCH_SYM_FILE | awk '{print $1}'`
  echo "inst: $instrument, exch_sym: $exch_symbol_, data_src_entry: $data_src_entry_, $DATAEXCH_SYM_FILE"

  if [ `echo $line | egrep "_CE_|_PE_" | wc -l` -eq "1" ]; then
    shortcode_=`LD_PRELOAD=/home/dvctrader/important/libcrypto.so.1.1 $get_shortcode_sym_exec $exch_symbol_ $date_`
  else 
    shortcode_=${instrument}
  fi

echo -e "\n******FOR:: $shortcode_ : \t$data_src_entry_ : \t$exch_symbol_ :\t$no_of_lots\n"
  echo "POS_ awk -F, -v undr=${underlying} '{gsub(/ /,'',\$2); if(\$2==undr){print \$0}}' $LOTSIZE_FILE | awk -F, -v no_lots_=$no_of_lots -v exp_no=${exp_no_} '{if(exp_no==0){print \$3*no_lots_} else if(exp_no==1){print \$4*no_lots_} else {print \$5*no_lots_}}'"

  pos_=`awk -F, -v undr=${underlying} '{gsub(/ /,"",$2); if($2==undr){for(i=1;i<=NF;i++){ printf "%s,",$(i)} printf "\n" }}' $LOTSIZE_FILE | awk -F, -v no_lots_=$no_of_lots -v exp_no=${exp_no_} '{if(exp_no==0){print $3*no_lots_} else if(exp_no==1){print $4*no_lots_} else {print $5*no_lots_}}'` 
  lot_size_=`awk -F, -v undr=${underlying} '{gsub(/ /,"",$2); if($2==undr){for(i=1;i<=NF;i++){ printf "%s,",$(i)} printf "\n" }}' $LOTSIZE_FILE | awk -F, -v exp_no=${exp_no_} -F, '{if(exp_no==0){print $3} else if(exp_no==1){print $4} else {print $5}}'`
  echo "grep $underlying $LOTSIZE_FILE | awk -F, -v exp_no=${exp_no_} -F, '{if(exp_no==0){print \$3} else if(exp_no==1){print \$4} else {print \$5}}'"
  echo "pos: $pos_, lot_sz: $lot_size_, no_lot: $no_of_lots"
  echo "CMD: grep $exch_symbol_ $ORS_TRADE_FILE | tr '\001' ' ' | tac" 
  echo "echo $no_of_lots | awk '{if(\$1<0) $1=\$1*-1; print \$1}'"
  abs_no_of_lots=`echo $no_of_lots | awk 'function abs(x){return ((x<0.0) ? -x : x)} { print abs($1)}'`
  echo "GREP grep $exch_symbol_ $ORS_TRADE_FILE | tr '\001' ' ' | tac | awk -v position=${pos_} -v lot_sz=${lot_size_} 'function abs(x){return ((x<0.0) ? -x : x)} {if((position>0 && \$2 == 0) || (position<0 && \$2 == 1)){printf "%d,%f\n",abs(\$3/lot_sz),\$4}}'"

  count=0;
  for trade in `grep $exch_symbol_ $ORS_TRADE_FILE | tr '\001' ' ' | tac | awk -v position=${pos_} -v lot_sz=${lot_size_} 'function abs(x){return ((x<0.0) ? -x : x)} {if((position>0 && $2 == 0) || (position<0 && $2 == 1)){printf "%d,%f\n",abs($3/lot_sz),$4}}'`;
  do
    echo "TRADE: $trade"
    rep_trade_=`echo $trade | cut -d, -f1`
    price_=`echo $trade | cut -d, -f2`
    for i in `seq 1 $rep_trade_`; 
    do
        ((count++))
       echo -e "   iSEQ, px, cnt: $i\t$lot_size_ $price_ $count" 
       total_comm_=`echo "$comm_ $lot_size_ $price_" | awk 'function abs(x){return ((x<0.0) ? -x : x)} {print abs($1*$2*$3)}'`
       echo "echo \"$comm_ $lot_size_ $price_\" | awk 'function abs(x){return ((x<0.0) ? -x : x)} {print abs(\$1*\$2*\$3)}'"
       echo "COMM, pos, price : $comm_, $pos_, $price_ "
       
       echo -e "${ORDER_ID_STRT_}000000${start_value_}_${tranch_no}_${instrument}\t${shortcode_}\t${TYPE_}\t${get_sign_}${lot_size_}\t${price_}\t${total_comm_} ${time_in_sec_}\tPASS"
       ((start_value_++)) 
    [[ $count == $abs_no_of_lots ]] && break;
    done
    [[ $count == $abs_no_of_lots ]] && break;
  done
done

