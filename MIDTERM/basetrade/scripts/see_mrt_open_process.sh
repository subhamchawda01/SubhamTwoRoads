#!/bin/bash
source $HOME/.bash_aliases

# Config-file example
# 534830 fr216
# 534832 fr215
# 534836 fr216
# 534840 chi13
file="$HOME/dvccode/configs/mrt_query_server_map"
    while read -u10 line; do
      qid=`echo $line | awk '{print $1}'`;
      srv_str=`echo $line | awk '{print $2}'`;
      srv=`alias ssh"$srv_str" | awk '{print $3}'`;
      ssh dvctrader@$srv "ps -ef | grep $qid | awk '{if (NF==14) print \$12}'" 
    done 10< $file
