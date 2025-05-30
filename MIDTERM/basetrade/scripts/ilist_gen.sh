#!/bin/bash

if [ $# -lt 5 ]; then echo "USAGE: <script> <shc_list> <pred_list> <filter_list> <bpx> <tpx> [tp_string=US_MORN_DAY] [corr_file_dir=/NAS1/indicatorwork] [ilist_dir=$HOME/archit/gen_ind/ilists]"; exit 1 ; fi

shc_list=$1; shift;
pred_list=$1; shift;
filter_list=$1; shift;
bpx=$1; shift; 
tpx=$1; shift;

tp_string="US_MORN_DAY";
corr_file_dir="/NAS1/indicatorwork";
if [ $# -gt 0 ]; then tp_string=$1; shift; fi
if [ $# -gt 0 ]; then corr_file_dir=$1; shift; fi

dir="$HOME/archit/gen_ind/ilists"; 
if [ $# -gt 0 ]; then dir=$1; shift; fi

if [ $bpx == "OrderWPrice" ] ; then sbpx="owp"; fi;
if [ $bpx == "OfflineMixMMS" ] ; then sbpx="OMix"; fi;
if [ $bpx == "Midprice" ] || [ $bpx == "MidPrice" ] ; then sbpx="mid"; fi;
if [ $bpx == "MktSizeWPrice" ] ; then sbpx="mkt"; fi;
if [ $bpx == "MktSinusoidal" ] ; then sbpx="sin"; fi;
if [ $bpx == "TradeWPrice" ] ; then sbpx="twp"; fi;

if [ $tpx == "OrderWPrice" ] ; then stpx="owp"; fi;
if [ $tpx == "OfflineMixMMS" ] ; then stpx="OMix"; fi;
if [ $tpx == "Midprice" ] || [ $tpx == "MidPrice" ] ; then stpx="mid"; fi;
if [ $tpx == "MktSizeWPrice" ] ; then stpx="mkt"; fi;
if [ $tpx == "MktSinusoidal" ] ; then stpx="sin"; fi;
if [ $tpx == "TradeWPrice" ] ; then stpx="twp"; fi;

uid=`date +%N`;
for shc in `cat $shc_list`; do 
  mkdir -p $dir/$shc; 
  for pred in `cat $pred_list`; do 
    for filter in `cat $filter_list`; do 
      file=$dir/$shc/ilist_"$shc"_"$sbpx"_"$stpx"_"$pred"_"$filter"_ab_corr; 
      ~/basetrade_install/bin/make_indicator_list $shc $corr_file_dir/"$shc"_"$pred"_"$filter"_na_e3_"$tp_string"_"$bpx"_"$tpx"/indicator_corr_record_file.txt.gz $bpx $tpx TODAY-85 TODAY-10 0.85 -a 1 2>/dev/null | head -n83 | grep -v "INDICATOREND" | awk '{if($1=="INDICATOR"){if( ($(NF-1)>0.1 && $(NF-2)>0.06) || ( $(NF-1)<-0.1 && $(NF-2)<-0.06) ){print $_;}} else{print $_;}}' > $file; 
      echo INDICATOREND >> $file; 
    done ; 

    comb_file=$dir/$shc/ilist_"$shc"_"$sbpx"_"$stpx"_"$pred"_ab_corr;
    echo -e "MODELINIT DEPBASE $shc $bpx $tpx\nMODELMATH LINEAR CHANGE\nINDICATORSTART" > $comb_file;
    cat $dir/$shc/ilist_"$shc"_"$sbpx"_"$stpx"_"$pred"_*_ab_corr | grep "INDICATOR " | awk -F"#" '{print $2, $1}' | awk '{if($1>0){print $1, $_} if($1<0){print -1*$1, $_}}' | sort -nrk1 | head -n200 | sort -k5 | uniq -f4 | sort -nrk1 | head -n100 | awk '{for(i=5;i<=NF;i++){printf "%s ", $i;}; print "#", $2, $3, $4; }' >> $comb_file;
    echo INDICATOREND >> $comb_file;
    echo $comb_file;
    
    grep "INDICATOR " $comb_file | awk -F"#" '{print $1}' > tmp1_"$uid"; 
    for filter in `cat $filter_list`; do 
      grep "INDICATOR " $dir/$shc/ilist_"$shc"_"$sbpx"_"$stpx"_"$pred"_"$filter"_ab_corr | awk -F"#" '{print $1}' > tmp2_"$uid"; 
      echo $filter `wc -l tmp1_"$uid" | awk '{print $1}'` `wc -l tmp2_"$uid" | awk '{print $1}'` `while read line; do grep "$line" tmp1_"$uid" ; done < tmp2_"$uid" | wc -l`; 
    done
    rm -f tmp1_"$uid" tmp2_"$uid";

  done; 
done
