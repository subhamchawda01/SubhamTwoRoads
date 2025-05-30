ssh dvctrader@10.23.74.51 << 'ENDSSH'
cp /spare/local/tradeinfo/NSE_Files/midterm_db /home/dvctrader/trash/midterm_db_temp
#cd /data1/apps/data/NSEBarData/Archive/Fut_Temp
#for i in *; do echo $i; /apps/anaconda/anaconda3/bin/python /home/dvctrader/infracore/scripts/MidTermDB_MinMaxPx_Updator.py $i ; done
ls /data1/apps/data/NSEBarData/Archive/Fut_Temp/* | awk 'BEGIN{FS="/"}{print $NF }' > tmp_tickers
for i in `head -2 tmp_tickers`; do /apps/anaconda/anaconda3/bin/python /home/dvctrader/infracore/scripts/MidTermDB_MinMaxPx_Updator.py $i; done
ENDSSH
