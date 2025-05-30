#!/bin/bash

if [ $# -le 0 ] ; then echo "USAGE: $0 DATE=TODAY [SHCFILE=/spare/local/tradeinfo/midterm_shc.txt] [TIMEOUT=30]"; exit 0 ; fi

out_dir=/tmp/BarData
uid=`date +%N`
out_dir=$out_dir/$uid

date=TODAY
if [ $# -gt 0 ] ; then date=$1; shift; fi
if [ $date == "TODAY" ] ; then date=`date +%Y%m%d` ; fi
shc_file=/spare/local/tradeinfo/midterm_shc.txt;
if [ $# -gt 0 ] ; then shc_file=$1; shift; fi
timeout=30
if [ $# -gt 0 ] ; then timeout=$1; shift; fi

data_exec=$HOME/basetrade_install/bin/generate_bardata

for shc in `cat $shc_file`; do 
  cmd="$data_exec SIM $shc $date $out_dir $timeout"
  echo $cmd
  $cmd 
  file=$out_dir/$shc/$date
  if [ -e $file ] ; then
    cmd="gzip -f $file"
    echo $cmd
    $cmd
  fi
done

nas_dir=/apps/hftrap/BarData/$timeout
ssh dvcinfra@10.23.74.40 "mkdir -p $nas_dir"
rsync -ravz $out_dir/ dvcinfra@10.23.74.40:$nas_dir/
ssh dvcinfra@10.23.74.40 "chmod 755 -R $nas_dir"

rm -rf $out_dir
