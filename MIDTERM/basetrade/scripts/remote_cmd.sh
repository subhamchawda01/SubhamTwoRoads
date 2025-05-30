#!/bin/bash


if [ $# -lt "2" ] ; then printf "USAGE: %s <SHORTCODE> <op_code>\n(log, trade, cron, start, getflat, seteco, cmd, plot etc/)\n\n" $0; exit; fi
srvs=(sdv-chi-srv11 sdv-chi-srv13 sdv-chi-srv14 sdv-tor-srv11 sdv-tor-srv12)

# @ 830 EST  CME
ZN_0=("sdv-chi-srv11 `seq 13011 13025`")
zn_0=("sdv-chi-srv11 `seq 961 969`")
ZF_0=("sdv-chi-srv13 `seq 12011 12025`")
zf_0=("sdv-chi-srv13 `seq 611 619`")
ZB_0=("sdv-chi-srv14 `seq 14011 14012`")
UB_0=("10.23.82.54 `seq 15011 15015`")

# @ 0820 EST TMX
CGB_0=("10.23.182.51 `seq 21011 21019`")
BAX_1=("10.23.182.52 `seq 23011 23019`")
BAX_2=("10.23.182.52 `seq 24011 24019`")
BAX_3=("10.23.182.52 `seq 25011 25019`")
BAX_4=("10.23.182.52 `seq 26011 26019`")
BAX_5=("10.23.182.52 `seq 27011 27019`")

# @0800 EST EUREX
FGBL_0=("sdv-fr2-srv12 `seq 4011 4013`")
fgbl_0=("sdv-fr2-srv12 `seq 401 409`")
FGBS_0=("sdv-fr2-srv13 `seq 2011 2030`")
fgbs_0=("sdv-fr2-srv13 `seq 301 309`")
FGBM_0=("sdv-fr2-srv14 `seq 3011 3030`")
fgbm_0=("sdv-fr2-srv14 `seq 101 109`")
FESX_0=("sdv-fr2-srv12 `seq 6011 6025`")
fesx_0=("sdv-fr2-srv12 `seq 201 209`")
FOAT_0=("sdv-fr2-srv12 `seq 9011 9019`")  # EST_800 to EST_830
foat_0=("10.23.102.52 `seq 701 709`")  # fr2-12
FDAX_0=("10.23.102.53  `seq 7011 7016`")
FBTP_0=("10.23.102.52  `seq 8011 8019`")
fbtp_0=("10.23.102.52  `seq 801 809`")

# @0900 EST BMF
BR_WIN_0=("10.23.23.12 `seq 32011 32019`")   
BR_IND_0=("10.23.23.11 `seq 33011 33012`")
#BR_WIN_0=("10.23.23.12 `seq 32011 32012`")
DI1F16=("10.23.23.11 `seq 35011 35019`")
DI1F15=("10.23.23.11 `seq 34011 34019`")
DI1F18=("10.23.23.11 `seq 37011 37019`")
DI1F17=("10.23.23.11 `seq 39011 39019`")
DI1N14=("10.23.23.11 `seq 34111 34119`")
DI1N15=("10.23.23.11 `seq 35111 35119`")
BR_DOL_0=("10.23.23.13 `seq 30011 30029`")
BR_IND_0=("10.23.23.12 `seq 31011 31015`")


# LIFFE
LFI_0=("10.23.52.53 `seq 90011 90015`")
LFI_1=("10.23.52.53 `seq 90111 90115`")
LFI_2=("10.23.52.53 `seq 90211 90215`")
LFI_3=("10.23.52.53 `seq 90311 90315`")
LFI_4=("10.23.52.53 `seq 90411 90415`")
LFI_5=("10.23.52.53 `seq 90511 90515`")
LFI_6=("10.23.52.53 `seq 90611 90615`")

LFL_0=("10.23.52.53 `seq 80011 80012`")
LFL_1=("10.23.52.53 `seq 80111 80112`")
LFL_2=("10.23.52.53 `seq 80211 80212`")
LFL_3=("10.23.52.53 `seq 80311 80312`")
LFL_4=("10.23.52.53 `seq 80411 80412`")
LFL_5=("10.23.52.53 `seq 80511 80512`")
LFL_6=("10.23.52.53 `seq 80611 80612`")

KFFTI_0=("10.23.52.52 `seq 50011 50015`")
kffti_0=("10.23.52.52 `seq 50016 50019`")
LFR_0=("10.23.52.51 `seq 60011 60019`")
LFZ_0=("10.23.52.51 `seq 70011 70019`")
lfr_0=("10.23.52.51 `seq 1211 1214`")
lfz_0=("10.23.52.51 `seq 1311 1314`")
JFFCE_0=("10.23.52.53 `seq 40011 40015`")
jffce_0=("10.23.52.53 `seq 40016 40019`")
YFEBM_0=("10.23.52.53 `seq 42011 42015`")


#OSE
NK_0=("10.134.210.184 `seq 210051 210051`")

PRODS=$1
# some shorthands
case $PRODS in
    LFI)
	PRODS="LFI_0 LFI_1 LFI_2 LFI_3 LFI_4 LFI_5 LFI_6" ;;
    LFL)
	PRODS="LFL_0 LFL_1 LFL_2 LFL_3 LFL_4 LFL_5 LFL_6" ;;
    BAX)
	PRODS="BAX_1 BAX_2 BAX_3 BAX_4 BAX_5" ;;
    DI1)
	PRODS="DI1F15 DI1F16 DI1F17 DI1F18 DI1N15" ;;
    LIFFE)
	PRODS="LFI_0 LFI_1 LFI_2 LFI_3 LFI_4 LFI_5 LFI_6 KFFTI_0 LFR_0 LFZ_0 JFFCE_0 YFEBM_0" ;; # LFLs 
    liffe)
	PRODS="LFI_0 LFI_1 LFI_2 LFI_3 LFI_4 LFI_5 LFI_6 kffti_0 lfr_0 lfz_0 jffce_0" ;; # LFLs 
    CME)
	PRODS="ZF_0 ZN_0 UB_0" ;;
    *)
	PRODS="$PRODS" ;;
