#!/bin/bash

if [ $# -ne 1 ]; then echo "$0 DATE"; exit; fi

YYYYMMDD=$1;

mkdir -p $HOME/hrishav;

FEAT_DIR="$HOME/SampleData";


rm -f $HOME/hrishav/gen_sampledata_summary

for SHC in `ls -1 $FEAT_DIR` ; do if [ -d $FEAT_DIR/$SHC ]; then
  echo $HOME/basetrade_install/scripts/generate_sampledata_averages.pl $SHC $FEAT_DIR/$SHC $YYYYMMDD ;
fi; done > $HOME/hrishav/gen_sampledata_summary;

/home/dvctrader/dvccode/scripts/datainfra/celeryFiles/celeryClient/celeryScripts/run_my_job.py -n hagarwal -f $HOME/hrishav/gen_sampledata_summary ;

