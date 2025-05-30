#!/bin/bash
PhySetFile="/spare/local/tradeinfo/BSE_Files/SecuritiesUnderPhysicalSettlement/fo_securities_under_physical_settlement.csv"
SecMargDir="/spare/local/tradeinfo/BSE_Files/SecuritiesMarginFiles"
HardMargin="/home/pengine/prod/live_configs/common_initial_margin_file.txt"
AddtsRejectFile="/tmp/reject_addts_prod"

declare -A addts_shortcode_position
declare -A security_shortcode_position
declare -A common_margin_shortcode_position
declare -A physical_shortcode_position
declare -A under_ban_shortcode_position 

declare -A hostname_to_ip
hostname_to_ip=( ["sdv-indb-srv12"]="192.168.132.12" )

declare -A hostname_to_profile=( ["sdv-indb-srv12"]="MSEQ9" )

#/home/pengine/prod/live_scripts/ors_control.pl BSE MSFO ADDTRADINGSYMBOL BSE_NIFTY_C0_O6_M1_W 40 20 40 80

print_msg_and_exit (){
   echo $1;
   exit
}

error_check() {
  if [ ${#security_shortcode_position[@]} -ne 0 ]; then
    echo "RESOLVE MARGIN"
    security_not_found
  fi
  if [ ${#common_margin_shortcode_position[@]} -ne 0 ]; then
    echo "RESOLVE RESETTING"
    common_init_margin
  fi
  if [ ${#physical_shortcode_position[@]} -ne 0 ]; then
    echo "RESOLVE PHYSICAL"
    physical_settlement
  fi
}

addts_check() {
  error_count=`grep -a -w "$SymHandle" ${OrsDir}/log.${date_} | grep -a -v "INFO" | tail -1 | egrep "ControlThread.*ERROR|Resetting" | wc -l`
  error=`grep -a -w "$SymHandle" ${OrsDir}/log.${date_} | grep -a -v "INFO" | tail -1 | egrep "ControlThread.*ERROR|Resetting"`
  if [[ $error_count -eq 1 ]] ; then
    if [ `echo "$error" | grep -a "MARGIN NOT PROVIDED" | wc -l` -eq 1 ] ; then
      echo "ERROR MARGIN : $SymHandle"
      security_shortcode_position[$SymHandle]=$position
    elif [ `echo "$error" | grep -a "Resetting" | wc -l` -eq 1 ] ; then
      echo "ERROR RESETTING : $SymHandle"
      common_margin_shortcode_position[$SymHandle]=$position
    elif [ `echo "$error" | grep -a "UNDER PHYSICAL SETTLEMENT" | wc -l` -eq 1 ] ; then
      echo "ERROR PHYSICAL : $SymHandle"
      physical_shortcode_position[$SymHandle]=$position
    else
      echo "addts of $SymHandle failed"
      echo "$error"
    fi
  else
    echo "addts of $SymHandle was successful no error logs"
  fi

}

do_file_addts() {
#  /home/pengine/prod/live_scripts/ors_control.pl BSE ${profile} LOADTRADINGSYMBOLFILE $AddtsRejectFile
   /home/pengine/prod/live_scripts/ADDTRADINGSYMBOL.sh $AddtsRejectFile
}

addts_prod_file() {
  if [ $position -ne -1 ]; then
    echo "BSE $profile ADDTRADINGSYMBOL \"$SymHandle\" $position $position $position $position" >> $AddtsRejectFile
    echo "Input File Position - ADDTS : ${SymHandle} $position $position $position $position" 
  elif [ `grep -a " ADDTRADINGSYMBOL " ${OrsDir}/log.${date_} | grep -a -v "ERROR" | grep -a -w "$SymHandle" | tail -1 | wc -l` -eq 1 ] ; then
    line_addts=`grep -a " ADDTRADINGSYMBOL " ${OrsDir}/log.${date_} | grep -a -v "ERROR" | grep -a -w "$SymHandle" | tail -1`;
    if [ `echo $line_addts | grep -a "CONTROLCOMMAND" | wc -l` -eq 1 ]; then
      addts_prod_positions=`echo $line_addts | cut -d' ' -f4-7`;
    else
      addts_prod_positions=`echo $line_addts | awk '{print $(NF-3),$(NF-2),$(NF-1),$NF}'`
    fi
    pos1=`echo $addts_prod_positions | cut -d' ' -f1`
    pos2=`echo $addts_prod_positions | cut -d' ' -f2`
    pos3=`echo $addts_prod_positions | cut -d' ' -f3`
    pos4=`echo $addts_prod_positions | cut -d' ' -f4`
    if [ $worst_case_pos -eq 1 ]; then
      echo "ORS log Position - ADDTS : ${SymHandle} $((pos1 + 20)) $((pos2 + 20)) $((pos3 + 20)) $((pos4 + 20))"
      echo "BSE $profile ADDTRADINGSYMBOL \"$SymHandle\" $((pos1 + 20)) $((pos2 + 20)) $((pos3 + 20)) $((pos4 + 20))" >> $AddtsRejectFile
      #echo "ORS log Position - ADDTS : ${SymHandle} $((pos1 + 20)) $((pos2 + 20)) $((pos3 + 20)) $((pos4 + 20))"
    else
      echo "ORS log Position - ADDTS : ${SymHandle} $pos1 $pos2 $pos3 $pos4"
      echo "BSE $profile ADDTRADINGSYMBOL \"$SymHandle\" $pos1 $pos2 $pos3 $pos4" >> $AddtsRejectFile
    fi
  elif [ `grep -a -w "$SymHandle" $DefaultAddtsFile | tail -1 | wc -l` -eq 1 ] ; then
    addts_prod_positions=`grep -a -w "$SymHandle" $DefaultAddtsFile | tail -1 | cut -d' ' -f5-`;
    pos1=`echo $addts_prod_positions | cut -d' ' -f1`
    pos2=`echo $addts_prod_positions | cut -d' ' -f2`
    pos3=`echo $addts_prod_positions | cut -d' ' -f3`
    pos4=`echo $addts_prod_positions | cut -d' ' -f4`
    echo "BSE $profile ADDTRADINGSYMBOL \"$SymHandle\" $pos1 $pos2 $pos3 $pos4" >> $AddtsRejectFile
    echo "Default addts file Position - ADDTS : ${SymHandle} $pos1 $pos2 $pos3 $pos4"
  else
    echo "BSE $profile ADDTRADINGSYMBOL \"$SymHandle\" 20 20 20 20" >> $AddtsRejectFile
    echo "BSE $profile ADDTRADINGSYMBOL $SymHandle 20 20 20 20" >> $DefaultAddtsFile
    echo "ADDTS : $SymHandle 20 20 20 20"
  fi
}

addts_prod() {
  
  unset security_shortcode_position
  unset common_margin_shortcode_position
  unset physical_shortcode_position
  unset under_ban_shortcode_position 
  declare -A security_shortcode_position
  declare -A common_margin_shortcode_position
  declare -A physical_shortcode_position
  declare -A under_ban_shortcode_position 

  >$AddtsRejectFile
  for key in ${!addts_shortcode_position[@]}; do
    SymHandle=$key
    position=${addts_shortcode_position[${key}]}
    echo "SymHandel : $SymHandle"
    addts_prod_file
  done
  do_file_addts
  sleep 3
  for key in ${!addts_shortcode_position[@]}; do
    SymHandle=$key
    addts_check
  done
  error_check

}

common_init_margin() {

  unset addts_shortcode_position
  declare -A addts_shortcode_position
  for key in ${!common_margin_shortcode_position[@]}; do
    SymHandle=$key
    addts_shortcode_position[$key]=${common_margin_shortcode_position[${key}]}
    echo "SymHandel : $SymHandle"

    if ! grep -waq "$SymHandle" $HardMargin ; then	
      append_line=`grep -a -n "BSE_" $HardMargin | tail -1 | cut -d':' -f1`
      sed "$append_line a ${SymHandle} 300 300 300" $HardMargin > /tmp/temp_margin_file
      cat /tmp/temp_margin_file > $HardMargin
      echo "file : $HardMargin symbol : $SymHandle appended"
    else
      max_pos=`expr 2 \* $(grep -a -w "$SymHandle" $HardMargin | head -1 | awk 'function max(a,b) { return a>b ? a:b } {print max(max($2,$3),$4)}')`
      replace_margin=`grep -a -w "$SymHandle" $HardMargin | head -1`
      replace_margin_with="$SymHandle $max_pos $max_pos $max_pos"
      replace_margin_with=`sed 's/&/\\\&/g' <<< $replace_margin_with`
      sed "s/$replace_margin/$replace_margin_with/g" $HardMargin > /tmp/temp_margin_file
      cat /tmp/temp_margin_file > $HardMargin
      echo "file : $HardMargin symbol : $SymHandle value doubled "
    fi
  done
  /home/pengine/prod/live_scripts/ors_control.pl BSE $profile RELOADMARGINFILE
  sleep 3
  addts_prod

}

security_not_found() {

  unset addts_shortcode_position
  declare -A addts_shortcode_position
  for key in ${!security_shortcode_position[@]}; do
    SymHandle=$key
    addts_shortcode_position[$key]=${security_shortcode_position[${key}]}
    echo "SymHandel : $SymHandle"

    secfile="${SecMargDir}/security_margin_${date_}.txt"
    symbol12=`echo "$SymHandle" | cut -d'_' -f1,2`
    if ! grep -waq "$SymHandle" $secfile || grep -waq "${SymHandle}\*" $secfile ; then
      symbol12="${symbol12}_FUT0"
      if grep -waq "$symbol12" $secfile ; then
        marval=`grep -a -w "$symbol12" $secfile | cut -d' ' -f2`
        ssh dvctrader@${ip_add} "echo \"$SymHandle $marval\" >> $secfile"
        echo "Secfile : $secfile  symbol : $SymHandle $marval"
      else
        ssh dvctrader@${ip_add} "echo \"$SymHandle 30\" >> $secfile"
        echo "Secfile : $secfile  symbol : $SymHandle 30"
      fi
    else
      marval=`grep -a -w "$SymHandle" $secfile | cut -d' ' -f2`
      echo "$SymHandle : $marval present in $secfile"
    fi
  done
  /home/pengine/prod/live_scripts/ors_control.pl BSE $profile RELOADSECURITYMARGINFILE
  sleep 3
  addts_prod

}

physical_settlement() {

  unset addts_shortcode_position
  declare -A addts_shortcode_position
  for key in ${!physical_shortcode_position[@]}; do
    SymHandle=$key
    addts_shortcode_position[$key]=${physical_shortcode_position[${key}]}
    echo "SymHandel : $SymHandle"

    phy_symbol=`echo "$SymHandle" | cut -d'_' -f2`
    if grep -waq "$phy_symbol" $PhySetFile; then
      ssh dvctrader@${ip_add} sed -i "/$phy_symbol/d" $PhySetFile
      echo "$phy_symbol removed from $PhySetFile"
    else
      echo "$phy_symbol not found in $PhySetFile"
    fi
  done
  /home/pengine/prod/live_scripts/ors_control.pl BSE $profile RELOADMARGINFILE
  sleep 3
  addts_prod

}

under_ban () {

  unset addts_shortcode_position
  declare -A addts_shortcode_position
  for key in ${!under_ban_shortcode_position[@]}; do
    SymHandle=$key
    addts_shortcode_position[$key]=${under_ban_shortcode_position[${key}]}
    echo "SymHandel : $SymHandle Position : ${under_ban_shortcode_position[${key}]}"

    product=`echo $SymHandle | cut -d'_' -f2`
    echo "Product : $product"
    ssh dvctrader@${ip_add} "sed -i '/$product/d' $UNDERBAN_FILE"
    echo "$product removed from $UNDERBAN_FILE"
  done
  /home/pengine/prod/live_scripts/ors_control.pl BSE $profile RELOADMARGINFILE
  sleep 3
  addts_prod

}


date_=`date +"%Y%m%d"`
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date_ T`
[ "$is_holiday" == "1" ] && print_msg_and_exit "BSE Holiday. Exiting..."

[ `ps aux | grep -a "cme_ilink_ors" | grep -a -v grep | wc -l` != "0" ] || print_msg_and_exit "ORS not running";

reject_type=$1
product_file=$2
SymHandle=$2
position=-1
worst_case_pos=-1
[[ $# -eq 3 ]] && position=$3
hname=`hostname`
ip_add=${hostname_to_ip[$hname]}
profile=${hostname_to_profile[$hname]}
OrsDir=`ps aux | grep -a "cme_ilink_ors" | grep -a -v grep | awk '{print $NF}'`
DefaultAddtsFile="/home/pengine/prod/live_configs/`hostname`_addts.cfg"
UNDERBAN_FILE="/spare/local/tradeinfo/BSE_Files/SecuritiesUnderBan/fo_secban_${date_}.csv"
ors_config_file="/home/pengine/prod/live_configs/common_${profile}_ors.cfg"
control_port=`grep -a "Control_Port" ${ors_config_file} | cut -d' ' -f2`
echo "Control_port: $control_port"


if [ $# -eq 2 ] || [ $# -eq 3 ]; then
        case "$reject_type" in

                "worstcasepos")
                   if [ `echo $SymHandle | grep -a "BSE_" | wc -l` -eq 1 ]; then
                     addts_shortcode_position[$SymHandle]=$position
                   else
                     while read -u 9 line
                     do
                       SymHandle=`echo $line | cut -d' ' -f 1`
                       [[ `echo $line | wc -w` -eq 2 ]] && position=`echo $line | cut -d' ' -f 2`
                       [[ `echo $line | wc -w` -eq 1 ]] && position=-1
                       addts_shortcode_position[$SymHandle]=$position
                     done 9< $product_file
                   fi
                   worst_case_pos=1;
                   addts_prod
                   ;;

                "margin")
                   if [ `echo $SymHandle | grep -a "BSE_" | wc -l` -eq 1 ]; then
                     security_shortcode_position[$SymHandle]=$position
                   else
                    while read -u 9 line
                     do
                       SymHandle=`echo $line | cut -d' ' -f 1`
                       [[ `echo $line | wc -w` -eq 2 ]] && position=`echo $line | cut -d' ' -f 2`
                       [[ `echo $line | wc -w` -eq 1 ]] && position=-1
                       security_shortcode_position[$SymHandle]=$position
                     done 9< $product_file
                   fi
                   security_not_found
                   ;;

                "addts")
                   if [ `echo $SymHandle | grep -a "BSE_" | wc -l` -eq 1 ]; then
                     addts_shortcode_position[$SymHandle]=$position
                   else
                     while read -u 9 line
                     do
                       SymHandle=`echo $line | cut -d' ' -f 1`
                       [[ `echo $line | wc -w` -eq 2 ]] && position=`echo $line | cut -d' ' -f 2`
                       [[ `echo $line | wc -w` -eq 1 ]] && position=-1
                       addts_shortcode_position[$SymHandle]=$position
                     done 9< $product_file
                   fi
                   addts_prod
                   ;;

                "resetting")
                   if [ `echo $SymHandle | grep -a "BSE_" | wc -l` -eq 1 ]; then
                     common_margin_shortcode_position[$SymHandle]=$position
                   else
                     while read -u 9 line
                     do
                       SymHandle=`echo $line | cut -d' ' -f 1`
                       [[ `echo $line | wc -w` -eq 2 ]] && position=`echo $line | cut -d' ' -f 2`
                       [[ `echo $line | wc -w` -eq 1 ]] && position=-1
                       common_margin_shortcode_position[$SymHandle]=$position
                     done 9< $product_file
                   fi
                   common_init_margin
                   ;;

                "physical")
                   if [ `echo $SymHandle | grep -a "BSE_" | wc -l` -eq 1 ]; then
                     physical_shortcode_position[$SymHandle]=$position
                   else
                     while read -u 9 line
                     do
                       SymHandle=`echo $line | cut -d' ' -f 1`
                       [[ `echo $line | wc -w` -eq 2 ]] && position=`echo $line | cut -d' ' -f 2`
                       [[ `echo $line | wc -w` -eq 1 ]] && position=-1
                       physical_shortcode_position[$SymHandle]=$position
                     done 9< $product_file
                   fi
                   physical_settlement
                   ;;

                "underban")
                   if [ `echo $SymHandle | grep -a "BSE_" | wc -l` -eq 1 ]; then
                     under_ban_shortcode_position[$SymHandle]=$position
                   else
                     while read -u 9 line
                     do
                       SymHandle=`echo $line | cut -d' ' -f 1`
                       [[ `echo $line | wc -w` -eq 2 ]] && position=`echo $line | cut -d' ' -f 2`
                       [[ `echo $line | wc -w` -eq 1 ]] && position=-1
                       under_ban_shortcode_position[$SymHandle]=$position
                     done 9< $product_file
                   fi
                   under_ban
                   ;;
                *)  echo "INVALID REJECT TYPE ENTERED <worstcasepos/resetting/addts/margin/physical/underban>"
        esac
else
        echo "USAGE: SCRIPT <worstcasepos/resetting/addts/margin/physical/underban> <(BSE_PRODUCT)/(BSE_PRODUCT POSITION)/infile(BSE_PRODUCT POSITION)/infile(BSE_PRODUCT)>"
fi
