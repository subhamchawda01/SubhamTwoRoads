#!/bin/bash

EOD_POS="/spare/local/files/NSE/MidTermLogs/EODPosFiles/live"
date_=`date +"%Y%m%d"`
tranchid_="/spare/local/files/NSE/MidTermLogs/trancheid_2_strategydetails_map"
simr_id=`grep SIMR $tranchid_ | awk '{print $1}'`
ssgap_id=`grep SSGapv2 $tranchid_ | awk '{print $1}'`
echo "SIMR: $simr_id SSGAP: $ssgap_id "

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date_ T`
if [ $is_holiday = "1" ] ; then
        echo "NSE Holiday. Exiting...";
        exit;
fi
echo "Check file" 

order_file="/spare/local/files/NSE/FileExecLogs/ordersfile"
order_id=`tail  -1 $order_file | awk '{print $1}'`

order_to_use_simr=$(( order_id + 100))
order_to_use_ssgap=$(( order_id + 101))

echo "CurrentOrder: $order_id ToUseSIMR: $order_to_use_simr ToUseSSgap: $order_to_use_ssgap"
echo "*** SIMR ***"
echo "*** SIMR ***" >/tmp/mail_alert_for_pedning_intra_Day_pos
pending_simr=`grep SIMR $EOD_POS  | grep -v ',0.0' | grep FUT0 | wc -l`
sym_=`echo -n $(grep SIMR $EOD_POS  | grep -v ',0.0' | grep FUT0 | cut -d',' -f2) | tr ' ' '|'`
pos_=`echo -n $(grep SIMR $EOD_POS  | grep -v ',0.0' | grep FUT0 | cut -d',' -f3) | tr ' ' '|'`
order_sim=`awk -v var=$simr_id '{if($4 ==var) {print$0}}' $order_file | tail -n1`
order_id_simr=`echo $order_sim | awk '{print $1}'`
echo "Order Sent By Strat:       $order_sim"
echo "Order Sent By Strat:       $order_sim" >>/tmp/mail_alert_for_pedning_intra_Day_pos
simr_order_toexec="$order_to_use_simr    COMPLEX%${pending_simr}%${sym_}%${pos_}%   0   $simr_id    -1  Entry"
echo "ORDER To be sent:       $simr_order_toexec"
echo "ORDER To be sent:       $simr_order_toexec" >>/tmp/mail_alert_for_pedning_intra_Day_pos
echo >>/tmp/mail_alert_for_pedning_intra_Day_pos

echo "*** SSGAP ***"
echo "*** SSGAP ***" >>/tmp/mail_alert_for_pedning_intra_Day_pos
pending_ssgap=`grep SSGap $EOD_POS  | grep -v ',0.0' | grep FUT0 | wc -l`
sym_=`echo -n $(grep SSGap $EOD_POS  | grep -v ',0.0' | grep FUT0 | cut -d',' -f2) | tr ' ' '|'`
pos_=`echo -n $(grep SSGap $EOD_POS  | grep -v ',0.0' | grep FUT0 | cut -d',' -f3) | tr ' ' '|'`
order_sim=`awk -v var=$ssgap_id '{if($4 ==var) {print$0}}' $order_file | tail -n1`
echo "Order Sent By Strat:       $order_sim"
echo "Order Sent By Strat:       $order_sim" >>/tmp/mail_alert_for_pedning_intra_Day_pos
ssgap_order_toexec="$order_to_use_ssgap   COMPLEX%${pending_ssgap}%${sym_}%${pos_}%   0   $ssgap_id   -1  Entry"
echo "ORDER to be sent:            $ssgap_order_toexec"
echo "ORDER to be sent:            $ssgap_order_toexec" >>/tmp/mail_alert_for_pedning_intra_Day_pos
order_id_ssgap=`echo $order_sim | awk '{print $1}'`

count_simr=`ssh 10.23.227.61 "grep $order_id_simr /spare/local/logs/alllogs/MediumTerm/manual_execlogic_dbglog.$date_ |wc -l"`
count_ssgap=`ssh 10.23.227.61 "grep $order_id_ssgap /spare/local/logs/alllogs/MediumTerm/manual_execlogic_dbglog.$date_ |wc -l"`
echo "NOTIONAL SIMR: $count_simr SSGAP: $count_ssgap "
echo >>/tmp/mail_alert_for_pedning_intra_Day_pos
echo "NOTIONAL SIMR: $count_simr SSGAP: $count_ssgap " >>/tmp/mail_alert_for_pedning_intra_Day_pos
echo >>/tmp/mail_alert_for_pedning_intra_Day_pos
echo "ORDER FILE: $order_file" >> /tmp/mail_alert_for_pedning_intra_Day_pos

cat /tmp/mail_alert_for_pedning_intra_Day_pos | mailx -s "IntraDayOrderPending $date_ SIMR: $pending_simr SSGAP: $pending_ssgap" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in  smit@tworoads-trading.co.in  subham.chawda@tworoads-trading.co.in;


#SIMR 1642172400000000783 COMPLEX%9%NSE_TATAPOWER_FUT0|NSE_MCDOWELL-N_FUT0|NSE_JINDALSTEL_FUT0%2.0|4.0|-2.0%  0   100 -1  Entry

#SSGAP 1642173300000000812 COMPLEX%9%NSE_ACC_FUT0|NSE_AXISBANK_FUT0|%-2.0|1.0%    0   45  -1  Entry



