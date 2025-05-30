#!/bin/bash
PhySetFile="/spare/local/tradeinfo/NSE_Files/SecuritiesUnderPhysicalSettlement/fo_securities_under_physical_settlement.csv"
SecMargDir="/spare/local/tradeinfo/NSE_Files/SecuritiesMarginFiles"
MidtermDir="/spare/local/logs/alllogs/MediumTerm"
ExecOrder="/home/dvctrader/LiveConfigs/shortcode_to_max_orders_"
OrsDir="/spare/local/ORSlogs/NSE_FO/MSFO"
HardMargin="/home/pengine/prod/live_configs/common_initial_margin_file.txt"
DefaultAddtsFile="/home/pengine/prod/live_configs/sdv-ind-srv11_addts.cfg"
ind11t="dvctrader@10.23.227.61"
tmpfil1="/tmp/alertchecking"
tmpfil2="/tmp/alertchecking2"
date_=`date +"%Y%m%d"`

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date_ T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi

SymHandle=$1
echo "SymHandel : $SymHandle"

#/home/pengine/prod/live_scripts/ors_control.pl NSE MSFO ADDTRADINGSYMBOL NSE_NIFTY_C0_O6_M1_W 40 20 40 80

addts_prod() {
  if [ `grep "CONTROLCOMMAND ADDTRADINGSYMBOL" ${OrsDir}/log.${date_} | grep -w ${SymHandle} | tail -1 | wc -l` -eq 1 ] ; then
    addts_prod_positions=`grep "CONTROLCOMMAND ADDTRADINGSYMBOL" ${OrsDir}/log.${date_} | grep -w ${SymHandle} | tail -1 | cut -d' ' -f3-7`;
    prod=`echo $addts_prod_positions | cut -d' ' -f1`
    pos1=`echo $addts_prod_positions | cut -d' ' -f2`
    pos2=`echo $addts_prod_positions | cut -d' ' -f3`
    pos3=`echo $addts_prod_positions | cut -d' ' -f4`
    pos4=`echo $addts_prod_positions | cut -d' ' -f5`
    /home/pengine/prod/live_scripts/ors_control.pl NSE MSFO ADDTRADINGSYMBOL $prod $((pos1 + 20)) $((pos2 + 20)) $((pos3 + 20)) $((pos4 + 20))
    echo "ORS log data - ADDTS : $addts_prod_positions"
  elif [ `grep -w ${SymHandle} $DefaultAddtsFile | tail -1 | wc -l` -eq 1 ] ; then
    addts_prod_positions=`grep -w ${SymHandle} $DefaultAddtsFile | tail -1 | cut -d' ' -f4-`;
    prod=`echo $addts_prod_positions | cut -d' ' -f1`
    pos1=`echo $addts_prod_positions | cut -d' ' -f2`
    pos2=`echo $addts_prod_positions | cut -d' ' -f3`
    pos3=`echo $addts_prod_positions | cut -d' ' -f4`
    pos4=`echo $addts_prod_positions | cut -d' ' -f5`
    /home/pengine/prod/live_scripts/ors_control.pl NSE MSFO ADDTRADINGSYMBOL $prod $pos1 $pos2 $pos3 $pos4
    echo "Default addts file data - ADDTS : $addts_prod_positions"
  else
    /home/pengine/prod/live_scripts/ors_control.pl NSE MSFO ADDTRADINGSYMBOL $SymHandle 40 20 40 40
    echo "NSE MSFO ADDTRADINGSYMBOL $SymHandle 40 20 40 40" >> $DefaultAddtsFile
    echo "ADDTS : $SymHandle 40 20 40 40"
  fi
  sleep 3;
  if [ `grep -w $SymHandle ${OrsDir}/log.${date_} | tail -1 | egrep "ControlThread.*ERROR|Resetting" | wc -l` -eq 1 ] ; then
    if [ `grep -w $SymHandle ${OrsDir}/log.${date_} | tail -1 | grep "ControlThread.*ERROR" | grep "MARGIN NOT PROVIDED" | wc -l` -eq 1 ] ; then
      security_not_found
    elif [ `grep -w $SymHandle ${OrsDir}/log.${date_} | tail -1 | egrep "Resetting" | wc -l` -eq 1 ] ; then
      margin_check
    elif [ `grep -w $SymHandle ${OrsDir}/log.${date_} | tail -1 | egrep "ControlThread.*ERROR" | grep "UNDER PHYSICAL SETTLEMENT" | wc -l` -eq 1 ] ; then
      physical_settlement
    else
      echo "addts of $SymHandle failed"
      grep -w $SymHandle ${OrsDir}/log.${date_} | tail -1 | grep "ControlThread.*ERROR"
    fi
  else
    echo "addts of $SymHandle was successful no error logs"
  fi
}