esac
       
PRODS="`echo $PRODS | sed 's/\<fti\>/KFFTI_0/g; s/\<fce\>/JFFCE_0/g; s/\<btp\>/FBTP_0/g;'`"
echo "Command on :  $PRODS";

for t in $PRODS;
do
    prod=(${!t})
    if [ ! "$prod" ]; then echo "Wrong product name!!"; exit; fi

    if [ "${prod[0]::10}" == "10.23.52.5" ]; 
    then 
	user_msg=/home/dvctrader/LiveExec/scripts/user_msg.sh
    else
	user_msg=/home/dvctrader/LiveExec/scripts/onload_user_msg.sh
    fi

    code=${prod[1]}
    code=${code::$((${#code}-2))}
    today=`date +%Y%m%d`

    cmd=""
    if [ $# -gt "2" ]
    then
	tid=$3
    else
	tid="*"
    fi
    
    if [[ $tid = "*" ]]; then TIDS=(${prod[@]:1}); 
    else
	echo "<$tid>";
	TIDS="";
	for t in $tid; do TIDS=(${TIDS[*]} ${code}$t); done
    fi


    echo ${TIDS[*]}
    if [ $# -gt "1" ]; 
    then 
	case $2 in 
	    0|log|LOG )
		if [[ $tid = '*' ]]; then 
		    cmd="tail -n3 /spare/local/logs/tradelogs/log.${today}.$code??" ;
		else
		    cmd="for t in ${TIDS[*]}; do echo /spare/local/logs/tradelogs/log.${today}.\$t; tail -n3 /spare/local/logs/tradelogs/log.${today}.\$t; done"
		fi
		;;
	    1|cron|CRON )
	    #cmd="crontab -l | grep -i $1" ;;
		cmd="for TID in ${prod[@]:1}; do  crontab -l | grep -w \$TID ; done | sort -u" ;;
	    2|trade|Trade|TRADE|trades )
		cmd="for t in ${TIDS[@]}; do grep \\.\$t /spare/local/logs/tradelogs/trades.${today}.${code}??  2>/dev/null | tail -n1; done" ;;
	    3|getflat|GETFLAT )
		cmd="for TID in ${TIDS[*]}; do  $user_msg --getflat --traderid \$TID; done" ;;
	    4|pos|POS|position )
		cmd="cat /spare/local/ORSlogs/*/*/position.$3" ;;
	    5|start|START )
		cmd="for TID in ${TIDS[*]}; do  $user_msg --start --traderid \$TID; done" ;;	
	    5|forceready|forceready )
		indicator_index=$4
		if [ ! $indicator_index ]; then indicator_index=0; fi;
		cmd="for TID in ${TIDS[*]}; do  $user_msg --forceindicatorready $indicator_index --traderid \$TID; done" ;;	
	    6|eco|ECO|seteco )
		ecoval=$4
		if [ ! $ecoval ]; then ecoval=5;fi
		cmd="for TID in ${TIDS[*]}; do  $user_msg --seteco $ecoval --traderid \$TID; done" ;;
	    7|chkflat|CHKFLAT|checkflat )
		cmd="grep getflat /spare/local/logs/tradelogs/log.${today}.${code}${tid}; " ;;
	    8|pnl|PNL )
		cmd="~/infracore_install/scripts/see_ors_pnl.pl /spare/local/ORSlogs/*/*/trades.${today} 2>/dev/null" ;;
	    9|cmd|CMD )
		cmd=$3 
		echo $cmd ;;
	    10|maxloss|MAXLOSS )
		cmd="for f in \`~/basetrade/scripts/print_crontabbed_params.sh | grep US | grep $t\` ; do grep -H \"UNIT_TRADE_SIZE\\|LOSS\" \$f; echo '========='; done"
		;;
	    11|plot|PLOT )
		cmd="-X /home/dvctrader/basetrade/scripts/plot_prod_real_I.sh US $t ; /home/dvctrader/basetrade/scripts/plot_price_prod_real_A.sh US $t " 
		;;
	    12|setuts|SETUTS )
		if [ $# -gt "3" ]; then uts=$4; else uts=2; fi;
		cmd="for TID in ${TIDS[*]}; do  $user_msg --setunitsize $uts --traderid \$TID; done" ;;
	    13|setmaxloss|SETMAXLOSS)
		if [ $# -gt "3" ]; then maxloss=$4; else maxloss=2; fi;
		cmd="for TID in ${TIDS[*]}; do  $user_msg --setmaxloss $maxloss --traderid \$TID; done" ;;
	#14|orders|ORDERS )
	    #cmd="for TID in ${TIDS[*]}; do  $user_msg --showorders --traderid \$TID; done" ;;
	    14| setgpos |setglobalpos | SETGLOBALPOS )
	    gpos=$4
	    cmd="for TID in ${TIDS[*]}; do  $user_msg --setmaxglobalpos $gpos --traderid \$TID; done" ;;
	    * )
		echo "Still not Set"; exit ;;
	esac
	
	echo "ssh dvctrader@${prod[0]} $cmd"
	ssh dvctrader@${prod[0]} $cmd
    fi
done
