#!/bin/bash

if [ $# -lt 2 ] ; then echo "USAGE: $0 sim_trd_file live_trd_file"; exit 0; fi; 

sim_file_=$1;
live_file_=$2;

uid_=`date +%N`;
sfile_tmp_=/tmp/merge_trd_sim_"$uid_";
lfile_tmp_=/tmp/merge_trd_liv_"$uid_";
rm -rf $sfile_tmp_ $lfile_tmp_;

append_seq_time_script=$HOME/basetrade/SimScripts/append_real_seqtime_to_trd_file.pl

awk '{if($2=="OPEN" || $2=="FLAT"){print $_,"SIM"}}' $sim_file_ > $sfile_tmp_;
$append_seq_time_script $live_file_ | awk '{if($2=="OPEN" || $2=="FLAT"){print $_,"LIV"}}' > $lfile_tmp_;

#awk '{if($2=="OPEN" || $2=="FLAT"){print $_,"LIV"}}' $live_file_ > $lfile_tmp_;

cat $sfile_tmp_ $lfile_tmp_ | sort -nk1 | awk '{ if($NF=="SIM"){spos=$7; spnl=$9;}else{lpos=$7; lpnl=$9;} print $_, lpos-spos, lpnl-spnl;}' | column -t;
rm -rf $sfile_tmp_ $lfile_tmp_;
