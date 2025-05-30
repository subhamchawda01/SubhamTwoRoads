#!/bin/bash

if [ $# -lt 7 ]; then echo "$0 <shortcode> <event_dat_file> <event_time> <start_time> <end_time> <fld_path> <tm_dur_mins>"; exit; fi

shc=$1;
event_datf=$2;
event_t=$3;
event_st=$4;
event_et=$5;
wdir1=$6;
tm_dur=${@:7};

wdir2=$wdir1"/"$shc;
if [[ ! -d $wdir2 ]] ; then mkdir -p $wdir2 ; fi

shc_px_ilist=$wdir2"/px_ilist";
echo -e "MODELINIT DEPBASE $shc MktSizeWPrice MktSizeWPrice\nMODELMATH LINEAR CHANGE\nINDICATORSTART\nINDICATOREND\n" > $shc_px_ilist;

uniqid=`date +%N`;
tmpdir="/spare/local/temp/";
tmpdat=$tmpdir"/timed_data_af_"$uniqid;
datg=$tmpdir"/dgout_af_"$uniqid;

ev_timef=$wdir2"/ev_times";
echo -n > $datg;
echo -n >  $ev_timef;

for dt in `tail -n +2 $event_datf | cut -d, -f1`; do
  stt=`~/basetrade_install/bin/get_utc_hhmm_str $event_st  $dt`;
  ett=`~/basetrade_install/bin/get_utc_hhmm_str $event_et $dt`;
  echo ~/basetrade_install/bin/datagen $shc_px_ilist $dt $stt $ett $uniqid $tmpdat 500 0 0 1 ; 
  ~/basetrade_install/bin/datagen $shc_px_ilist $dt $stt $ett $uniqid $tmpdat 500 0 0 1 ; 
  awk -vnmf=$dt '{print nmf" "$0}' $tmpdat >> $datg ; 
  rm $tmpdat;
  evtt=`~/basetrade_install/bin/get_utc_hhmm_str $event_t $dt`;
  echo $dt","$evtt >> $ev_timef;
done

scaleR=$HOME"/basetrade/AflashScripts/findPriceChange.R";
outF=$wdir2"/pxchange.dat";
cmdR="Rscript $scaleR $ev_timef $datg $outF $tm_dur";
echo $cmdR;
$cmdR;
rm $datg;
# echo Rscript $scaleR $ev_timef $datg $outF $tm_dur;
# Rscript $scaleR $ev_timef $datg $tm_dur ;

