#!/bin/bash

USAGE="$0 SHORTCODE ILIST 1 TIMED_DATA PRED_DUR\n$0 SHORTCODE ILIST 0 END_DATE NUM_DAYS START_TIME END_TIME PRED_DUR";

if [[ $# -lt 2 || ($3 != 0 && $3 != 1) || ($3 == 0 && $# -lt 7) || ($3 == 1 && $# -lt 4) ]];
then
    echo -e $USAGE;
    echo -e "Note: for timed_data/ilist, it expects:";
    echo -e "\tbasepx and futpx as the baseprice for the GeneralPrice indicator";
    echo -e "\tthe indicators as possible candidates for the bias for the GeneralPrice";
    exit 1;
fi

shortcode=$1;
ilist=$2;
option=$3;

today=`date +%s%N`;
user=`whoami`;
work_dir="/spare/local/$user/"$shortcode"_"$today"/";
if [ ! -d "$work_dir" ]; then mkdir -p $work_dir; fi

num_indicators=`cat $ilist | awk '{if($1=="INDICATOR"){c+=1}}END{print c}'`

if [ $option == 1 ]; then
  timed_data=$4;
  pred_dur=$5;
elif [ $option == 0 ]; then
  timed_data=$work_dir/timed_data;
  end_date=$4;
  num_days=$5;
  start_time=$6;
  end_time=$7;
  pred_dur=$8;
  start_date=`$HOME/basetrade_install/bin/calc_prev_week_day $end_date $num_days`;
  
  timed_data_generation_exec=$HOME/basetrade/ModelScripts/generate_timed_indicator_data.pl;
  echo $timed_data_generation_exec $ilist $start_date $end_date $start_time $end_time 4000 e1 0 $timed_data "&>" $work_dir/timed_data.log;
  $timed_data_generation_exec $ilist $start_date $end_date $start_time $end_time 4000 e1 0 $timed_data &> $work_dir/timed_data.log;
fi

if [ ! -f $timed_data ]; then
  echo "Error: timed_data $timed_data does not exist";
  exit 1;
fi

get_biaswt_exec=$HOME/basetrade/scripts/get_generalprice_bias_wt.R;
echo $get_biaswt_exec $timed_data $pred_dur "&>" $work_dir/genprice_wts;
$get_biaswt_exec $timed_data $pred_dur 1>$work_dir/genprice_wts 2>$work_dir/genprice_wts.log;

if [ ! -f $work_dir/genprice_wts ]; then
  echo "Error: $work_dir/genprice_wts does not exist";
  exit 1;
fi

cat $work_dir/genprice_wts;
echo; echo;

baseprice=`awk '{if($1=="MODELINIT") { print $4; } }' $ilist`;

for i in `seq 1 $num_indicators`; do
  indc_str=`grep "^INDICATOR " $ilist | sed "${i}q;d" | cut -d" " -f3-`;

  bias_weight=`grep "^Indc $i" $work_dir/genprice_wts | grep "Bias Weight" | awk '{print $NF;}'`;
  if [ -z $bias_weight ]; then 
    echo "Bias Weight not present for indc $indc_str";
  else
    echo "Configline for indc $indc:";
    echo $shortcode $baseprice $bias_weight $indc_str | awk '{printf("%-24s %s\t%s\t%s ", $1,$2,$3,$4); for(i=5;i<=NF;i++) { printf "%s ", $i;} print ""; }';
  fi;
  echo;
done;

echo;
echo "Not deleting $work_dir for future use. Please remove it manually";

