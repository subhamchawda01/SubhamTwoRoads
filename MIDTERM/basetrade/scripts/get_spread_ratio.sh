DATE=`date +'%Y%m%d'`

SHC="XT_0"
SYMBOL=`/home/dvcinfra/LiveExec/bin/get_exchange_symbol $SHC $DATE`  
Price_XT=`/home/dvcinfra/LiveExec/bin/mds_log_reader GENERIC /spare/local/MDSlogs/GENERIC/$SYMBOL"_"$DATE | grep Price | tail -n1 | cut -d':' -f2`


SHC="YT_0"
SYMBOL=`/home/dvcinfra/LiveExec/bin/get_exchange_symbol $SHC $DATE`  
Price_YT=`/home/dvcinfra/LiveExec/bin/mds_log_reader GENERIC /spare/local/MDSlogs/GENERIC/$SYMBOL"_"$DATE | grep Price | tail -n1 | cut -d':' -f2`

python /home/dvcinfra/LiveExec/scripts/xt_yt_spread_ratio.py $Price_XT $Price_YT