max_order() {
  cd $MidtermDir

  grep "ALERT" *_execlogic_dbglog.${date_} > $tmpfil1
  #tail -v -n +0 *_execlogic_dbglog.${date_} | awk '/^==> / {a=substr($0, 5, length-8); next} {print a":"$0}' | grep "ALERT" > $tmpfil1
  #zgrep "ALERT" *_execlogic_dbglog.20201008.gz  > $tmpfil1

  if grep -waq $SymHandle $tmpfil1 ; then
    midexec=`grep -w $SymHandle $tmpfil1 | tail -1 |cut -d' ' -f1 | cut -d'_' -f1` 
    livefile="${ExecOrder}$midexec"
    echo "mid exec : $midexec  livefile : $livefile"

    if ssh $ind11t "grep -waq $SymHandle $livefile"; then
      ssh $ind11t "awk -F',' -v sym=$SymHandle '{ if (\$1==sym) print \$1\",\"\$2*2; else print \$0}' $livefile > $tmpfil2"
      ssh $ind11t "cp $tmpfil2 $livefile"
      echo "livefile : $livefile symbol : $SymHandle value doubled"
    else
      ssh $ind11t "echo \"${SymHandle},60\" >> $livefile"
      echo "livefile : $livefile symbol : $SymHandle value changed"
    fi

  fi
  addts_prod
}

margin_check() {
  if ! grep -waq $SymHandle $HardMargin ; then	
    append_line=`grep -n "NSE_" $HardMargin | tail -1 | cut -d':' -f1`
    sed "$append_line a ${SymHandle} 300 300 300" $HardMargin > /tmp/temp_margin_file
    cat /tmp/temp_margin_file > $HardMargin
    echo "file : $HardMargin symbol : $SymHandle appended"
    /home/pengine/prod/live_scripts/ors_control.pl NSE MSFO RELOADMARGINFILE
    sleep 3
  else
    max_pos=`expr 2 \* $(grep -w $SymHandle $HardMargin | head -1 | awk 'function max(a,b) { return a>b ? a:b } {print max(max($2,$3),$4)}')`
    replace_margin=`grep -w $SymHandle $HardMargin | head -1`
    replace_margin_with="$SymHandle $max_pos $max_pos $max_pos"
    replace_margin_with=`sed 's/&/\\\&/g' <<< $replace_margin_with`
    sed "s/$replace_margin/$replace_margin_with/g" $HardMargin > /tmp/temp_margin_file
    cat /tmp/temp_margin_file > $HardMargin
    echo "file : $HardMargin symbol : $SymHandle value doubled "
    /home/pengine/prod/live_scripts/ors_control.pl NSE MSFO RELOADMARGINFILE
    sleep 3
  fi
  addts_prod
}

security_not_found() {
  secfile="${SecMargDir}/security_margin_${date_}.txt"
  symbol12=`echo "$SymHandle" | cut -d'_' -f1,2`
  if ! grep -waq $SymHandle $secfile ; then
    symbol12="${symbol12}_FUT0"
    if grep -waq $symbol12 $secfile ; then
      marval=`grep -w $symbol12 $secfile | cut -d' ' -f2`
      ssh $ind11t "echo \"$SymHandle $marval\" >> $secfile"
      echo "Secfile : $secfile  symbol : $SymHandle $marval"
      /home/pengine/prod/live_scripts/ors_control.pl NSE MSFO RELOADSECURITYMARGINFILE
      sleep 3
    else
      echo "Sec Margin File cann't be handled FUT0 for $symbol12 is missing"
      exit
    fi
  else
    marval=`grep -w $SymHandle $secfile | cut -d' ' -f2`
    echo "$SymHandle : $marval present in $secfile"
  fi
  addts_prod
}

physical_settlement() {
  phy_symbol=`echo "$SymHandle" | cut -d'_' -f2`
  if grep -waq $phy_symbol $PhySetFile; then
    ssh $ind11t sed -i "/$phy_symbol/d" $PhySetFile
    /home/pengine/prod/live_scripts/ors_control.pl NSE MSFO RELOADMARGINFILE 
    sleep 3
    echo "$phy_symbol removed from $PhySetFile"
  else
    echo "$phy_symbol not found in $PhySetFile"
  fi
  addts_prod  
}

if [ $# == 2 ]; then
        case "$2" in
                "maxorder")  max_order
                ;;
                "security")  security_not_found
                ;;
                "addts")  addts_prod
                ;;
                "margin")  margin_check
                ;;
                "physical")  physical_settlement
                ;;
        esac
else
        echo "USAGE: SCRIPT <short_code> <maxorder/security/addts/margin/physical>"
fi
