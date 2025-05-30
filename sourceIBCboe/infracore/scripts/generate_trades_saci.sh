# only generate file. IND16 has the cron which reads these file and sends the mail
#!/bin/bash 
Log_Dir="/spare/local/logs/tradelogs"
Trade_Dir="/spare/local/ORSlogs/NSE_EQ/MSEQ2/";
Summary_Dir="/tmp";
saci_file="/tmp/rmc_saci_list";
mail_report="/tmp/saci_trade_position_report.txt";
declare -a strat_id
countpos=0

#strat_id=(`cat /home/dvctrader/ATHENA/run.sh | grep START | grep "^[^#]" | cut -d' ' -f6`)
strat_id=(123706)
date=`date +"%Y%m%d"`
mach=${HOSTNAME: -2}
echo "=================================IND$mach - ${strat_id[0]}================================="  > $mail_report
for sid in ${strat_id[@]}
do
    [[ -f $Log_Dir"/log.${date}.${sid}" ]] && ( grep "RMC_SACI" $Log_Dir"/log.${date}.${sid}"| awk '{print $6}' >$saci_file )
    [[ -f $Log_Dir"/log.${date}.${sid}.gz" ]] && ( zgrep "RMC_SACI" $Log_Dir"/log.${date}.${sid}.gz"| awk '{print $6}' >$saci_file )
    grep -f $saci_file $Trade_Dir"/trades."$date  > "/tmp/saci_trades."$date
    `/apps/anaconda/anaconda3/bin/python /home/pengine/prod/live_scripts/position_summary.py CM "/tmp/saci_trades."$date $Summary_Dir"/summary"$sid`
    while read line
    do 
       pos=`echo $line | cut -d' ' -f4`
       inst=`echo $line | cut -d' ' -f1`  
       if [[ $pos -ne 0 ]] ; then 
		echo "INSTRUMENT : $inst                                  POSITION : $pos " >> $mail_report
       fi
    done < $Summary_Dir"/summary"$sid
done
