#1/bin/bash
SecMargDir="/spare/local/tradeinfo/NSE_Files/SecuritiesMarginFiles"
MidtermDir="/spare/local/logs/alllogs/MediumTerm"
ExecOrder="/home/dvctrader/LiveConfigs/shortcode_to_max_orders_"
OrsDir="/spare/local/ORSlogs/NSE_FO/MSFO"
HardMargin="/home/pengine/prod/live_configs/common_initial_margin_file.txt"
ind11t="dvctrader@10.23.227.61"
tmpfil1="/tmp/alertchecking"
tmpfil2="/tmp/alertchecking2"
date_=`date +"%Y%m%d"`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date_ T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi
if [ $# -ne 3 ] ; then
  echo "Called As : " $* "NSE_SUNTV_FUT0";
  exit;
fi

SymHandle=$1

#/home/pengine/prod/live_scripts/ors_control.pl NSE MSFO ADDTRADINGSYMBOL NSE_NIFTY_C0_O6_M1_W 40 20 40 80

cd $MidtermDir

tail -n200 *_execlogic_dbglog.${date_} | awk '/^==> / {a=substr($0, 5, length-8); next} {print a":"$0}' | grep 'ALERT' > $tmpfil1

if grep -aq $SymHandle $tmpfil1 ; then
	midexec=`grep $SymHandle $tmpfil1 | tail -1 |cut -d' ' -f1 | cut -d'_' -f1` 
	livefile="${ExecOrder}$midexec"
	if grep -aq $SymHandle $livefile; then
		ssh $ind11t "awk -F',' -v sym=$SymHandle '{ if ($1==sym) print $1\",\"$2*2; else print $0}' $livefile > $tmpfil2"
		ssh $ind11t "cp $tmpfil2 $livefile"
	else
		ssh $ind11t "echo \"${SymHandle},60\" >> $livefile"
	fi
fi

if ! grep -aq $SymHandle $HardMargin ; then	
	echo "${SymHandle} 198 40 40" >>$HardMargin
fi

secfile="${SecMargDir}/security_margin_${date_}.txt"
symbol12=`echo "$SymHandle" | cut -d'_' -f1,2`
if ! grep -aq $SymHandle $secfile
	symbol12="${symbol12}_FUT0"
	if grep -aq $symbol12 $secfile
		marval=`grep $symbol12 $secfile | cut -d' ' -f2`
		echo "$SymHandle $marval" >> $secfile
	else
		echo "Sec Margin File cann't be handled FUT0 is missing"
	fi
fi
/home/pengine/prod/live_scripts/ors_control.pl NSE MSFO RELOADSECURITYMARGINFILE
/home/pengine/prod/live_scripts/ors_control.pl NSE MSFO ADDTRADINGSYMBOL $SymHandle 60 20 60 80
