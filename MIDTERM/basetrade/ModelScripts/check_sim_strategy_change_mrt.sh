#!/bin/bash

server="dvctrader@10.23.74.52"
cmd="ssh $server \"ls -lt ~/LiveExec/bin/sim_strategy'.~'* | head -n10 | awk '{if (\\\$5 > 0) print \\\$9}' | head -n3\""
#echo $cmd

output=$(eval $cmd)
#echo $output
latest_sim_strategy="/home/dvctrader/LiveExec/bin/sim_strategy"
output=$latest_sim_strategy" "$output
#echo $output

TARGET_PATH="/spare/local/sim_strategy_version"
rm $TARGET_PATH/*

SIMRESULT=()
strat1="/home/dvctrader/modelling/mrt_strats/6jgc/strats/w_meanrevport_shclist_6jgc_30_300_3_mur1_maxspread3_med_threshold_EST_800_EST_1500_pfi1"
mail_body=""
for i in $output;
do
    `scp $server:$i /spare/\local/sim_strategy_version`
    file=`echo "$i" | rev | cut -d"/" -f1 | rev`
#    echo $file
    cmd="$TARGET_PATH/$file SIM $strat1 100000001 20171009 | grep SIMRESULT"
#    echo $cmd
    result=$(eval $cmd)
#    echo $result
    SIMRESULT+=("$result")
    mail_body=$mail_body$i" "$result"\n"
done

send_mail=false
for ((i = 0; i < ${#SIMRESULT[@]}; i++))
do
    if [[ "${SIMRESULT[0]}" != "${SIMRESULT[$i]}" ]] 
    then
#        echo ${SIMRESULT[0]}
#        echo ${SIMRESULT[$i]}
        send_mail=true
    fi
done

echo $send_mail
if [[ $send_mail == true ]] 
then 
    date=$(date +\%Y\%m\%d -d "yesterday")
    cmd="printf \"$mail_body\" | mail -s \"Sim Results Changed $date\" mehul.goyal@tworoads.co.in hrishav.agarwal@tworoads.co.in kaushik.putta@circulumvite.com"
#    echo $cmd
    eval $cmd
fi
